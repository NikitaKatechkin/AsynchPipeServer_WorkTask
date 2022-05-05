#include "CustomNetworkAgent.h"

CustomAsynchNetworkAgent::CustomAsynchNetworkAgent(const std::wstring& pipePath,
    const DWORD capacity,
    const DWORD bufsize) :
    m_pipePath(pipePath),
    m_capacity(capacity),
    m_bufsize(bufsize)
{
    //ALLOCATING MEMORY START
    m_state = std::make_unique<Server_State[]>(m_capacity);
    m_event = std::make_unique<HANDLE[]>(m_capacity);

    m_pipe = std::make_unique<HANDLE[]>(m_capacity);
    m_overlapped = std::make_unique<OVERLAPPED[]>(m_capacity);

    m_requestBuffers = std::make_unique<std::unique_ptr<TCHAR[]>[]>(m_capacity);
    m_bytesRead = std::make_unique<DWORD[]>(m_capacity);

    m_replyBuffers = std::make_unique<std::unique_ptr<TCHAR[]>[]>(m_capacity);
    m_bytesWritten = std::make_unique<DWORD[]>(m_capacity);

    for (DWORD index = 0; index < m_capacity; index++)
    {
        m_requestBuffers[index] = std::unique_ptr<TCHAR[]>(new TCHAR[m_bufsize]);
        m_replyBuffers[index] = std::unique_ptr<TCHAR[]>(new TCHAR[m_bufsize]);
    }
    //ALLOCATING MEMORY END
}

CustomAsynchNetworkAgent::~CustomAsynchNetworkAgent()
{
    if (m_isServerRunning == true)
    {
        this->stop();
    }

    for (DWORD index = 0; index < m_capacity; index++)
    {
        DisconnectNamedPipe(m_pipe[index]);

        if (m_pipe[index] != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_pipe[index]);
        }
    }
}

void CustomAsynchNetworkAgent::run()
{
    m_isServerRunning = true;
    m_processLoopThread = std::thread(&CustomAsynchNetworkAgent::processLoop, this);
}

void CustomAsynchNetworkAgent::stop()
{
    m_isServerRunning = false;
    SetEvent(m_event[0]);

    m_processLoopThread.join();
}

bool CustomAsynchNetworkAgent::read(TCHAR* buffer,
                                    DWORD bytesRead,
                                    Callback readCallback, 
                                    const DWORD index)
{
    if (m_state[index] != Server_State::Connected)
    {
        m_printLogMutex.lock();

        std::cout << "[CustomServer::adoptedRead()] ";
        std::cout << "Could not perform read operation, because state of a named pipe with index = ";
        std::cout << index << " is " << static_cast<int>(m_state[index]);
        std::cout << " != " << static_cast<int>(Server_State::Connected) << std::endl;

        m_printLogMutex.unlock();

        return false;
    }

    std::lock_guard<std::mutex> operation_lock(m_serviceOperationMutex);


    m_readCallbackDstBuffer = buffer;
    //m_callbackDstBytesRead = bytesRead;
    m_bytesRead[index] = bytesRead;
    m_readCallback = readCallback;

    m_state[index] = Server_State::Reading_Signaled;

    SetEvent(m_overlapped[index].hEvent);

    //m_serviceOperationMutex.unlock();

    return true;
}

bool CustomAsynchNetworkAgent::write(const TCHAR* message,
                                     DWORD bytesWritten,
                                     Callback writeCallback, 
                                     const DWORD index)
{
    if (m_state[index] != Server_State::Connected)
    {
        m_printLogMutex.lock();

        std::cout << "[CustomServer::adoptedWrite()] ";
        std::cout << "Could not perform write operation, because state of a named pipe with index = ";
        std::cout << index << " is " << static_cast<int>(m_state[index]);
        std::cout << " != " << static_cast<int>(Server_State::Connected) << std::endl;

        m_printLogMutex.unlock();

        return false;
    }

    std::lock_guard<std::mutex> operation_lock(m_serviceOperationMutex);


    //m_callbackDstBytesWritten = bytesWritten;
    m_writeCallback = writeCallback;
    m_bytesWritten[index] = bytesWritten;

    memcpy_s(m_replyBuffers[index].get(), m_bytesWritten[index],
             message, m_bufsize * sizeof(TCHAR));

    m_state[index] = Server_State::Writing_Signaled;

    SetEvent(m_overlapped[index].hEvent);

    //m_serviceOperationMutex.unlock();


    return true;
}

