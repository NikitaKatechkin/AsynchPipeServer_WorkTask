#include "pch.h"
#include <AsynchPipeServer/CustomServer.h>

namespace TestToolkit
{
	void CopyReadInfo(const TCHAR* l_buffer_read, const DWORD l_bytes_read);

	void CopyWriteInfo(const TCHAR* l_buffer_write, const DWORD l_bytes_written);

	void WriteOperationPerformer(
							const TCHAR* write_buffer, 
							DWORD bytes_written, 
							CustomAsynchServer::Callback callback_function);

	void ReadOperationPerformer(
		TCHAR* read_buffer,
		DWORD bytes_read,
		CustomAsynchServer::Callback callback_function, 
		const DWORD index);
}

TEST(CustomAsynchServerTestCase, CreateTest)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";

	try
	{
		CustomAsynchServer server(pipePath, 1);
	}
	catch (const std::exception&)
	{
		FAIL();
	}
}

TEST(CustomAsynchServerTestCase, RunStopTest)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";

	CustomAsynchServer server(pipePath, 1);

	server.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	HANDLE client = CreateFile(pipePath,
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
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	CustomAsynchServer server(pipePath, 1);

	server.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	HANDLE client = CreateFile(pipePath,
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
	EXPECT_EQ(
		WriteFile(client, write_buffer, bufSize * sizeof(TCHAR), &bytes_written, nullptr), TRUE);

	EXPECT_EQ(bytes_written, bufSize * sizeof(TCHAR));

	TCHAR* read_buffer = new TCHAR[bufSize];
	DWORD bytes_read = bufSize * sizeof(TCHAR);
	server.read(read_buffer, bytes_read, nullptr);

	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	TestToolkit::CopyReadInfo(read_buffer, bytes_read);
	TestToolkit::CopyReadInfo(write_buffer, bytes_written);

	//memcmp(read_buffer, write_buffer.c_str(), sizeof(TCHAR) * 512);

	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * bufSize), 0);
	EXPECT_EQ(bytes_read, bytes_written);

	server.stop();

	EXPECT_EQ(true, true);

	CloseHandle(client);
}

TEST(CustomAsynchServerTestCase, WriteTest)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	CustomAsynchServer server(pipePath, 1);

	server.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	HANDLE client = CreateFile(pipePath,
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

	const TCHAR write_buffer[bufSize] = L"Hello world)))";
	DWORD bytes_written = bufSize * sizeof(TCHAR);
	server.write(write_buffer, bytes_written, nullptr);

	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	//EXPECT_EQ(bytes_written, bufSize * sizeof(TCHAR));

	TCHAR read_buffer[bufSize];
	DWORD bytes_read = 0;
	EXPECT_EQ(ReadFile(client, read_buffer, bufSize * sizeof(TCHAR), &bytes_read, nullptr), TRUE);

	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * bufSize), 0);
	EXPECT_EQ(bytes_read, bytes_written);

	server.stop();

	EXPECT_EQ(true, true);

	CloseHandle(client);
}

TEST(CustomAsynchServerTestCase, NullMessageTransferWriteTest)
{
	TestToolkit::WriteOperationPerformer(nullptr, 1024, nullptr);
}

int main(int argc, char* argv[])
{
	/**
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
	**/

	
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
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

void TestToolkit::WriteOperationPerformer(const TCHAR* write_buffer, 
	DWORD bytes_written, 
	CustomAsynchServer::Callback callback_function)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	CustomAsynchServer server(pipePath, 1);

	server.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	HANDLE client = CreateFile(pipePath,
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

	//const TCHAR write_buffer[bufSize] = L"Hello world)))";
	//DWORD bytes_written = bufSize * sizeof(TCHAR);
	server.write(write_buffer, bytes_written, callback_function);


	std::this_thread::sleep_for(std::chrono::seconds(1));
	//EXPECT_EQ(bytes_written, bufSize * sizeof(TCHAR));

	TCHAR read_buffer[bufSize];
	DWORD bytes_read = 0;
	EXPECT_EQ(ReadFile(client, read_buffer, bufSize * sizeof(TCHAR), &bytes_read, nullptr), TRUE);

	if (write_buffer != nullptr)
	{
		EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * bufSize), 0);
	}
	EXPECT_EQ(bytes_read, bytes_written);

	server.stop();

	EXPECT_EQ(true, true);

	CloseHandle(client);
}

void TestToolkit::ReadOperationPerformer(TCHAR* read_buffer, 
	DWORD bytes_read, 
	CustomAsynchServer::Callback callback_function, 
	const DWORD index)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	CustomAsynchServer server(pipePath, 1);

	server.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	HANDLE client = CreateFile(pipePath,
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
	EXPECT_EQ(
		WriteFile(client, write_buffer, bufSize * sizeof(TCHAR), &bytes_written, nullptr), TRUE);

	EXPECT_EQ(bytes_written, bufSize * sizeof(TCHAR));

	//TCHAR* read_buffer = new TCHAR[bufSize];
	//DWORD bytes_read = bufSize * sizeof(TCHAR);
	server.read(read_buffer, bytes_read, callback_function, index);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	TestToolkit::CopyReadInfo(read_buffer, bytes_read);
	TestToolkit::CopyReadInfo(write_buffer, bytes_written);

	//memcmp(read_buffer, write_buffer.c_str(), sizeof(TCHAR) * 512);

	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * bufSize), 0);
	EXPECT_EQ(bytes_read, bytes_written);

	server.stop();

	EXPECT_EQ(true, true);

	CloseHandle(client);
}
