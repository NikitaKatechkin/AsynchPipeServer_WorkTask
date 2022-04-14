/**
* 
* WORKING_VERSION 
* 
#include "CustomServer.h"

static const DWORD PIPE_OPEN_MODE = (PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED);
static const DWORD PIPE_MODE = (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT);
static const DWORD PIPE_CAPACITY = PIPE_UNLIMITED_INSTANCES;
static const DWORD PIPE_TIMEOUT = 5000;
static const LPSECURITY_ATTRIBUTES PIPE_SECURITY_SETTINGS = nullptr;

CustomServer::CustomServer(const std::wstring& l_pipe_name):
    m_pipe_name(l_pipe_name)
{
    m_event = CreateEvent(NULL, 
                          TRUE, 
                          TRUE, 
                          NULL);

    if (m_event == NULL)
    {
        std::stringstream error_message;

        error_message << "[CustomServer::CustomServer()->CreateEvent()]" << " ";
        error_message << "Failed to create an event." << std::endl;

        throw std::exception(error_message.str().c_str());
    }

    m_overlap.hEvent = m_event;

    m_pipe = CreateNamedPipe(m_pipe_name.c_str(), 
                             PIPE_OPEN_MODE, 
                             PIPE_MODE, 
                             255, 
                             DEFAULT_BUFSIZE * sizeof(TCHAR),
                             DEFAULT_BUFSIZE * sizeof(TCHAR), 
                             PIPE_TIMEOUT,
                             PIPE_SECURITY_SETTINGS);

    if (m_pipe == INVALID_HANDLE_VALUE)
    {
        std::stringstream error_message;

        error_message << "[CustomServer::CustomServer()->CreateNamedPipe()]" << " ";
        error_message << "Failed to create a named pipe with GLE = ";
        error_message << GetLastError() << "." << std::endl;

        throw std::exception(error_message.str().c_str());
    }

    m_is_pended = this->ConnectToNewClient(m_pipe, &m_overlap);
    m_state = m_is_pended ? SERVER_STATE::CONNECTION_IN_PROGRESS :
                            SERVER_STATE::READING_IN_PROGRESS;

    m_requests_buffer = new TCHAR[DEFAULT_BUFSIZE];
    m_replies_buffer = new TCHAR[DEFAULT_BUFSIZE];
}

CustomServer::~CustomServer()
{
    
}

void CustomServer::MainLoopVer2()
{
    DWORD bytes_processed, error_code;
    bool is_success;

    while (true)
    {
        DWORD event_index;
        if (CatchEvent(event_index) == false)
        {
            continue;
        }

        if (m_is_pended == true)
        {
            is_success = GetOverlappedResult(m_pipe, 
                                                  &m_overlap, 
                                                  &bytes_processed, 
                                                  FALSE);

            switch (m_state)
            {
            case SERVER_STATE::CONNECTION_IN_PROGRESS:
                if (is_success == false)
                {
                    std::stringstream error_message;

                    error_message << "[ CustomServer::MainLoopVer2()]" << " ";
                    error_message << "Failed with GLE = ";
                    error_message << GetLastError() << "." << std::endl;

                    throw std::exception(error_message.str().c_str());
                }

                m_state = SERVER_STATE::READING_IN_PROGRESS;

                break;
            case SERVER_STATE::READING_IN_PROGRESS:
                if ((is_success == false) || (bytes_processed == 0))
                {
                    //TO-DO
                    DisconnectAndReconnect();
                    continue;
                }

                m_bytes_read = bytes_processed;
                m_state = SERVER_STATE::WRITING_IN_PROGRESS;

                break;
            case SERVER_STATE::WRITING_IN_PROGRESS:
                if ((is_success == false) || (bytes_processed != m_bytes_write))
                {
                    //TO-DO
                    DisconnectAndReconnect();
                    continue;
                }

                m_state = SERVER_STATE::READING_IN_PROGRESS;

                break;
            default:
            {
                printf("Invalid pipe state.\n");
                return;
            }
            }
        }

        switch (m_state)
        {
        case SERVER_STATE::READING_IN_PROGRESS:
            is_success = ReadFile(m_pipe, 
                                       m_requests_buffer, 
                                       DEFAULT_BUFSIZE * sizeof(TCHAR),
                                       &m_bytes_read, 
                                       &m_overlap);

            if ((is_success == true) && (m_bytes_read != 0))
            {
                m_is_pended = false;
                m_state = SERVER_STATE::WRITING_IN_PROGRESS;
                continue;
            }

            error_code = GetLastError();
            if ((is_success == false) && (error_code == ERROR_IO_PENDING))
            {
                m_is_pended = true;
                continue;
            }

            DisconnectAndReconnect();

            break;
        case SERVER_STATE::WRITING_IN_PROGRESS:
            GetAnswerToRequest();

            is_success = WriteFile(m_pipe,
                                        m_replies_buffer,
                                        m_bytes_write,
                                        &bytes_processed,
                                        &m_overlap);

            if ((is_success == true) && (bytes_processed == m_bytes_write))
            {
                m_is_pended = false;
                m_state = SERVER_STATE::READING_IN_PROGRESS;
                continue;
            }

            error_code = GetLastError();
            if ((is_success == false) && (error_code == ERROR_IO_PENDING))
            {
                m_is_pended = true;
                continue;
            }

            DisconnectAndReconnect();

            break;
        default:
            break;
        
        }
    }
}

bool CustomServer::CatchEvent(DWORD& l_index)
{
    l_index = WaitForMultipleObjects(
        1,
        &m_event,
        FALSE,
        INFINITE);

    if (l_index == WAIT_FAILED)
    {
        std::cout << "[CustomServer::CatchEvent()->WaitForMultipleObjects()]" << std::endl;
        std::cout << "Occured event index is out of bounds" << std::endl;
        return false;
    }
    else if (l_index == WAIT_TIMEOUT)
    {
        std::cout << "[CustomServer::CatchEvent()->WaitForMultipleObjects()]" << std::endl;
        std::cout << "No events was accured during specified amount of time." << std::endl;
        return false;
    }

    l_index -= WAIT_OBJECT_0;



    return (l_index < 1);
}

BOOL CustomServer::ConnectToNewClient(HANDLE l_pipe, LPOVERLAPPED l_overlapped)
{
    BOOL fConnected, fPendingIO = FALSE;

    // Start an overlapped connection for this pipe instance. 
    fConnected = ConnectNamedPipe(l_pipe, l_overlapped);

    // Overlapped ConnectNamedPipe should return zero. 
    if (fConnected)
    {
        printf("ConnectNamedPipe failed with %d.\n", GetLastError());
        return FALSE;
    }

    switch (GetLastError())
    {
        // The overlapped connection in progress. 
    case ERROR_IO_PENDING:
        fPendingIO = TRUE;
        break;

        // Client is already connected, so signal an event. 

    case ERROR_PIPE_CONNECTED:
        if (SetEvent(l_overlapped->hEvent))
            break;

        // If an error occurs during the connect operation... 
    default:
    {
        printf("ConnectNamedPipe failed with %d.\n", GetLastError());
        return FALSE;
    }
    }

    return fPendingIO;
}

void CustomServer::DisconnectAndReconnect()
{
    // Disconnect the pipe instance. 

    if (DisconnectNamedPipe(m_pipe) == false)
    {
        printf("DisconnectNamedPipe failed with %d.\n", GetLastError());
    }

    // Call a subroutine to connect to the new client. 

    m_is_pended = this->ConnectToNewClient(m_pipe, &m_overlap);
    m_state = m_is_pended ? SERVER_STATE::CONNECTION_IN_PROGRESS :
                            SERVER_STATE::READING_IN_PROGRESS;
}

void CustomServer::GetAnswerToRequest()
{
    _tprintf(TEXT("[%d] %s\n"), m_pipe, m_requests_buffer);
    StringCchCopy(m_replies_buffer, DEFAULT_BUFSIZE, TEXT("Default answer from server"));
    m_bytes_write = (lstrlen(m_replies_buffer) + 1) * sizeof(TCHAR);
}
**/

