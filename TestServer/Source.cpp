#include "CustomServer.h"

static const LPCTSTR DEFAULT_PIPE_NAME = L"\\\\.\\pipe\\mynamedpipe";

void CopyReadInfo(const std::wstring& l_buffer_read, const DWORD l_bytes_read)
{
    std::wcout << "[SERVER]: " << L"{ TEXT MESSAGE } = " << l_buffer_read;
    std::wcout << " { NUMBER BYTES READ } = " << l_bytes_read << ";";
    std::wcout << std::endl;
}

void CopyWriteInfo(const DWORD l_bytes_written)
{
    std::wcout << "[SERVICE INFO]: ";
    std::wcout << " { NUMBER BYTES WRITTEN } = " << l_bytes_written << ";";
    std::wcout << std::endl;
}

int main()
{
    try
    {
        CustomAsynchServer test_server(DEFAULT_PIPE_NAME, 1);

        std::wstring read_buffer = L"";
        DWORD bytes_read = 0;

        test_server.run();

        if (test_server.read(0, &read_buffer, &bytes_read, CopyReadInfo) == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            test_server.read(0, &read_buffer, &bytes_read, CopyReadInfo);
        }

        DWORD bytes_written = 0;

        if (test_server.write(0, L"Hello, world)))", &bytes_written, CopyWriteInfo) == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            test_server.write(0, L"Hello, world)))", &bytes_written, CopyWriteInfo);
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));

        test_server.stop();
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }


    system("pause");
    return 0;
}