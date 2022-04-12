#include "CustomClient.h"

#include <sstream>
/**
----------------WORKING VERSION--------------------
static const LPCTSTR PIPE_NAME = TEXT("\\\\.\\pipe\\mynamedpipe");
static const DWORD PIPE_OPEN_MODE = GENERIC_READ | GENERIC_WRITE;
static const DWORD SHARING_MODE = NULL;
static const LPSECURITY_ATTRIBUTES PIPE_SECURITY_SETTINGS = nullptr;
static const DWORD EXISTING_PROPERTIES = OPEN_EXISTING;
static const DWORD PIPE_SETTINGS = NULL;
static const HANDLE TEMPLATE_FILE = NULL;

static const unsigned int MAX_NUMBER_OF_TRIES = 5;
static const DWORD TIMEOUT_MILISEC = 20000;

CustomClient::CustomClient(const LPCTSTR l_pipe_name):
    m_pipe_name(l_pipe_name)
{
    m_pipe = CreateFile(
        m_pipe_name,   // pipe name 
        PIPE_OPEN_MODE,  // read and write access ,
        SHARING_MODE,              // no sharing 
        PIPE_SECURITY_SETTINGS,           // default security attributes
        EXISTING_PROPERTIES,  // opens existing pipe 
        PIPE_SETTINGS,              // default attributes 
        TEMPLATE_FILE);
    
    //Check
    m_is_connected = (m_pipe != INVALID_HANDLE_VALUE);
}

CustomClient::~CustomClient()
{
    CloseHandle(m_pipe);
}

void CustomClient::WaitConnection()
{
    if (m_is_connected == false)
    {
        for (unsigned int try_index = 0; try_index < MAX_NUMBER_OF_TRIES; try_index)
        {
            m_pipe = CreateFile(
                m_pipe_name,   // pipe name 
                PIPE_OPEN_MODE,  // read and write access ,
                SHARING_MODE,              // no sharing 
                PIPE_SECURITY_SETTINGS,           // default security attributes
                EXISTING_PROPERTIES,  // opens existing pipe 
                PIPE_SETTINGS,              // default attributes 
                TEMPLATE_FILE);

            //Check if client succesfully connected
            if (m_pipe != INVALID_HANDLE_VALUE)
            {
                m_is_connected = true;
                break;
            }

            //Check that the reason of connection error was because of pipe business
            if (GetLastError() != ERROR_PIPE_BUSY)
            {
                throw std::exception("[CustomClient::CustomClient()] False error reason when connect to a named pipe.");
            }

            //Wait to connect again
            if (WaitNamedPipe(m_pipe_name, TIMEOUT_MILISEC) == NULL)
            {
                throw std::exception("[CustomClient::CustomClient()] Error while waiting named pipe to connect.");
            }
        }

    }

    //Final check
    if (m_pipe == INVALID_HANDLE_VALUE)
    {
        throw std::exception("[CustomClient::CustomClient()] Failed to connect to a named pipe.");
    }
    else
    {
        m_is_connected = true;

        DWORD mode = PIPE_READMODE_MESSAGE;
        bool success = SetNamedPipeHandleState(
            m_pipe,    // pipe handle 
            &mode,  // new pipe mode 
            NULL,     // don't set maximum bytes 
            NULL);    // don't set maximum time 
        if (success == false)
        {
            throw std::exception("[CustomClient::CustomClient()] Failed to change a named pipe state.");
        }
    }
}

std::wstring CustomClient::Recieve()
{
    std::wstring result = TEXT("");

    if (m_is_connected)
    {
        TCHAR read_buffer[DEFAULT_BUFSIZE] = { '\0' };
        DWORD bytes_processed = 0;

        bool reading_success = ReadFile(
            m_pipe,
            read_buffer,
            DEFAULT_BUFSIZE * sizeof(TCHAR),
            &bytes_processed,
            nullptr);

        if (!reading_success)
        {
            throw std::exception("[CustomClient::Recieve()] Error while reading client message.");
        }

        result = (read_buffer);
    }
    else
    {
        std::cout << "Pipe not connected. Can not read data from it.";
        std::cout << "Empty data was returned." << std::endl;
    }

    return result;
}

void CustomClient::Send(const std::wstring& message)
{
    if (m_is_connected)
    {
        DWORD bytes_processed = 0;

        bool writing_success = WriteFile(
            m_pipe,
            message.c_str(),
            DEFAULT_BUFSIZE * sizeof(TCHAR),
            &bytes_processed,
            nullptr);

        if (!writing_success)
        {
            throw std::exception("[CustomClient::Send()] Error while writing server message.");
        }
        else
        {
            std::cout << bytes_processed << " bytes of data was sent. ";
            std::wcout << "[TRANSFERED TEXT] = " << message << std::endl;
        }
    }
    else
    {
        std::cout << "Pipe not connected. Can not send data to it.";
    }
}
**/

