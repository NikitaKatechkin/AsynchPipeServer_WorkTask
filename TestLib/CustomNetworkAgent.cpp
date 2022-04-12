#include "CustomNetworkAgent.h"

//static const DWORD DEFAULT_BUFSIZE = 512;

CustomNetworkAgent::CustomNetworkAgent(const std::wstring& l_pipe_name):
	m_pipe_name(l_pipe_name)
{

}

bool CustomNetworkAgent::Send(const std::wstring& message, const DWORD& l_buffer_size)
{
    if (m_is_connected == false)
    {
        this->Connect();
    }

    if (m_is_connected == false)
    {
        throw std::exception(
            "[CustomClient::Send()] Sending message failed. Fail to establish connection.");
    }

    TCHAR* send_buffer = new TCHAR[static_cast<uint32_t>(l_buffer_size)];
    StringCchCopy(send_buffer, l_buffer_size, message.c_str());

    DWORD bytes_processed = 0;
    bool writing_success = WriteFile(
        m_pipe,
        send_buffer,
        l_buffer_size * sizeof(TCHAR),
        &bytes_processed,
        nullptr);

    if (writing_success == true)
    {
        std::cout << "[SERVICE INFO] " << bytes_processed << " bytes of data was sent. ";
        std::wcout << "{TRANSFERED TEXT} = " << message << std::endl;
    }
    else
    {
        std::cout << "[CustomClient::Send()] Error while writing server message." << std::endl;
    }

    delete[] send_buffer;
    send_buffer = nullptr;

    return writing_success;
}

bool CustomNetworkAgent::Recieve(std::wstring& l_buffer, const DWORD& l_buffer_size)
{
    if (m_is_connected == false)
    {
        this->Connect();
    }

    if (m_is_connected == false)
    {
        throw std::exception(
            "[CustomClient::Recieve()] Recieving message failed. Fail to establish connection.");
    }

    TCHAR* read_buffer = new TCHAR[static_cast<uint32_t>(l_buffer_size)] { '\0' };
    DWORD bytes_processed = 0;

    bool reading_success = ReadFile(
        m_pipe,
        read_buffer,
        l_buffer_size * sizeof(TCHAR),
        &bytes_processed,
        nullptr);

    if (reading_success == true)
    {
        l_buffer = read_buffer;
    }

    delete[] read_buffer;
    read_buffer = nullptr;

    return reading_success;
}