void CustomAsynchNetworkAgent::processLoop()
{
    while (m_isServerRunning == true)
    {
        DWORD index = 0;

        if (waitForEvent(index) == false)
        {
            continue;
        }


        m_printLogMutex.lock();
        std::cout << "Index = " << index << "; STATE = " << int(m_state[index]) << ";" << std::endl;
        m_printLogMutex.unlock();

        switch (m_state[index])
        {
        case Server_State::Non_Initialized:
            initConnect(index);

            break;
        case Server_State::Connection_Pended:
            OnConnect(index);
            //OnPended(index);

            break;
        case Server_State::Connected:
            ResetEvent(m_overlapped[index].hEvent);

            break;
        case Server_State::Reading_Pended:
            OnRead(index);
            //OnPended(index);

            break;
        case Server_State::Reading_Signaled:
            initRead(index);
            //exit_tmp_flag = true;
            break;
        case Server_State::Writing_Pended:
            OnWrite(index);
            //OnPended(index);

            break;
        case Server_State::Writing_Signaled:
            initWrite(index);

            break;
        default:
            break;
        }
    }
}

bool CustomAsynchNetworkAgent::waitForEvent(DWORD& index)
{
    index = WaitForMultipleObjects(m_capacity,
        m_event.get(),
        FALSE,
        INFINITE);

    if (index == WAIT_FAILED)
    {
        std::cout << "[CustomServer::CatchEvent()->WaitForMultipleObjects()]" << std::endl;
        std::cout << "Failed to get an index of an event." << std::endl;
        return false;
    }
    else if (index == WAIT_TIMEOUT)
    {
        std::cout << "[CustomServer::CatchEvent()->WaitForMultipleObjects()]" << std::endl;
        std::cout << "No events was accured during specified amount of time." << std::endl;
        return false;
    }

    index -= WAIT_OBJECT_0;

    return (index < m_capacity);
}

void CustomAsynchNetworkAgent::initRead(const DWORD index)
{

    DWORD bytes_processed = 0;
    bool is_success = false;

    m_serviceOperationMutex.lock();

    //TRYING TO READ A MESSAGE FROM A NAMED PIPE
    is_success = ReadFile(m_pipe[index],
        m_requestBuffers[index].get(),
        m_bufsize * sizeof(TCHAR),
        &bytes_processed,
        &m_overlapped[index]);

    m_serviceOperationMutex.unlock();

    if ((is_success == true) && (bytes_processed != 0))
    {
        //ADD PROCESS IF READ 0 BYTES

        std::lock_guard<std::mutex> operation_lock(m_serviceOperationMutex);

        //MESSAGE WAS SUCCESSFULY READ -> SWITCH TO CONNECTED STATE
        //m_bytesRead[index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        memcpy_s(m_readCallbackDstBuffer, m_bytesRead[index],
                 m_requestBuffers[index].get(), m_bufsize * sizeof(TCHAR));

        //*m_callbackDstBytesRead = m_bytesRead[index];
        if (m_readCallback != nullptr)
        {
            m_printLogMutex.lock();

            m_readCallback(m_readCallbackDstBuffer, bytes_processed);

            m_printLogMutex.unlock();
        }

        //std::wcout << L"[CLIENT]: " << m_request_buffers[l_index] << std::endl;

        m_state[index] = Server_State::Connected;

        //m_serviceOperationMutex.unlock();
    }
    else
    {
        DWORD error_code = GetLastError();

        if ((is_success == false) || (error_code == ERROR_IO_PENDING))
        {
            std::lock_guard<std::mutex> print_lock(m_printLogMutex);

            //READING OPERATION WAS PENDED -> SWITCH TO READING_PENDED STATE

            std::cout << "[SERVICE INFO] ";
            std::cout << "Read operation was pended on a named pipe with index = ";
            std::cout << index << std::endl;

            //m_printLogMutex.unlock();

            std::lock_guard<std::mutex> operation_lock(m_serviceOperationMutex);

            m_state[index] = Server_State::Reading_Pended;

            //m_serviceOperationMutex.unlock();
        }
        else
        {
            //ERROR OCCURED WHILE ATTEMPTING TO READ DATA

            m_printLogMutex.lock();

            //TO-DO: HERE SHOULD BE RECONNECT()
            std::cout << "[CustomServer::InitRead()->ReadFile()] ";
            std::cout << "Failed to read data on a named pipe with index = " << index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;

            m_printLogMutex.unlock();

            std::lock_guard<std::mutex> operation_lock(m_serviceOperationMutex);

            m_readCallbackDstBuffer = nullptr;
            //*m_callbackDstBytesRead = GetLastError();
            if (m_readCallback != nullptr)
            {
                std::lock_guard<std::mutex> print_lock(m_printLogMutex);

                m_readCallback(m_readCallbackDstBuffer, GetLastError());
            }

            //m_serviceOperationMutex.unlock();
        }
    }
}

