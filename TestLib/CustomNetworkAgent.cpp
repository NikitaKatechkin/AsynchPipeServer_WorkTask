#include "CustomNetworkAgent.h"

CustomAsynchNetworkAgent::CustomAsynchNetworkAgent(const std::wstring& pipe_path,
    const DWORD capacity) :
    m_pipe_path(pipe_path),
    m_capacity(capacity)
{
    //ALLOCATING MEMORY START
    m_state = new Server_State[m_capacity]{ Server_State::Non_Initialized };
    m_event = new HANDLE[m_capacity]{ INVALID_HANDLE_VALUE };

    m_pipe = new HANDLE[m_capacity]{ INVALID_HANDLE_VALUE };
    m_overlapped = new OVERLAPPED[m_capacity]{ NULL };

    m_request_buffers = new TCHAR * [m_capacity] { nullptr };
    m_bytes_read = new DWORD[m_capacity]{ 0 };

    m_reply_buffers = new TCHAR * [m_capacity] { nullptr };
    m_bytes_written = new DWORD[m_capacity]{ 0 };

    for (DWORD index = 0; index < m_capacity; index++)
    {
        m_request_buffers[index] = new TCHAR[DEFAULT_BUFSIZE]{ '\0' };
        m_reply_buffers[index] = new TCHAR[DEFAULT_BUFSIZE]{ '\0' };
    }
    //ALLOCATING MEMORY END

    /**
    for (DWORD index = 0; index < m_capacity; index++)
    {
        m_event[index] = CreateEvent(NULL, TRUE, TRUE, NULL);

        if (m_event[index] == NULL)
        {
            std::stringstream error_message;

            error_message << "[CustomServer::CustomServer()->CreateEvent()] ";
            error_message << "Failed to create an event object with GLE = ";
            error_message << GetLastError() << "." << std::endl;

            throw std::exception(error_message.str().c_str());
        }

        m_overlapped[index].hEvent = m_event[index];

        //this->ConstructConnect(m_pipe[index]);
        ConstructConnect(m_pipe[index]);

        initConnect(index);
    }
    **/
}

CustomAsynchNetworkAgent::~CustomAsynchNetworkAgent()
{
    //stop();
    //delete m_process_loop_th;

    if (m_is_server_running == true)
    {
        this->stop();
    }

    delete[] m_state;
    delete[] m_event;

    for (DWORD index = 0; index < m_capacity; index++)
    {
        delete[] m_request_buffers[index];
        delete[] m_reply_buffers[index];
    }
    delete[] m_request_buffers;
    delete[] m_reply_buffers;

    for (DWORD index = 0; index < m_capacity; index++)
    {
        DisconnectNamedPipe(m_pipe[index]);

        if (m_pipe[index] != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_pipe[index]);
        }
    }
    delete[] m_pipe;

    delete[] m_overlapped;

    delete[] m_bytes_read;
    delete[] m_bytes_written;
}

void CustomAsynchNetworkAgent::run()
{
    m_is_server_running = true;
    m_process_loop_th = std::thread(&CustomAsynchNetworkAgent::processLoop, this);
}

void CustomAsynchNetworkAgent::stop()
{
    /**
    *
    * WORKING VERSION
    *
    m_is_server_running = false;
    m_process_loop_th.join();
    *
    *
    *
    **/

    /**
    bool is_all_pipe_handled = true;
    while (true)
    {
        is_all_pipe_handled = true;
        for (DWORD index = 0; index < m_capacity; index++)
        {
            if (!((m_state[index] == Server_State::Non_Initialized) ||
                (m_state[index] == Server_State::Disconnected) ||
                (m_state[index] == Server_State::Connected) ||
                (m_state[index] == Server_State::Connection_Pended)))
            {
                is_all_pipe_handled = false;
                break;
            }
        }

        if (is_all_pipe_handled == true)
        {
            m_is_server_running = false;
            break;
        }
    }
    **/

    m_is_server_running = false;
    SetEvent(m_event[0]);

    m_process_loop_th.join();
}

