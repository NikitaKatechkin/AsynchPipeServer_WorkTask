#include "CustomClient.h"

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
        CustomAsynchClient test_client(DEFAULT_PIPE_NAME);

        test_client.run();

        DWORD bytes_written = 0;

        if (test_client.write(0, L"Hello, world)))", &bytes_written, CopyWriteInfo) == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            test_client.write(0, L"Hello, world)))", &bytes_written, CopyWriteInfo);
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::wstring read_buffer = L"";
        DWORD bytes_read = 0;

        if (test_client.read(0, &read_buffer, &bytes_read, CopyReadInfo) == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            test_client.read(0, &read_buffer, &bytes_read, CopyReadInfo);
        }

        test_client.stop();
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }


    system("pause");
    return 0;
}