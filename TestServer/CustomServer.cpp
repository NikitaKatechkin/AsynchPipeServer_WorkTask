#include "CustomServer.h"

static const DWORD PIPE_OPEN_MODE = (PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED);
static const DWORD PIPE_MODE = (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT);
static const DWORD PIPE_CAPACITY = PIPE_UNLIMITED_INSTANCES;
static const DWORD PIPE_TIMEOUT = 5000;
static const LPSECURITY_ATTRIBUTES PIPE_SECURITY_SETTINGS = nullptr;

CustomAsynchServer::CustomAsynchServer(const std::wstring& pipe_path,
    const DWORD capacity) :
    CustomAsynchNetworkAgent(pipe_path, capacity)
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
}

CustomAsynchServer::~CustomAsynchServer()
{
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
