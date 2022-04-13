#include "CustomClient.h"

static const LPCTSTR DEFAULT_PIPE_NAME = L"\\\\.\\pipe\\mynamedpipe";


int main()
{
    CustomClient test_clent(DEFAULT_PIPE_NAME);

    test_clent.Connect();

    test_clent.Send(L"Message from client.");

    //std::this_thread::sleep_for(std::chrono::seconds(1));

    std::wstring buffer;
    std::wcout << L"[SERVER]: " << L"{STATUS = " << test_clent.Recieve(buffer) << L"} - ";
    std::wcout << buffer << std::endl;

    test_clent.Disconnect();

    system("pause");
    return 0;
    /**
    CustomClient test_clent(DEFAULT_PIPE_NAME);
    test_clent.Connect();

    test_clent.Send(L"Message from client.");
    
    std::wstring buffer;
    std::wcout << L"[SERVER]: " << L"{STATUS = " << test_clent.Recieve(buffer) << L"} - ";
    std::wcout << buffer << std::endl;

    test_clent.Disconnect();

    system("pause");
    return 0;
    **/
}