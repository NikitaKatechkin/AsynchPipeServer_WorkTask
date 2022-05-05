#include "pch.h"
#include <TestClient/CustomClient.h>

namespace TestToolkit
{
	void CopyReadInfo(const TCHAR* l_buffer_read, const DWORD l_bytes_read);

	void CopyWriteInfo(const TCHAR* l_buffer_write, const DWORD l_bytes_written);
}

TEST(CustomAsynchClientTestCase, CreateTest) 
{
	const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";

	HANDLE server = CreateNamedPipe(L"\\\\.\\pipe\\mynamedpipe",
									PIPE_ACCESS_DUPLEX,
									PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
									PIPE_UNLIMITED_INSTANCES,
									512 * sizeof(TCHAR),
									512 * sizeof(TCHAR),
									0,
									NULL);

	CustomAsynchClient client(pipe_path);

	ConnectNamedPipe(server, nullptr);

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

TEST(CustomAsynchClientTestCase, RunStopTest)
{
	const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";

	HANDLE server = CreateNamedPipe(L"\\\\.\\pipe\\mynamedpipe",
									PIPE_ACCESS_DUPLEX,
									PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
									PIPE_UNLIMITED_INSTANCES,
									512 * sizeof(TCHAR),
									512 * sizeof(TCHAR),
									0,
									NULL);

	CustomAsynchClient client(pipe_path);

	client.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	ConnectNamedPipe(server, nullptr);

	client.stop();

	EXPECT_EQ(true, true);

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

TEST(CustomAsynchClientTestCase, ReadTest)
{
	const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	HANDLE server = CreateNamedPipe(L"\\\\.\\pipe\\mynamedpipe",
									PIPE_ACCESS_DUPLEX,
									PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
									PIPE_UNLIMITED_INSTANCES,
									512 * sizeof(TCHAR),
									512 * sizeof(TCHAR),
									0,
									NULL);

	CustomAsynchClient client(pipe_path);

	client.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	ConnectNamedPipe(server, nullptr);

	DWORD bytes_written = 0;
	const TCHAR write_buffer[sizeof(TCHAR) * 512] = L"Hello world)))";
	WriteFile(server, write_buffer, 512 * sizeof(TCHAR), &bytes_written, nullptr);

	TCHAR* read_buffer = new TCHAR[sizeof(TCHAR) * 512];
	DWORD bytes_read = 512 * sizeof(TCHAR);
	client.read(read_buffer, bytes_read, nullptr);
	
	while (bytes_read != bytes_written)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	client.stop();

	EXPECT_EQ(true, true);
	EXPECT_EQ(bytes_written, bytes_read);
	EXPECT_EQ(bytes_written, sizeof(TCHAR) * 512);
	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * 512), 0);

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

TEST(CustomAsynchClientTestCase, WriteTest)
{
	const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";

	HANDLE server = CreateNamedPipe(L"\\\\.\\pipe\\mynamedpipe",
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		512 * sizeof(TCHAR),
		512 * sizeof(TCHAR),
		0,
		NULL);

	CustomAsynchClient client(pipe_path);

	client.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	ConnectNamedPipe(server, nullptr);

	DWORD bytes_written = 0;
	const TCHAR write_buffer[sizeof(TCHAR) * 512] = L"Hello world)))";
	client.write(write_buffer, bytes_written, nullptr);

	while (bytes_written != sizeof(TCHAR) * 512)
	{

	}

	std::cout << "HERE" << " " << bytes_written << std::endl;

	TCHAR read_buffer[sizeof(TCHAR) * 512];
	DWORD bytes_read = 0;
	ReadFile(server, read_buffer, sizeof(TCHAR) * 512, &bytes_read, nullptr);

	client.stop();

	EXPECT_EQ(true, true);
	EXPECT_EQ(bytes_written, bytes_read);
	EXPECT_EQ(bytes_written, sizeof(TCHAR) * 512);
	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * 512), 0);

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

int main(int argc, char* argv[])
{
	try
	{
		const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";
		const DWORD bufSize = 512;

		CustomAsynchClient test_client(pipe_path);

		test_client.run();

		DWORD bytes_written = bufSize * sizeof(TCHAR);

		if (test_client.write(L"Hello, world)))", bytes_written, TestToolkit::CopyWriteInfo) == false)
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));

			test_client.write(L"Hello, world)))", bytes_written, TestToolkit::CopyWriteInfo);
		}

		std::this_thread::sleep_for(std::chrono::seconds(3));

		TCHAR* read_buffer = new TCHAR[bufSize];
		DWORD bytes_read = bufSize * sizeof(TCHAR);

		if (test_client.read(read_buffer, bytes_read, TestToolkit::CopyReadInfo) == false)
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));

			test_client.read(read_buffer, bytes_read, TestToolkit::CopyReadInfo);
		}

		test_client.stop();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	system("pause");
	return 0;

	/**
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
	**/
}

void TestToolkit::CopyReadInfo(const TCHAR* l_buffer_read, const DWORD l_bytes_read)
{
	std::wcout << "[SERVER]: " << L"{ TEXT MESSAGE } = " << l_buffer_read;
	std::wcout << " { NUMBER BYTES READ } = " << l_bytes_read << ";";
	std::wcout << std::endl;
}

void TestToolkit::CopyWriteInfo(const TCHAR* l_buffer_write, const DWORD l_bytes_written)
{
	std::wcout << "[SERVICE INFO]: " << L"{ TEXT MESSAGE } = " << l_buffer_write;
	std::wcout << " { NUMBER BYTES WRITTEN } = " << l_bytes_written << ";";
	std::wcout << std::endl;
}