//static const LPCTSTR PIPE_NAME = TEXT("\\\\.\\pipe\\mynamedpipe");
static const DWORD PIPE_OPEN_MODE = GENERIC_READ | GENERIC_WRITE;
static const DWORD SHARING_MODE = NULL;
static const LPSECURITY_ATTRIBUTES PIPE_SECURITY_SETTINGS = nullptr;
static const DWORD EXISTING_PROPERTIES = OPEN_EXISTING;
static const DWORD PIPE_SETTINGS = NULL;
static const HANDLE TEMPLATE_FILE = NULL;

static const unsigned int MAX_NUMBER_OF_TRIES = 5;
static const DWORD TIMEOUT_MILISEC = 5000;

CustomClient::CustomClient(const std::wstring& l_pipe_name):
    CustomNetworkAgent(l_pipe_name)
{

}

CustomClient::~CustomClient()
{
    Disconnect();
}

bool CustomClient::Disconnect()
{
    if (m_is_connected == true)
    {
        CloseHandle(m_pipe);
        m_is_connected = false;
    }
    
    return !m_is_connected;
}

bool CustomClient::Connect()
{
    if (m_is_connected == false)
    {
        std::cout << "[CustomClient::Connect()] Trying to connect to server..." << std::endl;

        m_pipe = CreateFile(
            m_pipe_name.c_str(),   // pipe name 
            PIPE_OPEN_MODE,  // read and write access ,
            SHARING_MODE,              // no sharing 
            PIPE_SECURITY_SETTINGS,           // default security attributes
            EXISTING_PROPERTIES,  // opens existing pipe 
            PIPE_SETTINGS,              // default attributes 
            TEMPLATE_FILE);

        bool is_file_created = (m_pipe != INVALID_HANDLE_VALUE);

        if (is_file_created == true)
        {
            std::cout << "[CustomClient::Connect()] Successfuly connected to server.";
            std::cout << "Trying to change pipe configuration for message exchange..." << std::endl;

            DWORD pipe_mode = PIPE_READMODE_MESSAGE;
            m_is_connected = SetNamedPipeHandleState(
                m_pipe,    // pipe handle 
                &pipe_mode,  // new pipe mode 
                NULL,     // don't set maximum bytes 
                NULL);

            if (m_is_connected == true)
            {
                std::cout << "[CustomClient::Connect()] Pipe configuration successfuly changed.";
                std::cout << std::endl;
            }
            else
            {
                std::cout << "[CustomClient::Connect()] Fail to change pipe configuration." << std::endl;
            }
        }
        else
        {
            std::cout << "[CustomClient::Connect()] Failed to connect to server." << std::endl;
        }
    }
    else
    {
        std::cout << "[CustomClient::Connect()] Client already connected." << std::endl;
    }

    return m_is_connected;

    /**if (m_is_connected == false)
    {
        m_pipe = CreateFile(
            m_pipe_name.c_str(),   // pipe name 
            PIPE_OPEN_MODE,  // read and write access ,
            SHARING_MODE,              // no sharing 
            PIPE_SECURITY_SETTINGS,           // default security attributes
            EXISTING_PROPERTIES,  // opens existing pipe 
            PIPE_SETTINGS,              // default attributes 
            TEMPLATE_FILE);

        //Check if client succesfully connected
        if (m_pipe != INVALID_HANDLE_VALUE)
        {
            m_is_connected = true;
            //break;
        }

        //Check that the reason of connection error was because of pipe business
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            throw std::exception("[CustomClient::Connect()] False error reason when connect to a named pipe.");
        }

        //Wait to connect again
        
        if (WaitNamedPipe(m_pipe_name.c_str(), TIMEOUT_MILISEC) == NULL)
        {
            throw std::exception("[CustomClient::Connect()] Error while waiting named pipe to connect.");
        }
        
    }

    //Final check
    if (m_pipe == INVALID_HANDLE_VALUE)
    {
        throw std::exception("[CustomClient::Connect()] Failed to connect to a named pipe.");
    }
    else
    {
        m_is_connected = true;

        DWORD mode = PIPE_READMODE_MESSAGE;
        bool success = SetNamedPipeHandleState(
            m_pipe,    // pipe handle 
            &mode,  // new pipe mode 
            NULL,     // don't set maximum bytes 
            NULL);    // don't set maximum time 
        if (success == false)
        {
            throw std::exception("[CustomClient::Connect()] Failed to change a named pipe state.");
        }
    }
    **/
}
