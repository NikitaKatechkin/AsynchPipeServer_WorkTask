#include "pch.h"
#include <TestServer/CustomServer.h>

namespace TestToolkit
{
	void CopyReadInfo(const std::wstring& l_buffer_read, const DWORD l_bytes_read);

	void CopyWriteInfo(const DWORD l_bytes_written);
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

	/**
	HANDLE client = CreateFile(L"\\\\.\\pipe\\mynamedpipe",
							   GENERIC_READ | GENERIC_WRITE,
							   FILE_SHARE_READ | FILE_SHARE_WRITE,
							   nullptr,
							   OPEN_EXISTING,
							   NULL,
							   NULL);
	**/
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

TEST(CustomAsynchClientTestCase, ReadTest)
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

	DWORD bytes_written = 0;
	const std::wstring write_buffer = L"Hello world)))";
	WriteFile(client, write_buffer.c_str(), 512 * sizeof(TCHAR), &bytes_written, nullptr);

	EXPECT_EQ(bytes_written, 512 * sizeof(TCHAR));

	std::wstring read_buffer = L"";
	DWORD bytes_read = 0;
	server.read(&read_buffer, &bytes_read, nullptr);

	while (bytes_read != bytes_written)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	EXPECT_EQ(read_buffer, write_buffer);
	EXPECT_EQ(bytes_read, bytes_written);

	server.stop();

	EXPECT_EQ(true, true);

	CloseHandle(client);
}

TEST(CustomAsynchClientTestCase, WriteTest)
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

	std::wstring write_buffer = L"Hello world)))";
	DWORD bytes_written = 0;
	server.write(write_buffer, &bytes_written, nullptr);

	while (bytes_written != sizeof(TCHAR) * 512)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	EXPECT_EQ(bytes_written, 512 * sizeof(TCHAR));

	TCHAR read_buffer[sizeof(TCHAR) * 512];
	DWORD bytes_read = 0;
	ReadFile(client, read_buffer, 512 * sizeof(TCHAR), &bytes_read, nullptr);

	EXPECT_EQ(read_buffer, write_buffer);
	EXPECT_EQ(bytes_read, bytes_written);

	server.stop();

	EXPECT_EQ(true, true);

	CloseHandle(client);
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

void TestToolkit::CopyReadInfo(const std::wstring& l_buffer_read, const DWORD l_bytes_read)
{
	std::wcout << "[SERVER]: " << L"{ TEXT MESSAGE } = " << l_buffer_read;
	std::wcout << " { NUMBER BYTES READ } = " << l_bytes_read << ";";
	std::wcout << std::endl;
}

void TestToolkit::CopyWriteInfo(const DWORD l_bytes_written)
{

	std::wcout << "[SERVICE INFO]: ";
	std::wcout << " { NUMBER BYTES WRITTEN } = " << l_bytes_written << ";";
	std::wcout << std::endl;
}