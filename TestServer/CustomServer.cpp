#include "CustomServer.h"

static const DWORD PIPE_OPEN_MODE = (PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED);
static const DWORD PIPE_MODE = (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT);
static const DWORD PIPE_CAPACITY = PIPE_UNLIMITED_INSTANCES;
static const DWORD PIPE_TIMEOUT = 5000;
static const LPSECURITY_ATTRIBUTES PIPE_SECURITY_SETTINGS = nullptr;

CustomAsynchServer::CustomAsynchServer(const std::wstring& pipe_path, 
                                       const DWORD capacity,
                                       const DWORD bufsize) :
    CustomAsynchNetworkAgent(pipe_path, capacity, bufsize)
{
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

        m_pipe[index] = CreateNamedPipe(m_pipePath.c_str(),
            PIPE_OPEN_MODE,
            PIPE_MODE,
            m_capacity,
            m_bufsize * sizeof(TCHAR),
            m_bufsize * sizeof(TCHAR),
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
}

void CustomAsynchServer::initConnect(const DWORD index)
{
    m_serviceOperationMutex.lock();

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

    m_serviceOperationMutex.unlock();
}

/**
void CustomAsynchServer::ConstructConnect(HANDLE& l_pipe)
{
    l_pipe = CreateNamedPipe(m_pipe_path.c_str(),
        PIPE_OPEN_MODE,
        PIPE_MODE,
        m_capacity,
        DEFAULT_BUFSIZE * sizeof(TCHAR),
        DEFAULT_BUFSIZE * sizeof(TCHAR),
        PIPE_TIMEOUT,
        PIPE_SECURITY_SETTINGS);

    if (l_pipe == INVALID_HANDLE_VALUE)
    {
        std::stringstream error_message;

        error_message << "[CustomServer::CustomServer()->CreateNamedPipe()] ";
        error_message << "Failed to create a named pipe object with GLE = ";
        error_message << GetLastError() << "." << std::endl;

        throw std::exception(error_message.str().c_str());
    }
}
**/