/**
bool CustomServer::Connect(const size_t index)
{
    bool is_connected = ConnectNamedPipe(m_pipe[index], &m_overlap[index]);

    if (is_connected == true)
    {
        std::cout << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "[SERVICE INFO] " << "Failed to connect pipe." << std::endl;
        std::cout << "[SERVICE INFO] " << "***" << "Pipe instance number = ";
        std::cout << index << std::endl;
        std::cout << "[SERVICE INFO] " << "***" << "Pipe instance descriprot = ";
        std::cout << m_pipe[index] << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << std::endl;

        m_state[index] = SERVER_STATE::NON_INITIALIZED;
        return false;
    }

    switch (GetLastError())
    {
    case ERROR_IO_PENDING:
        std::cout << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "[SERVICE INFO] " << "New connection was pended." << std::endl;
        std::cout << "[SERVICE INFO] " << "Connection properties: " << std::endl;
        std::cout << "[SERVICE INFO] " << "***" << "Pipe instance number = ";
        std::cout << index << std::endl;
        std::cout << "[SERVICE INFO] " << "***" << "Pipe instance descriprot = ";
        std::cout << m_pipe[index] << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << std::endl;

        m_state[index] = SERVER_STATE::CONNECTION_IN_PROGRESS;
        //return true;
        break;
    case ERROR_PIPE_CONNECTED:
        if (SetEvent(m_overlap[index].hEvent) == TRUE)
        {
            std::cout << std::endl;
            std::cout << "------------------------------------------------" << std::endl;
            std::cout << "[SERVICE INFO] " << "New connection established." << std::endl;
            std::cout << "[SERVICE INFO] " << "Connection properties: " << std::endl;
            std::cout << "[SERVICE INFO] " << "***" << "Pipe instance number = ";
            std::cout << index << std::endl;
            std::cout << "[SERVICE INFO] " << "***" << "Pipe instance descriprot = ";
            std::cout << m_pipe[index] << std::endl;
            std::cout << "------------------------------------------------" << std::endl;
            std::cout << std::endl;

            m_state[index] = SERVER_STATE::CONNECTED;

            return true;
            break;
        }
    default:
        std::cout << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "[SERVICE INFO] " << "Undefined error was occured during connection.";
        std::cout << std::endl;
        std::cout << "[SERVICE INFO] " << "***" << "Pipe instance number = ";
        std::cout << index << std::endl;
        std::cout << "[SERVICE INFO] " << "***" << "Pipe instance descriprot = ";
        std::cout << m_pipe[index] << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << std::endl;

        m_state[index] = SERVER_STATE::NON_INITIALIZED;

        return false;
        break;
        //
        std::stringstream error_message;
        error_message << "[CustomServer::Connect()->ConnectNamedPipe()] ";
        error_message << "Failed to connect client to server. \n";

        throw std::exception(error_message.str().c_str());
        //
    }
    return false;
}

void CustomServer::Disconnect(const size_t index)
{
    if (m_state[index] != SERVER_STATE::NON_INITIALIZED)
    {
        std::cout << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "[SERVICE INFO] " << "Connection was demolish." << std::endl;
        std::cout << "[SERVICE INFO] " << "Connection properties: " << std::endl;
        std::cout << "[SERVICE INFO] " << "***" << "Pipe instance number = ";
        std::cout << index << std::endl;
        std::cout << "[SERVICE INFO] " << "***" << "Pipe instance descriprot = ";
        std::cout << m_pipe[index] << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << std::endl;

        DisconnectNamedPipe(m_pipe[index]);
    }
}

void CustomServer::MainLoop()
{
    while (true)
    {
        DWORD event_index;
        bool is_event_valid = CatchEvent(event_index);

        if (is_event_valid == false) 
        { 
            continue; 
        }

        if ((m_state[event_index] == SERVER_STATE::CONNECTION_IN_PROGRESS) || 
            (m_state[event_index] == SERVER_STATE::READING_IN_PROGRESS) || 
            (m_state[event_index] == SERVER_STATE::WRITING_IN_PROGRESS))
        {
            DWORD cbRet = 0;
            bool overlapped_operation_result = GetOverlappedResult(
                m_pipe[event_index], 
                &m_overlap[event_index], 
                &cbRet, 
                FALSE);

            switch (m_state[event_index])
            {
            case SERVER_STATE::CONNECTION_IN_PROGRESS:
                if (overlapped_operation_result == true)
                {
                    m_state[event_index] = SERVER_STATE::CONNECTED;
                }
                else
                {
                    m_state[event_index] = SERVER_STATE::NON_INITIALIZED;
                    continue;
                }

                break;
            case SERVER_STATE::READING_IN_PROGRESS:
                if (!(!overlapped_operation_result || cbRet == 0))
                {
                    m_state[event_index] = SERVER_STATE::CONNECTED;
                }
                else
                {
                    m_state[event_index] = SERVER_STATE::NON_INITIALIZED;
                    continue;
                }
                break;
            case SERVER_STATE::WRITING_IN_PROGRESS:
                break;
            default:
                break;
            }
        }

        switch (m_state[event_index])
        {
        case SERVER_STATE::READING_SIGNALED:
            //ReadFile()
            break;
        case SERVER_STATE::WRITING_SIGNALED:
            //WriteFile()
            break;
        case SERVER_STATE::CONNECTED:
            break;
        default:
            break;
        }
    }
}
**/