bool CustomAsynchNetworkAgent::read(const DWORD index,
    READ_CALLBACK read_callback,
    std::wstring* buffer,
    DWORD* bytes_read)
{
    if (m_state[index] != Server_State::Connected)
    {
        m_mutex.lock();

        std::cout << "[CustomServer::adoptedRead()] ";
        std::cout << "Could not perform read operation, because state of a named pipe with index = ";
        std::cout << index << " is " << static_cast<int>(m_state[index]);
        std::cout << " != " << static_cast<int>(Server_State::Connected) << std::endl;

        m_mutex.unlock();

        return false;
    }

    if (read_callback != nullptr)
    {
        this->m_read_callback = read_callback;
        m_read_callback_dst_buffer = buffer;
        m_callback_dst_bytes_read = bytes_read;
    }

    m_state[index] = Server_State::Reading_Signaled;

    SetEvent(m_overlapped[index].hEvent);

    return true;
}

bool CustomAsynchNetworkAgent::write(const DWORD index, const std::wstring& message,
    WRITE_CALLBACK write_callback,
    DWORD* bytes_written)
{
    if (m_state[index] != Server_State::Connected)
    {
        m_mutex.lock();

        std::cout << "[CustomServer::adoptedWrite()] ";
        std::cout << "Could not perform write operation, because state of a named pipe with index = ";
        std::cout << index << " is " << static_cast<int>(m_state[index]);
        std::cout << " != " << static_cast<int>(Server_State::Connected) << std::endl;

        m_mutex.unlock();

        return false;
    }

    if (write_callback != nullptr)
    {
        this->m_write_callback = write_callback;
        m_callback_dst_bytes_written = bytes_written;
    }

    StringCchCopy(m_reply_buffers[index], DEFAULT_BUFSIZE,
        message.c_str());

    m_state[index] = Server_State::Writing_Signaled;

    SetEvent(m_overlapped[index].hEvent);

    return true;
}

/**
void CustomAsynchNetworkAgent::ConstructConnect(HANDLE& pipe)
{
    pipe = CreateNamedPipe(m_pipe_path.c_str(),
        PIPE_OPEN_MODE,
        PIPE_MODE,
        m_capacity,
        DEFAULT_BUFSIZE * sizeof(TCHAR),
        DEFAULT_BUFSIZE * sizeof(TCHAR),
        PIPE_TIMEOUT,
        PIPE_SECURITY_SETTINGS);

    if (pipe == INVALID_HANDLE_VALUE)
    {
        std::stringstream error_message;

        error_message << "[CustomServer::CustomServer()->CreateNamedPipe()] ";
        error_message << "Failed to create a named pipe object with GLE = ";
        error_message << GetLastError() << "." << std::endl;

        throw std::exception(error_message.str().c_str());
    }
}
**/

void CustomAsynchNetworkAgent::processLoop()
{
    m_is_server_running = true;

    while (m_is_server_running == true)
    {
        DWORD index = 0;

        if (catchEvent(index) == false)
        {
            continue;
        }


        m_mutex.lock();
        std::cout << "Index = " << index << "; STATE = " << int(m_state[index]) << ";" << std::endl;
        m_mutex.unlock();

        switch (m_state[index])
        {
        case Server_State::Non_Initialized:
            initConnect(index);

            break;
        case Server_State::Connection_Pended:
            pendedConnect(index);

            break;
        case Server_State::Connected:
            ResetEvent(m_overlapped[index].hEvent);

            break;
        case Server_State::Reading_Pended:
            pendedRead(index);
            //exit_tmp_flag = true;
            break;
        case Server_State::Reading_Signaled:
            initRead(index);
            //exit_tmp_flag = true;
            break;
        case Server_State::Writing_Pended:
            pendedWrite(index);

            break;
        case Server_State::Writing_Signaled:
            initWrite(index);

            break;
        default:
            break;
        }
    }
}