void CustomAsynchNetworkAgent::initWrite(const DWORD index)
{
    std::lock_guard<std::mutex> operation_lock(m_serviceOperationMutex);

    DWORD bytes_processed = 0;
    bool is_success = false;

    is_success = WriteFile(m_pipe[index],
        m_replyBuffers[index].get(),
        m_bufsize * sizeof(TCHAR),
        &bytes_processed,
        &m_overlapped[index]);

    if ((is_success == true) && (bytes_processed != 0))
    {
        //MESSAGE WAS SUCCESSFULY SEND
        //m_bytesWritten[index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        //m_callbackDstBytesWritten = m_bytesWritten[index];
        if (m_writeCallback != nullptr)
        {
            std::lock_guard<std::mutex> print_lock(m_printLogMutex);

            m_writeCallback(m_replyBuffers[index].get(), bytes_processed);
        }

        m_state[index] = Server_State::Connected;
    }
    else
    {
        //ERROR OCCURED WHILE ATTEMPTING TO SEND DATA

        DWORD error_code = GetLastError();

        if ((is_success == false) && (error_code == ERROR_IO_PENDING))
        {
            //WRITE OPERATION WAS PENDED

            std::lock_guard<std::mutex> print_lock(m_printLogMutex);

            std::cout << "[SERVICE INFO] ";
            std::cout << "Write operation was pended on a named pipe with index = ";
            std::cout << index << std::endl;

            m_state[index] = Server_State::Writing_Pended;
        }
        else
        {
            //WRITE OPERATION WAS FAILED

            //TO-DO: HERE SHOULD BE RECONNECT()
            m_printLogMutex.lock();

            std::cout << "[CustomServer::InitWrite()->WriteFile()] ";
            std::cout << "Failed to write data on a named pipe with index = " << index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;

            m_printLogMutex.unlock();
            //m_bytesWritten[index] = GetLastError();

            if (m_writeCallback != nullptr)
            {
                std::lock_guard<std::mutex> print_lock(m_printLogMutex);

                m_writeCallback(nullptr, GetLastError());
            }
        }
    }

    //m_serviceOperationMutex.unlock();
}

bool CustomAsynchNetworkAgent::OnPended(DWORD* bytes_processed, const DWORD index)
{
    bool is_success = false;

    m_serviceOperationMutex.lock();

    is_success = GetOverlappedResult(m_pipe[index],
                                     &m_overlapped[index],
                                     bytes_processed,
                                     FALSE);

    m_serviceOperationMutex.unlock();

    if (is_success == false)
    {
    // ERROR WAS OCCURED WHILE PERFORM PENDED OPERATION

        *bytes_processed = GetLastError();

        std::lock_guard<std::mutex> print_lock(m_printLogMutex);

        //TO-DO: HERE SHOULD BE RECONNECT()
        std::cout << "[CustomServer::PendedRead()->GetOverlappedResult()] ";
        std::cout << "Failed to read data on a named pipe with index = " << index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;

        //m_printLogMutex.unlock();
    }

    return is_success;
}

void CustomAsynchNetworkAgent::OnConnect(const DWORD index)
{
    DWORD bytes_processed = 0;
    
    if (OnPended(&bytes_processed, index) == true)
    {
        m_state[index] = Server_State::Connected;
    }
}

void CustomAsynchNetworkAgent::OnRead(const DWORD index)
{
    DWORD bytes_read = 0;
    bool is_success = OnPended(&bytes_read, index);

    if (is_success == true)
    {
        if (bytes_read != 0)
        {
            //MESSAGE WAS SUCCESSFULY READ -> SWITCH TO CONNECTED STATE
            //m_bytesRead[index] = *m_callbackDstBytesRead;

            //TO-DO: HERE SHOULD BE CALLBACK()
            memcpy_s(m_readCallbackDstBuffer, m_bytesRead[index],
                     m_requestBuffers[index].get(), m_bufsize * sizeof(TCHAR));
            if (m_readCallback != nullptr)
            {
                std::lock_guard<std::mutex> print_lock(m_printLogMutex);

                m_readCallback(m_readCallbackDstBuffer, bytes_read);

                //m_printLogMutex.unlock();
            }

            m_state[index] = Server_State::Connected;
        }
    }
    else
    {
        m_readCallbackDstBuffer = nullptr;
        if (m_readCallback != nullptr)
        {
            std::lock_guard<std::mutex> print_lock(m_printLogMutex);

            m_readCallback(m_readCallbackDstBuffer, GetLastError());

            //m_printLogMutex.unlock();
        }
    }
}

void CustomAsynchNetworkAgent::OnWrite(const DWORD index)
{
    DWORD bytes_written = 0;
    bool is_success = OnPended(&bytes_written, index);

    if (is_success == true)
    {
        if (bytes_written == m_bufsize * sizeof(TCHAR))
        {
            //MESSAGE WAS SUCCESSFULY READ -> SWITCH TO CONNECTED STATE
            //m_bytesWritten[index] = m_callbackDstBytesWritten;

            if (m_readCallback != nullptr)
            {
                std::lock_guard<std::mutex> print_lock(m_printLogMutex);

                m_writeCallback(m_replyBuffers[index].get() , bytes_written);

                //m_printLogMutex.unlock();
            }

            m_state[index] = Server_State::Connected;
        }
    }
    else
    {
        if (m_readCallback != nullptr)
        {
            std::lock_guard<std::mutex> print_lock(m_printLogMutex);

            m_writeCallback(m_replyBuffers[index].get() , GetLastError());

            //m_printLogMutex.unlock();
        }
    }
}