#include "CustomServer.h"

static const DWORD PIPE_OPEN_MODE = (PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED);
static const DWORD PIPE_MODE = (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT);
static const DWORD PIPE_CAPACITY = PIPE_UNLIMITED_INSTANCES;
static const DWORD PIPE_TIMEOUT = 5000;
static const LPSECURITY_ATTRIBUTES PIPE_SECURITY_SETTINGS = nullptr;

CustomServer::CustomServer(const std::wstring& l_pipe_path, 
                           const DWORD l_capacity):
    m_pipe_path(l_pipe_path),
    m_capacity(l_capacity)
{
    //ALLOCATING MEMORY START
    m_state = new SERVER_STATE[m_capacity]{ SERVER_STATE::NON_INITIALIZED };
    m_event = new HANDLE[m_capacity] { INVALID_HANDLE_VALUE };

    m_pipe = new HANDLE[m_capacity] { INVALID_HANDLE_VALUE };
    m_overlapped = new OVERLAPPED[m_capacity] { NULL };

    m_request_buffers = new TCHAR * [m_capacity] { nullptr };
    m_bytes_read = new DWORD[m_capacity] { 0 };

    m_reply_buffers = new TCHAR * [m_capacity] { nullptr };
    m_bytes_written = new DWORD[m_capacity]{ 0 };

    for (DWORD index = 0; index < m_capacity; index++)
    {
        m_request_buffers[index] = new TCHAR[DEFAULT_BUFSIZE] { '\0' };
        m_reply_buffers[index] = new TCHAR[DEFAULT_BUFSIZE] { '\0' };
    }
    //ALLOCATING MEMORY END

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

        m_pipe[index] = CreateNamedPipe(m_pipe_path.c_str(), 
                                        PIPE_OPEN_MODE, 
                                        PIPE_MODE, 
                                        m_capacity, 
                                        DEFAULT_BUFSIZE * sizeof(TCHAR), 
                                        DEFAULT_BUFSIZE * sizeof(TCHAR), 
                                        PIPE_TIMEOUT, 
                                        PIPE_SECURITY_SETTINGS);

        if (m_pipe[index] == INVALID_HANDLE_VALUE)
        {
            std::stringstream error_message;

            error_message << "[CustomServer::CustomServer()->CreateNamedPipe()] ";
            error_message << "Failed to create a named pipe object with GLE = ";
            error_message << GetLastError() << "." << std::endl;

            throw std::exception(error_message.str().c_str());
        }

        initConnect(index);
    }

    //m_process_loop_th = new std::thread(&CustomServer::ProcessLoopV2, this);
}