bool CustomAsynchNetworkAgent::catchEvent(DWORD index)
{
    index = WaitForMultipleObjects(m_capacity,
        m_event,
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

void CustomAsynchNetworkAgent::initConnect(const DWORD index)
{
    m_mutex.lock();

    //TRYING TO CONNECT A NAMED PIPE

    bool is_connection_succeed = !ConnectNamedPipe(m_pipe[index],
        &m_overlapped[index]);

    if (is_connection_succeed == true)
    {
        switch (GetLastError())
        {
        case ERROR_IO_PENDING:
            //CONNECTION WAS PENDED -> SWITCH TO CONNECTION_PENDED STATE

            std::cout << "[SERVICE INFO] ";
            std::cout << "Connection was pended on a named pipe with index = ";
            std::cout << index << std::endl;

            m_state[index] = Server_State::Connection_Pended;

            break;
        case ERROR_PIPE_CONNECTED:
            //PIPE ALREADY CONNECTED -> TRY TO SET AN EVENT

            /**
            if (SetEvent(m_overlapped[l_index].hEvent) == TRUE)
            {
                //EVENT SUCCESSFULY SET -> SWITCH TO CONNECTED STATE

                std::cout << "[SERVICE INFO] ";
                std::cout << "Connection was succesfuly established";
                std::cout << " on a named pipe with index = " << l_index << std::endl;

                m_state[l_index] = SERVER_STATE::CONNECTED;
            }
            else
            {
                //FAILED TO SET EVENT OF A CONNECTED PIPE

                std::cout << "[CustomServer::InitConnect()->SetEvent()] ";
                std::cout << "Failed to set an uevent with index = " << l_index;
                std::cout << " with GLE = " << GetLastError() << "." << std::endl;
            }
            **/
            m_state[index] = Server_State::Connected;

            break;
        default:


            std::cout << "[CustomServer::InitConnect()->ConnectNamedPipe()] ";
            std::cout << "Unknown error occured on a named pipe with index = " << index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;

            break;
        }
    }
    else
    {
        std::cout << "[CustomServer::CustomServer()->ConnectNamedPipe()] ";
        std::cout << "Failed to connect a named pipe with index = " << index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }

    m_mutex.unlock();
}

void CustomAsynchNetworkAgent::pendedConnect(const DWORD index)
{
    m_mutex.lock();

    DWORD bytes_processed = 0;
    bool is_finished = false;

    //TRYING TO PERFORM PENDED CONNECT OPERATION
    is_finished = GetOverlappedResult(m_pipe[index],
        &m_overlapped[index],
        &bytes_processed,
        FALSE);

    if (is_finished == true)
    {
        //PIPE SUCCESFULY CONNECTED -> SWITCH TO CONNECTED STATE

        std::cout << "[SERVICE INFO]: ";
        std::cout << "Pipe with index = " << index << " successfuly connected.";
        std::cout << std::endl;

        m_state[index] = Server_State::Connected;
    }
    else
    {
        //OPERATION FAILED

        std::cout << "[CustomServer::PendedConnect()->GetOverlappedResult()] ";
        std::cout << "Failed to connect a named pipe with index = " << index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }

    m_mutex.unlock();
}

void CustomAsynchNetworkAgent::initRead(const DWORD index)
{
    m_mutex.lock();

    DWORD bytes_processed = 0;
    bool is_success = false;

    //TRYING TO READ A MESSAGE FROM A NAMED PIPE
    is_success = ReadFile(m_pipe[index],
        m_request_buffers[index],
        DEFAULT_BUFSIZE * sizeof(TCHAR),
        &bytes_processed,
        &m_overlapped[index]);

    if ((is_success == true) && (bytes_processed != 0))
    {
        //MESSAGE WAS SUCCESSFULY READ -> SWITCH TO CONNECTED STATE
        m_bytes_read[index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        if (m_read_callback != nullptr)
        {
            m_read_callback(m_read_callback_dst_buffer, m_callback_dst_bytes_read,
                m_request_buffers[index], m_bytes_read[index]);
        }

        //std::wcout << L"[CLIENT]: " << m_request_buffers[l_index] << std::endl;

        m_state[index] = Server_State::Connected;
    }
    else
    {
        DWORD error_code = GetLastError();

        if ((is_success == false) || (error_code == ERROR_IO_PENDING))
        {
            //READING OPERATION WAS PENDED -> SWITCH TO READING_PENDED STATE

            std::cout << "[SERVICE INFO] ";
            std::cout << "Read operation was pended on a named pipe with index = ";
            std::cout << index << std::endl;

            m_state[index] = Server_State::Reading_Pended;
        }
        else
        {
            //ERROR OCCURED WHILE ATTEMPTING TO READ DATA

            //TO-DO: HERE SHOULD BE RECONNECT()
            std::cout << "[CustomServer::InitRead()->ReadFile()] ";
            std::cout << "Failed to read data on a named pipe with index = " << index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;
        }
    }

    m_mutex.unlock();
}

void CustomAsynchNetworkAgent::pendedRead(const DWORD index)
{
    m_mutex.lock();

    DWORD bytes_processed = 0;
    bool is_finished = false;

    is_finished = GetOverlappedResult(m_pipe[index],
        &m_overlapped[index],
        &bytes_processed,
        FALSE);

    if ((is_finished == true) && (bytes_processed != 0))
    {
        //MESSAGE WAS SUCCESSFULY READ -> SWITCH TO CONNECTED STATE
        m_bytes_read[index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        if (m_read_callback != nullptr)
        {
            m_read_callback(m_read_callback_dst_buffer, m_callback_dst_bytes_read,
                m_request_buffers[index], m_bytes_read[index]);
        }

        //std::wcout << L"[CLIENT]: " << m_request_buffers[l_index] << std::endl;

        m_state[index] = Server_State::Connected;
    }
    else
    {
        // ERROR WAS OCCURED WHILE PERFORM PENDED OPERATION

        //TO-DO: HERE SHOULD BE RECONNECT()
        std::cout << "[CustomServer::PendedRead()->GetOverlappedResult()] ";
        std::cout << "Failed to read data on a named pipe with index = " << index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }

    m_mutex.unlock();
}

void CustomAsynchNetworkAgent::initWrite(const DWORD index)
{
    /**
    std::wstring l_buffer_to_write = L"Answer from server.";
    StringCchCopy(m_reply_buffers[l_index], DEFAULT_BUFSIZE,
                  l_buffer_to_write.c_str());
    **/

    m_mutex.lock();

    DWORD bytes_processed = 0;
    bool is_success = false;

    is_success = WriteFile(m_pipe[index],
        m_reply_buffers[index],
        DEFAULT_BUFSIZE * sizeof(TCHAR),
        &bytes_processed,
        &m_overlapped[index]);

    if ((is_success == true) && (bytes_processed != 0))
    {
        //MESSAGE WAS SUCCESSFULY SEND
        m_bytes_written[index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        if (m_write_callback != nullptr)
        {
            m_write_callback(m_callback_dst_bytes_written, m_bytes_written[index]);
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

            std::cout << "[SERVICE INFO] ";
            std::cout << "Write operation was pended on a named pipe with index = ";
            std::cout << index << std::endl;

            m_state[index] = Server_State::Writing_Pended;
        }
        else
        {
            //WRITE OPERATION WAS FAILED

            //TO-DO: HERE SHOULD BE RECONNECT()
            std::cout << "[CustomServer::InitWrite()->WriteFile()] ";
            std::cout << "Failed to write data on a named pipe with index = " << index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;
        }
    }

    m_mutex.unlock();
}

void CustomAsynchNetworkAgent::pendedWrite(const DWORD index)
{
    m_mutex.lock();

    DWORD bytes_processed = 0;
    bool is_finished = false;

    is_finished = GetOverlappedResult(m_pipe[index],
        &m_overlapped[index],
        &bytes_processed,
        FALSE);

    if ((is_finished == true) || (bytes_processed != 0))
    {
        // PENDED WRITE OPERATION SUCCEED -> SWITCH TO CONNECTED STATE
        m_bytes_written[index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        if (m_write_callback != nullptr)
        {
            m_write_callback(m_callback_dst_bytes_written, m_bytes_written[index]);
        }

        m_state[index] = Server_State::Connected;
    }
    else
    {
        // ERROR WAS OCCURED WHILE PERFORM PENDED OPERATION

        //TO-DO: HERE SHOULD BE RECONNECT()
        std::cout << "[CustomServer::PendedRead()->GetOverlappedResult()] ";
        std::cout << "Failed to write data on a named pipe with index = " << index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }

    m_mutex.unlock();
}

