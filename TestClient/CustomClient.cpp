#include "CustomClient.h"

static const DWORD PIPE_OPEN_MODE = GENERIC_READ | GENERIC_WRITE;
static const DWORD SHARING_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
static const LPSECURITY_ATTRIBUTES PIPE_SECURITY_SETTINGS = nullptr;
static const DWORD EXISTING_PROPERTIES = OPEN_EXISTING;
static const DWORD PIPE_SETTINGS = NULL;
static const HANDLE TEMPLATE_FILE = NULL;

static const unsigned int MAX_NUMBER_OF_TRIES = 5;
static const DWORD TIMEOUT_MILISEC = 5000;

CustomAsynchClient::CustomAsynchClient(const std::wstring& pipe_path,
                                       const DWORD bufsize):
    CustomAsynchNetworkAgent(pipe_path, 1, bufsize)
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

        initConnect(index);
    }
}

void CustomAsynchClient::initConnect(const DWORD index)
{
    m_serviceOperationMutex.lock();

    //TRYING TO CONNECT A NAMED PIPE

    m_pipe[index] = CreateFile(m_pipePath.c_str(),   // pipe name 
        PIPE_OPEN_MODE,  // read and write access ,
        SHARING_MODE,              // no sharing 
        PIPE_SECURITY_SETTINGS,           // default security attributes
        EXISTING_PROPERTIES,  // opens existing pipe 
        PIPE_SETTINGS,              // default attributes 
        TEMPLATE_FILE);

    bool is_connection_succeed = (m_pipe[index] != INVALID_HANDLE_VALUE);

    if (is_connection_succeed == true)
    {
        std::cout << "[CustomAsynchClient::ConstructConnect()] Successfuly connected to server.";
        std::cout << "Trying to change pipe configuration for message exchange..." << std::endl;

        DWORD pipe_mode = PIPE_READMODE_MESSAGE;
        bool is_connected = SetNamedPipeHandleState(m_pipe[index],    // pipe handle 
            &pipe_mode,  // new pipe mode 
            NULL,     // don't set maximum bytes 
            NULL);

        if (is_connected == true)
        {
            std::cout << "[CustomAsynchClient::ConstructConnect()]";
            std::cout << " Pipe configuration successfuly changed.";
            std::cout << std::endl;

            m_state[index] = Server_State::Connected;
        }
        else
        {
            std::stringstream error_message;

            error_message << "[CustomAsynchClient::ConstructConnect()] ";
            error_message << "Failed to create a named pipe object with GLE = ";
            error_message << GetLastError() << "." << std::endl;

            throw std::exception(error_message.str().c_str());
        }

        /**
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

            m_state[index] = Server_State::Connected;

            break;
        default:


            std::cout << "[CustomServer::InitConnect()->ConnectNamedPipe()] ";
            std::cout << "Unknown error occured on a named pipe with index = " << index;
            std::cout << " with GLE = " << GetLastError() << "." << std::endl;

            break;
        }
        **/
    }
    else
    {
        std::cout << "[CustomServer::CustomServer()->ConnectNamedPipe()] ";
        std::cout << "Failed to connect a named pipe with index = " << index;
        std::cout << " with GLE = " << GetLastError() << "." << std::endl;
    }

    m_serviceOperationMutex.unlock();
}