CustomServer::~CustomServer()
{
    //m_process_loop_th->join();
    stop();

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

/**
* 
* OLD VERSION
* 
void CustomServer::Connect(const DWORD& l_index)
{
    if ((m_state[l_index] == SERVER_STATE::NON_INITIALIZED) ||
        (m_state[l_index] == SERVER_STATE::DISCONNECTED))
    {
        //Connecting Process
        bool is_success = ConnectNamedPipe(m_pipe[l_index], 
                                           &m_overlapped[l_index]);

        if (is_success == true)
        {
            std::cout << "[CustomServer::CustomServer()->CreateEvent()] ";
            std::cout << "Failed to connect a named pipe with index = " << l_index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;

            return;
            //return false;
        }
        
        switch (GetLastError())
        {
        case ERROR_IO_PENDING:
            m_state[l_index] = SERVER_STATE::CONNECTION_PENDED;
            break;
            //return false;
        case ERROR_PIPE_CONNECTED:
            if (SetEvent(m_overlapped->hEvent))
            {
                m_state[l_index] = SERVER_STATE::CONNECTED;
                //return true;
            }
            else
            {
                //return false;
            }
            break;
        default:
            break;
            //return false;
        }
    }
    else if (m_state[l_index] == SERVER_STATE::CONNECTION_PENDED)
    {
        //Say to already pended
        std::cout << "[SERVICE INFO]: ";
        std::cout << "Pipe with index = " << l_index << " already connected.";
        std::cout << std::endl;

        //return false;
    }
    else
    {
        std::cout << "[SERVICE INFO]: ";
        std::cout << "Pipe with index = " << l_index << " already connected.";
        std::cout << std::endl;

        //return true;
    }
}
**/
/**
bool CustomServer::Connect(const DWORD& l_index)
{
    if ((m_state[l_index] == SERVER_STATE::NON_INITIALIZED) ||
        (m_state[l_index] == SERVER_STATE::DISCONNECTED))
    {
        //Connecting Process
        bool is_success = ConnectNamedPipe(m_pipe[l_index],
            &m_overlapped[l_index]);

        if (is_success == true)
        {
            std::cout << "[CustomServer::CustomServer()->CreateEvent()] ";
            std::cout << "Failed to connect a named pipe with index = " << l_index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;

            //return;
            return false;
        }

        switch (GetLastError())
        {
        case ERROR_IO_PENDING:
            m_state[l_index] = SERVER_STATE::CONNECTION_PENDED;
            //break;
            return false;
        case ERROR_PIPE_CONNECTED:
            if (SetEvent(m_overlapped->hEvent))
            {
                std::cout << "[SERVICE INFO]: ";
                std::cout << "Pipe with index = " << l_index << " successfuly connected.";

                m_state[l_index] = SERVER_STATE::CONNECTED;
                return true;
            }
            else
            {
                return false;
            }
            //break;
        default:
            //break;
            return false;
        }
    }
    else if (m_state[l_index] == SERVER_STATE::CONNECTION_PENDED)
    {
        //Say to already pended
        DWORD bytes_processed = 0;
        bool is_finished = GetOverlappedResult(m_pipe[l_index], 
                                               &m_overlapped[l_index], 
                                               &bytes_processed, 
                                               FALSE);

        if (is_finished == true)
        {
            std::cout << "[SERVICE INFO]: ";
            std::cout << "Pipe with index = " << l_index << " successfuly connected.";
            std::cout << std::endl;

            m_state[l_index] = SERVER_STATE::CONNECTED;
        }
        else
        {
            std::cout << "[SERVICE INFO]: ";
            std::cout << "Pipe with index = " << l_index << " pended.";
            std::cout << std::endl;
        }

        return is_finished;
    }
    else
    {
        std::cout << "[SERVICE INFO]: ";
        std::cout << "{STATE = " << int(m_state[l_index]) << " }";
        std::cout << "Pipe with index = " << l_index << " already connected.";
        std::cout << std::endl;

        return true;
    }
}

bool CustomServer::Disconnect(const DWORD& l_index)
{
    bool is_disconnected = (DisconnectNamedPipe(m_pipe[l_index]));

    if (is_disconnected == false)
    {
        std::cout << "[CustomServer::Disconnect()->DisconnectNamedPipe()] ";
        std::cout << "Failed to disconnect a named pipe with index = " << l_index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }
    else
    {
        std::cout << "[SERVICE INFO]: ";
        std::cout << "Pipe with index = " << l_index << " successfuly disconnected.";
        std::cout << std::endl;
    }

    return is_disconnected;
}

bool CustomServer::Recieve(const DWORD& l_index, std::wstring& l_buffer)
{
    if ((m_state[l_index] == SERVER_STATE::NON_INITIALIZED) ||
        (m_state[l_index] == SERVER_STATE::DISCONNECTED))
    {
        //std::cout << "HERE" << std::endl;

        if (this->Connect(l_index) == false)
        {
            return false;
        }
    }

    if (m_state[l_index] == SERVER_STATE::CONNECTED)
    {
        //std::cout << "HERE" << std::endl;

        DWORD bytes_processed = 0;
        bool is_success = ReadFile(m_pipe[l_index],
            m_request_buffers[l_index],
            DEFAULT_BUFSIZE * sizeof(TCHAR),
            &bytes_processed,
            &m_overlapped[l_index]);

        //std::cout << is_success << std::endl;
        //std::cout << bytes_processed << std::endl;

        if ((is_success == true) && (bytes_processed != 0))
        {
            //std::cout << "HERE 1" << std::endl;

            l_buffer = m_request_buffers[l_index];

            return true;
        }

        DWORD error_code = GetLastError();
        if ((is_success == false) || (error_code == ERROR_IO_PENDING))
        {
            m_state[l_index] = SERVER_STATE::READING_PENDED;

            return false;
        }

        return false;
    }
    else if (m_state[l_index] == SERVER_STATE::READING_PENDED)
    {
        DWORD bytes_processed = 0;
        bool is_finished = GetOverlappedResult(m_pipe[l_index],
            &m_overlapped[l_index],
            &bytes_processed,
            FALSE);

        if ((is_finished == false) || (bytes_processed == 0))
        {
            return false;
        }

        m_bytes_read[l_index] = bytes_processed;
        l_buffer = m_request_buffers[l_index];

        return true;
    }
    else
    {
        return false;
    }
}

bool CustomServer::Send(const DWORD& l_index, const std::wstring& l_buffer)
{
    if ((m_state[l_index] == SERVER_STATE::NON_INITIALIZED) ||
        (m_state[l_index] == SERVER_STATE::DISCONNECTED))
    {
        //std::cout << "HERE" << std::endl;

        if (this->Connect(l_index) == false)
        {
            return false;
        }
    }

    if (m_state[l_index] == SERVER_STATE::CONNECTED)
    {
        std::cout << "HERE" << std::endl;
        StringCchCopy(m_reply_buffers[l_index], DEFAULT_BUFSIZE, l_buffer.c_str());

        DWORD bytes_processed = 0;
        bool is_success = WriteFile(m_pipe[l_index],
            m_reply_buffers[l_index],
            DEFAULT_BUFSIZE * sizeof(TCHAR),
            &bytes_processed,
            &m_overlapped[l_index]);

        //std::cout << is_success << std::endl;
        //std::cout << bytes_processed << std::endl;

        if ((is_success == true) && (bytes_processed != 0))
        {
            //std::cout << "HERE 1" << std::endl;

            //l_buffer = m_request_buffers[l_index];

            return true;
        }

        DWORD error_code = GetLastError();
        if ((is_success == false) && (error_code == ERROR_IO_PENDING))
        {
            m_state[l_index] = SERVER_STATE::WRITING_PENDED;

            return false;
        }

        return false;
    }
    else if (m_state[l_index] == SERVER_STATE::WRITING_PENDED)
    {
        DWORD bytes_processed = 0;
        bool is_finished = GetOverlappedResult(m_pipe[l_index],
            &m_overlapped[l_index],
            &bytes_processed,
            FALSE);

        if ((is_finished == false) && (bytes_processed != m_bytes_written[l_index]))
        {
            return false;
        }

        m_bytes_read[l_index] = bytes_processed;
        //l_buffer = m_request_buffers[l_index];

        return true;
    }
    else
    {
        return false;
    }
}


void CustomServer::ProcessLoop()
{
    while (true)
    {
        DWORD index = 0;

        if (CatchEvent(index) == false)
        {
            continue;
        }
        
        switch (m_state[index])
        {
        case SERVER_STATE::NON_INITIALIZED:
            bool is_success = ConnectNamedPipe(m_pipe[index],
                                               &m_overlapped[index]);

            if (is_success == true)
            {
                std::cout << "[CustomServer::CustomServer()->CreateEvent()] ";
                std::cout << "Failed to connect a named pipe with index = " << index;
                std::cout << " with GLE = " << GetLastError() << "." << std::endl;
            }
            else
            {
                switch (GetLastError())
                {
                case ERROR_IO_PENDING:
                    m_state[index] = SERVER_STATE::CONNECTION_PENDED;

                    break;
                case ERROR_PIPE_CONNECTED:
                    if (SetEvent(m_overlapped[index].hEvent))
                    {
                        std::cout << "[SERVICE INFO]: ";
                        std::cout << "Pipe with index = " << index << " successfuly connected.";

                        m_state[index] = SERVER_STATE::CONNECTED;
                    }
                    else
                    {

                    }

                    break;
                default:
                    break;
                }
            }

            break;
        case SERVER_STATE::CONNECTION_PENDED:

            DWORD bytes_processed = 0;
            bool is_finished = GetOverlappedResult(m_pipe[index],
                &m_overlapped[index],
                &bytes_processed,
                FALSE);

            if (is_finished == true)
            {
                std::cout << "[SERVICE INFO]: ";
                std::cout << "Pipe with index = " << index << " successfuly connected.";
                std::cout << std::endl;

                m_state[index] = SERVER_STATE::CONNECTED;
            }
            else
            {
                std::cout << "[SERVICE INFO]: ";
                std::cout << "Pipe with index = " << index << " pended.";
                std::cout << std::endl;
            }

            break;
        case SERVER_STATE::READING_SIGNALED:

            DWORD bytes_processed = 0;
            bool is_success = ReadFile(m_pipe[index],
                m_request_buffers[index],
                DEFAULT_BUFSIZE * sizeof(TCHAR),
                &bytes_processed,
                &m_overlapped[index]);

            //std::cout << is_success << std::endl;
            //std::cout << bytes_processed << std::endl;

            if ((is_success == true) && (bytes_processed != 0))
            {
                //MESSAGE WAS SUCCESSFULY SEND
                // 
                //l_buffer = m_request_buffers[index];

                std::wcout << m_request_buffers[index] << std::endl;

                m_state[index] = SERVER_STATE::CONNECTED;
            }
            else
            {
                //ERROR OCCURED WHILE ATTEMPTING TO READ DATA

                DWORD error_code = GetLastError();

                if ((is_success == false) || (error_code == ERROR_IO_PENDING))
                {
                    m_state[index] = SERVER_STATE::READING_PENDED;
                }
                else
                {
                    //READ OPERATION WAS FAILED

                    //DISCONNECT FROM CLIENT
                }
            }

            break;
        case SERVER_STATE::READING_PENDED:

            DWORD bytes_processed = 0;
            bool is_finished = GetOverlappedResult(m_pipe[index],
                &m_overlapped[index],
                &bytes_processed,
                FALSE);

            if ((is_finished == false) || (bytes_processed == 0))
            {
                // ERROR WAS OCCURED WHILE PERFORM PENDED OPERATION
            }
            else
            {
                // PENDED READ OPERATION SUCCEED

                m_bytes_read[index] = bytes_processed;
                //l_buffer = m_request_buffers[l_index];
                std::wcout << L"[CLIENT]: " << m_request_buffers[index] << std::endl;

                m_state[index] = SERVER_STATE::CONNECTED;
            }

            break;
        case SERVER_STATE::WRITING_SIGNALED:

            //std::wstring l_buffer_to_write = L"Answer from server.";
            StringCchCopy(m_reply_buffers[index], DEFAULT_BUFSIZE, 
                std::wstring(L"Answer from server.").c_str());

            DWORD bytes_processed = 0;
            bool is_success = WriteFile(m_pipe[index],
                m_reply_buffers[index],
                DEFAULT_BUFSIZE * sizeof(TCHAR),
                &bytes_processed,
                &m_overlapped[index]);

            if ((is_success == true) && (bytes_processed != 0))
            {
                //MESSAGE WAS SUCCESSFULY SEND

                m_state[index] = SERVER_STATE::CONNECTED;
            }
            else
            {
                //ERROR OCCURED WHILE ATTEMPTING TO SEND DATA

                DWORD error_code = GetLastError();

                if ((is_success == false) && (error_code == ERROR_IO_PENDING))
                {
                    //WRITE OPERATION WAS PENDED
                    m_state[index] = SERVER_STATE::WRITING_PENDED;
                }
                else
                {
                    //WRITE OPERATION WAS FAILED

                    //DISCONNECT FROM CLIENT
                }
            }

            break;
        case SERVER_STATE::WRITING_PENDED:

            DWORD bytes_processed = 0;
            bool is_finished = GetOverlappedResult(m_pipe[index],
                &m_overlapped[index],
                &bytes_processed,
                FALSE);

            if ((is_finished == false) || (bytes_processed != m_bytes_written[index]))
            {
                // ERROR WAS OCCURED WHILE PERFORM PENDED OPERATION
            }
            else
            {
                // PENDED WRITE OPERATION SUCCEED

                //m_bytes_read[index] = bytes_processed;
                //l_buffer = m_request_buffers[l_index];
                //std::wcout << L"[CLIENT]: " << m_request_buffers[index] << std::endl;

                m_state[index] = SERVER_STATE::CONNECTED;
            }

            break;
        default:
            break;
        }
    }
}
**/

void CustomServer::processLoopV2()
{
    m_is_server_running = true;

    while (m_is_server_running == true)
    {
        DWORD index = 0;

        if (catchEvent(index) == false)
        {
            continue;
        }

        std::cout << "Index = " << index << "; STATE = " << int(m_state[index]) << ";" << std::endl;

        switch (m_state[index])
        {
        case SERVER_STATE::NON_INITIALIZED:
            initConnect(index);

            break;
        case SERVER_STATE::CONNECTION_PENDED:
            pendedConnect(index);

            break;
        case SERVER_STATE::CONNECTED:
            ResetEvent(m_overlapped[index].hEvent);

            break;
        case SERVER_STATE::READING_PENDED:
            pendedRead(index);
            //exit_tmp_flag = true;
            break;
        case SERVER_STATE::READING_SIGNALED:
            initRead(index);
            //exit_tmp_flag = true;
            break;
        case SERVER_STATE::WRITING_PENDED:
            pendedWrite(index);

            break;
        case SERVER_STATE::WRITING_SIGNALED:
            initWrite(index);

            break;
        default:
            break;
        }
    }
}

void CustomServer::run()
{
    m_process_loop_th = new std::thread(&CustomServer::processLoopV2, this);
    m_process_loop_th->detach();
}

void CustomServer::stop()
{
    //m_is_server_running = false;
    //m_process_loop_th->join();

    bool is_all_pipe_handled = true;
    while (true)
    {
        is_all_pipe_handled = true;
        for (DWORD index = 0; index < m_capacity; index++)
        {
            if (!((m_state[index] == SERVER_STATE::NON_INITIALIZED) ||
                (m_state[index] == SERVER_STATE::DISCONNECTED) ||
                (m_state[index] == SERVER_STATE::CONNECTED) ||
                (m_state[index] == SERVER_STATE::CONNECTION_PENDED)))
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
}

bool CustomServer::adoptedRead(const DWORD l_index,
                        void(*l_read_callback)(std::wstring* l_dst_buffer, DWORD* l_dst_bytes_read,
                             const std::wstring& l_src_buffer, const DWORD l_src_bytes),
                        std::wstring* l_buffer,
                        DWORD* l_bytes_read)
{
    if (m_state[l_index] != SERVER_STATE::CONNECTED)
    {
        std::cout << "[CustomServer::adoptedRead()] ";
        std::cout << "Could not perform read operation, because state of a named pipe with index = ";
        std::cout << l_index << " is " << static_cast<int>(m_state[l_index]);
        std::cout << " != " << static_cast<int>(SERVER_STATE::CONNECTED) << std::endl;

        return false;
    }

    if (l_read_callback != nullptr)
    {
        read_callback = l_read_callback;
        m_read_callback_dst_buffer = l_buffer;
        m_callback_dst_bytes_read = l_bytes_read;
    }

    m_state[l_index] = SERVER_STATE::READING_SIGNALED;

    SetEvent(m_overlapped[l_index].hEvent);

    return true;
}

bool CustomServer::adoptedWrite(const DWORD l_index, const std::wstring& l_message,
                                void(*l_write_callback)(DWORD* l_bytes_written,
                                    const DWORD l_src_bytes),
                                DWORD* l_bytes_written)
{
    if (m_state[l_index] != SERVER_STATE::CONNECTED)
    {
        std::cout << "[CustomServer::adoptedWrite()] ";
        std::cout << "Could not perform read operation, because state of a named pipe with index = ";
        std::cout << l_index << " is " << static_cast<int>(m_state[l_index]);
        std::cout << " != " << static_cast<int>(SERVER_STATE::CONNECTED) << std::endl;
         

        return false;
    }

    if (l_write_callback != nullptr)
    {
        write_callback = l_write_callback;
        m_callback_dst_bytes_written = l_bytes_written;
    }

    StringCchCopy(m_reply_buffers[l_index], DEFAULT_BUFSIZE,
                  l_message.c_str());

    m_state[l_index] = SERVER_STATE::WRITING_SIGNALED;

    SetEvent(m_overlapped[l_index].hEvent);

    return true;
}

//EXPERIMENTAL PART

bool CustomServer::catchEvent(DWORD l_index)
{
    l_index = WaitForMultipleObjects(m_capacity,
        m_event,
        FALSE,
        INFINITE);

    if (l_index == WAIT_FAILED)
    {
        std::cout << "[CustomServer::CatchEvent()->WaitForMultipleObjects()]" << std::endl;
        std::cout << "Failed to get an index of an event." << std::endl;
        return false;
    }
    else if (l_index == WAIT_TIMEOUT)
    {
        std::cout << "[CustomServer::CatchEvent()->WaitForMultipleObjects()]" << std::endl;
        std::cout << "No events was accured during specified amount of time." << std::endl;
        return false;
    }

    l_index -= WAIT_OBJECT_0;

    return (l_index < m_capacity);
}

void CustomServer::initConnect(const DWORD l_index)
{
    //TRYING TO CONNECT A NAMED PIPE

    bool is_connection_succeed = !ConnectNamedPipe(m_pipe[l_index],
                                                  &m_overlapped[l_index]);

    if (is_connection_succeed == true)
    {
        switch (GetLastError())
        {
        case ERROR_IO_PENDING:
            //CONNECTION WAS PENDED -> SWITCH TO CONNECTION_PENDED STATE

            std::cout << "[SERVICE INFO] ";
            std::cout << "Connection was pended on a named pipe with index = ";
            std::cout << l_index << std::endl;

            m_state[l_index] = SERVER_STATE::CONNECTION_PENDED;

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
            m_state[l_index] = SERVER_STATE::CONNECTED;

            break;
        default:


            std::cout << "[CustomServer::InitConnect()->ConnectNamedPipe()] ";
            std::cout << "Unknown error occured on a named pipe with index = " << l_index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;

            break;
        }
    }
    else
    {
        std::cout << "[CustomServer::CustomServer()->ConnectNamedPipe()] ";
        std::cout << "Failed to connect a named pipe with index = " << l_index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }
}

void CustomServer::pendedConnect(const DWORD l_index)
{
    DWORD bytes_processed = 0;
    bool is_finished = false;

    //TRYING TO PERFORM PENDED CONNECT OPERATION
    is_finished = GetOverlappedResult(m_pipe[l_index], 
                                      &m_overlapped[l_index], 
                                      &bytes_processed, 
                                      FALSE);

    if (is_finished == true)
    {
        //PIPE SUCCESFULY CONNECTED -> SWITCH TO CONNECTED STATE

        std::cout << "[SERVICE INFO]: ";
        std::cout << "Pipe with index = " << l_index << " successfuly connected.";
        std::cout << std::endl;

        m_state[l_index] = SERVER_STATE::CONNECTED;
    }
    else
    {
        //OPERATION FAILED

        std::cout << "[CustomServer::PendedConnect()->GetOverlappedResult()] ";
        std::cout << "Failed to connect a named pipe with index = " << l_index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }
}

void CustomServer::initRead(const DWORD l_index)
{
    DWORD bytes_processed = 0;
    bool is_success = false;

    //TRYING TO READ A MESSAGE FROM A NAMED PIPE
    is_success = ReadFile(m_pipe[l_index],
                          m_request_buffers[l_index],
                          DEFAULT_BUFSIZE * sizeof(TCHAR),
                          &bytes_processed,
                          &m_overlapped[l_index]);

    if ((is_success == true) && (bytes_processed != 0))
    {
        //MESSAGE WAS SUCCESSFULY READ -> SWITCH TO CONNECTED STATE
        m_bytes_read[l_index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        if (read_callback != nullptr)
        {
            read_callback(m_read_callback_dst_buffer, m_callback_dst_bytes_read, 
                          m_request_buffers[l_index], m_bytes_read[l_index]);
        }

        //std::wcout << L"[CLIENT]: " << m_request_buffers[l_index] << std::endl;

        m_state[l_index] = SERVER_STATE::CONNECTED;
    }
    else
    {
        DWORD error_code = GetLastError();

        if ((is_success == false) || (error_code == ERROR_IO_PENDING))
        {
            //READING OPERATION WAS PENDED -> SWITCH TO READING_PENDED STATE

            std::cout << "[SERVICE INFO] ";
            std::cout << "Read operation was pended on a named pipe with index = ";
            std::cout << l_index << std::endl;

            m_state[l_index] = SERVER_STATE::READING_PENDED;
        }
        else
        {
            //ERROR OCCURED WHILE ATTEMPTING TO READ DATA

            //TO-DO: HERE SHOULD BE RECONNECT()
            std::cout << "[CustomServer::InitRead()->ReadFile()] ";
            std::cout << "Failed to read data on a named pipe with index = " << l_index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;
        }
    }
}

void CustomServer::pendedRead(const DWORD l_index)
{
    DWORD bytes_processed = 0;
    bool is_finished = false;
    
    is_finished = GetOverlappedResult(m_pipe[l_index],
                                      &m_overlapped[l_index],
                                      &bytes_processed,
                                      FALSE);

    if ((is_finished == true) && (bytes_processed != 0))
    {
        //MESSAGE WAS SUCCESSFULY READ -> SWITCH TO CONNECTED STATE
        m_bytes_read[l_index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        if (read_callback != nullptr)
        {
            read_callback(m_read_callback_dst_buffer, m_callback_dst_bytes_read,
                          m_request_buffers[l_index], m_bytes_read[l_index]);
        }

        //std::wcout << L"[CLIENT]: " << m_request_buffers[l_index] << std::endl;

        m_state[l_index] = SERVER_STATE::CONNECTED;
    }
    else
    {
        // ERROR WAS OCCURED WHILE PERFORM PENDED OPERATION

        //TO-DO: HERE SHOULD BE RECONNECT()
        std::cout << "[CustomServer::PendedRead()->GetOverlappedResult()] ";
        std::cout << "Failed to read data on a named pipe with index = " << l_index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }
}

void CustomServer::initWrite(const DWORD l_index)
{
    /**
    std::wstring l_buffer_to_write = L"Answer from server.";
    StringCchCopy(m_reply_buffers[l_index], DEFAULT_BUFSIZE,
                  l_buffer_to_write.c_str());
    **/

    DWORD bytes_processed = 0;
    bool is_success = false;
    
    is_success = WriteFile(m_pipe[l_index],
                           m_reply_buffers[l_index],
                           DEFAULT_BUFSIZE * sizeof(TCHAR),
                           &bytes_processed,
                           &m_overlapped[l_index]);

    if ((is_success == true) && (bytes_processed != 0))
    {
        //MESSAGE WAS SUCCESSFULY SEND
        m_bytes_written[l_index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        if (write_callback != nullptr)
        {
            write_callback(m_callback_dst_bytes_written, m_bytes_written[l_index]);
        }

        m_state[l_index] = SERVER_STATE::CONNECTED;
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
            std::cout << l_index << std::endl;

            m_state[l_index] = SERVER_STATE::WRITING_PENDED;
        }
        else
        {
            //WRITE OPERATION WAS FAILED

            //TO-DO: HERE SHOULD BE RECONNECT()
            std::cout << "[CustomServer::InitWrite()->WriteFile()] ";
            std::cout << "Failed to write data on a named pipe with index = " << l_index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;
        }
    }
}

void CustomServer::pendedWrite(const DWORD l_index)
{
    DWORD bytes_processed = 0;
    bool is_finished = false;
    
    is_finished = GetOverlappedResult(m_pipe[l_index],
                                    &m_overlapped[l_index],
                                    &bytes_processed,
                                    FALSE);

    if ((is_finished == true) || (bytes_processed != 0))
    {
        // PENDED WRITE OPERATION SUCCEED -> SWITCH TO CONNECTED STATE
        m_bytes_written[l_index] = bytes_processed;

        //TO-DO: HERE SHOULD BE CALLBACK()
        if (write_callback != nullptr)
        {
            write_callback(m_callback_dst_bytes_written, m_bytes_written[l_index]);
        }

        m_state[l_index] = SERVER_STATE::CONNECTED;
    }
    else
    {
        // ERROR WAS OCCURED WHILE PERFORM PENDED OPERATION
        
        //TO-DO: HERE SHOULD BE RECONNECT()
        std::cout << "[CustomServer::PendedRead()->GetOverlappedResult()] ";
        std::cout << "Failed to write data on a named pipe with index = " << l_index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }
}

