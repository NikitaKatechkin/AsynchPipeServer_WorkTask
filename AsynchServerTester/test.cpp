#include "pch.h"
#include <TestServer/CustomServer.h>

namespace TestToolkit
{
	void CopyReadInfo(const TCHAR* l_buffer_read, const DWORD l_bytes_read);

	void CopyWriteInfo(const TCHAR* l_buffer_write, const DWORD l_bytes_written);
}

TEST(CustomAsynchServerTestCase, CreateTest)
{
	const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";

	try
	{
		CustomAsynchServer server(pipe_path, 1);
	}
	catch (const std::exception&)
	{
		FAIL();
	}
}

TEST(CustomAsynchServerTestCase, RunStopTest)
{
	const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";

	CustomAsynchServer server(pipe_path, 1);

	server.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	HANDLE client = CreateFile(L"\\\\.\\pipe\\mynamedpipe",
							   GENERIC_READ | GENERIC_WRITE,
							   FILE_SHARE_READ | FILE_SHARE_WRITE,
							   nullptr,
							   OPEN_EXISTING,
							   NULL,
							   NULL);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	server.stop();

	EXPECT_EQ(true, true);

	EXPECT_NE(client, INVALID_HANDLE_VALUE);
	CloseHandle(client);
}

TEST(CustomAsynchServerTestCase, ReadTest)
{
	const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	CustomAsynchServer server(pipe_path, 1);

	server.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	HANDLE client = CreateFile(L"\\\\.\\pipe\\mynamedpipe",
							   GENERIC_READ | GENERIC_WRITE,
							   FILE_SHARE_READ | FILE_SHARE_WRITE,
							   nullptr,
							   OPEN_EXISTING,
							   NULL,
							   NULL);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	EXPECT_NE(client, INVALID_HANDLE_VALUE);

	DWORD pipe_mode = PIPE_READMODE_MESSAGE;
	EXPECT_NE(SetNamedPipeHandleState(client, &pipe_mode, NULL, NULL), false);

	DWORD bytes_written = 0;
	const TCHAR write_buffer[bufSize] = L"Hello world)))";
	WriteFile(client, write_buffer, bufSize * sizeof(TCHAR), &bytes_written, nullptr);

	EXPECT_EQ(bytes_written, bufSize * sizeof(TCHAR));

	TCHAR* read_buffer = new TCHAR[sizeof(TCHAR) * bufSize];
	DWORD bytes_read = bufSize * sizeof(TCHAR);
	server.read(read_buffer, bytes_read, nullptr);

	while (bytes_read != bytes_written)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	//memcmp(read_buffer, write_buffer.c_str(), sizeof(TCHAR) * 512);

	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * 512), 0);
	EXPECT_EQ(bytes_read, bytes_written);

	server.stop();

	EXPECT_EQ(true, true);

	CloseHandle(client);
}

TEST(CustomAsynchServerTestCase, WriteTest)
{
	const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";

	CustomAsynchServer server(pipe_path, 1);

	server.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	HANDLE client = CreateFile(L"\\\\.\\pipe\\mynamedpipe",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		NULL,
		NULL);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	EXPECT_NE(client, INVALID_HANDLE_VALUE);

	DWORD pipe_mode = PIPE_READMODE_MESSAGE;
	EXPECT_NE(SetNamedPipeHandleState(client, &pipe_mode, NULL, NULL), false);

	const TCHAR write_buffer[sizeof(TCHAR) * 512] = L"Hello world)))";
	DWORD bytes_written = 0;
	server.write(write_buffer, bytes_written, nullptr);

	while (bytes_written != sizeof(TCHAR) * 512)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	EXPECT_EQ(bytes_written, 512 * sizeof(TCHAR));

	TCHAR read_buffer[sizeof(TCHAR) * 512];
	DWORD bytes_read = 0;
	ReadFile(client, read_buffer, 512 * sizeof(TCHAR), &bytes_read, nullptr);

	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * 512), 0);
	EXPECT_EQ(bytes_read, bytes_written);

	server.stop();

	EXPECT_EQ(true, true);

	CloseHandle(client);
}

int main(int argc, char* argv[])
{
	try
	{
		const LPCTSTR pipe_path = L"\\\\.\\pipe\\mynamedpipe";
		const DWORD bufSize = 512;

		CustomAsynchServer test_server(pipe_path, 1);

		TCHAR* read_buffer = new TCHAR[bufSize];
		DWORD bytes_read = bufSize * sizeof(TCHAR);

		test_server.run();

		if (test_server.read(read_buffer, bytes_read, TestToolkit::CopyReadInfo) == false)
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));

			test_server.read(read_buffer, bytes_read, TestToolkit::CopyReadInfo);
		}

		DWORD bytes_written = bufSize * sizeof(TCHAR);

		if (test_server.write(L"Hello, world)))", bytes_written, TestToolkit::CopyWriteInfo) == false)
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));

			test_server.write(L"Hello, world)))", bytes_written, TestToolkit::CopyWriteInfo);
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

	/**
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
	**/
}

void TestToolkit::CopyReadInfo(const TCHAR* l_buffer_read, const DWORD l_bytes_read)
{
	std::wcout << "[CLIENT]: " << L"{ TEXT MESSAGE } = " << l_buffer_read;
	std::wcout << " { NUMBER BYTES READ } = " << l_bytes_read << ";";
	std::wcout << std::endl;
}

void TestToolkit::CopyWriteInfo(const TCHAR* l_buffer_write, const DWORD l_bytes_written)
{
	std::wcout << "[SERVICE INFO]: " << L"{ TEXT MESSAGE } = " << l_buffer_write;
	std::wcout << " { NUMBER BYTES WRITTEN } = " << l_bytes_written << ";";
	std::wcout << std::endl;
}