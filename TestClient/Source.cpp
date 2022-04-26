#include "CustomClient.h"

static const LPCTSTR DEFAULT_PIPE_NAME = L"\\\\.\\pipe\\mynamedpipe";

void CopyReadInfo(std::wstring* l_dst_buffer, DWORD* l_dst_bytes_read,
    const std::wstring& l_src_buffer, const DWORD l_src_bytes)
{
    *l_dst_buffer = l_src_buffer;
    *l_dst_bytes_read = l_src_bytes;

    std::wcout << "[SERVER]: " << L"{ TEXT MESSAGE } = " << *l_dst_buffer;
    std::wcout << " { NUMBER BYTES READ } = " << *l_dst_bytes_read << ";";
    std::wcout << std::endl;
}

void CopyWriteInfo(DWORD* l_dst_bytes_read, const DWORD l_src_bytes)
{
    *l_dst_bytes_read = l_src_bytes;

    std::wcout << "[SERVICE INFO]: ";
    std::wcout << " { NUMBER BYTES WRITTEN } = " << *l_dst_bytes_read << ";";
    std::wcout << std::endl;
}

int main()
{
    try
    {
        CustomAsynchClient test_client(DEFAULT_PIPE_NAME);

        test_client.run();

        DWORD bytes_written = 0;

        if (test_client.write(0, L"Hello, world)))",
            CopyWriteInfo, &bytes_written) == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            test_client.write(0, L"Hello, world)))",
                CopyWriteInfo, &bytes_written);
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::wstring read_buffer = L"";
        DWORD bytes_read = 0;

        if (test_client.read(0, CopyReadInfo,
            &read_buffer, &bytes_read) == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            test_client.read(0, CopyReadInfo,
                &read_buffer, &bytes_read);
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