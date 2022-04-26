#include "CustomServer.h"

static const LPCTSTR DEFAULT_PIPE_NAME = L"\\\\.\\pipe\\mynamedpipe";

void CopyReadInfo(std::wstring* l_dst_buffer, DWORD* l_dst_bytes_read,
    const std::wstring& l_src_buffer, const DWORD l_src_bytes)
{
    *l_dst_buffer = l_src_buffer;
    *l_dst_bytes_read = l_src_bytes;

    std::wcout << "[CLIENT]: " << L"{ TEXT MESSAGE } = " << *l_dst_buffer;
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
        CustomAsynchServer test_server(DEFAULT_PIPE_NAME, 1);

        std::wstring read_buffer = L"";
        DWORD bytes_read = 0;

        test_server.run();

        if (test_server.read(0, CopyReadInfo,
            &read_buffer, &bytes_read) == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            test_server.read(0, CopyReadInfo,
                &read_buffer, &bytes_read);
        }

        DWORD bytes_written = 0;

        if (test_server.write(0, L"Hello, world)))",
            CopyWriteInfo, &bytes_written) == false)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            test_server.write(0, L"Hello, world)))",
                CopyWriteInfo, &bytes_written);
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