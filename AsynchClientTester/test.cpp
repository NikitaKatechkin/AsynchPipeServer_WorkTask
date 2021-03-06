#include "pch.h"
#include <AsynchPipeClient/CustomClient.h>

namespace TestToolkit
{
	void CopyReadInfo(const TCHAR* l_buffer_read, const DWORD l_bytes_read);

	void CopyWriteInfo(const TCHAR* l_buffer_write, const DWORD l_bytes_written);

	void WriteOperationPerformer(
		const TCHAR* write_buffer,
		DWORD bytes_written,
		CustomAsynchClient::Callback callback_function);

	void ReadOperationPerformer(
		TCHAR* read_buffer,
		DWORD bytes_read,
		CustomAsynchClient::Callback callback_function);

	namespace MockedFunctionality
	{
		struct FlagBundle
		{
			bool m_wasFuctionCalled = false; //g_ as global 

			TCHAR* m_bufferAddress = nullptr;
			DWORD m_bufferSize = 0;

			void Reset()
			{
				bool m_wasFuctionCalled = false; //g_ as global 

				TCHAR* m_bufferAddress = nullptr;
				DWORD m_bufferSize = 0;
			}
		};

		FlagBundle g_allFlags;

		void FlaggedReadCallback(const TCHAR* l_buffer_read, const DWORD l_bytes_read);
		void FlaggedWriteCallback(const TCHAR* l_buffer_write, const DWORD l_bytes_written);
	}
}

TEST(CustomAsynchClientTestCase, CreateTest) 
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	HANDLE server = CreateNamedPipe(pipePath,
									PIPE_ACCESS_DUPLEX,
									PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
									PIPE_UNLIMITED_INSTANCES,
									bufSize * sizeof(TCHAR),
									bufSize * sizeof(TCHAR),
									0,
									NULL);

	CustomAsynchClient client(pipePath);

	ConnectNamedPipe(server, nullptr);

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

TEST(CustomAsynchClientTestCase, RunStopTest)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	HANDLE server = CreateNamedPipe(L"\\\\.\\pipe\\mynamedpipe",
									PIPE_ACCESS_DUPLEX,
									PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
									PIPE_UNLIMITED_INSTANCES,
									bufSize * sizeof(TCHAR),
									bufSize * sizeof(TCHAR),
									0,
									NULL);

	CustomAsynchClient client(pipePath);

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
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	HANDLE server = CreateNamedPipe(pipePath,
									PIPE_ACCESS_DUPLEX,
									PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
									PIPE_UNLIMITED_INSTANCES,
									bufSize * sizeof(TCHAR),
									bufSize * sizeof(TCHAR),
									0,
									NULL);

	CustomAsynchClient client(pipePath);

	client.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	ConnectNamedPipe(server, nullptr);

	DWORD bytes_written = 0;
	const TCHAR write_buffer[bufSize] = L"Hello world)))";
	WriteFile(server, write_buffer, bufSize * sizeof(TCHAR), &bytes_written, nullptr);

	TCHAR* read_buffer = new TCHAR[bufSize];
	DWORD bytes_read = bufSize * sizeof(TCHAR);
	client.read(read_buffer, bytes_read, nullptr);
	
	while (bytes_read != bytes_written)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	client.stop();

	EXPECT_EQ(true, true);
	EXPECT_EQ(bytes_written, bytes_read);
	EXPECT_EQ(bytes_written, sizeof(TCHAR) * bufSize);
	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * bufSize), 0);

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

TEST(CustomAsynchClientTestCase, WriteTest)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	HANDLE server = CreateNamedPipe(L"\\\\.\\pipe\\mynamedpipe",
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		bufSize * sizeof(TCHAR),
		bufSize * sizeof(TCHAR),
		0,
		NULL);

	CustomAsynchClient client(pipePath);

	client.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	ConnectNamedPipe(server, nullptr);

	DWORD bytes_written = bufSize * sizeof(TCHAR);
	const TCHAR write_buffer[bufSize] = L"Hello world)))";
	client.write(write_buffer, bytes_written, nullptr);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	TCHAR read_buffer[bufSize];
	DWORD bytes_read = 0;
	EXPECT_EQ(ReadFile(server, read_buffer, sizeof(TCHAR) * 512, &bytes_read, nullptr), true);

	client.stop();

	EXPECT_EQ(true, true);
	EXPECT_EQ(bytes_written, bytes_read);
	EXPECT_EQ(bytes_written, sizeof(TCHAR) * bufSize);
	EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * bufSize), 0);

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

TEST(CustomAsynchClientTestCase, NullMessageTransferWriteTest)
{
	TestToolkit::WriteOperationPerformer(nullptr, 1024, nullptr);
}

TEST(CustomAsynchClientTestCase, NonNullCallbackMessageTransferWriteTest)
{
	const DWORD bufSize = 512;
	const TCHAR* write_buffer = new TCHAR[bufSize]{ L"Hello world)))" };

	TestToolkit::WriteOperationPerformer(write_buffer,
		sizeof(TCHAR) * bufSize,
		TestToolkit::CopyWriteInfo);
}

TEST(CustomAsynchClientTestCase, NullMessageRecieveReadTest)
{
	const DWORD bufSize = 512;

	TestToolkit::ReadOperationPerformer(nullptr,
		bufSize * sizeof(TCHAR),
		nullptr);
}

TEST(CustomAsynchClientTestCase, NonNullCallbackRecieveReadTest)
{
	const DWORD bufSize = 512;
	TCHAR* read_buffer = new TCHAR[bufSize];

	TestToolkit::ReadOperationPerformer(read_buffer,
		bufSize * sizeof(TCHAR),
		TestToolkit::CopyReadInfo);
}

TEST(CustomAsynchClientTestCase, NonNullCallbackRecieveReadTestWithFlags)
{
	const DWORD bufSize = 512;
	TCHAR* read_buffer = new TCHAR[bufSize];

	TestToolkit::MockedFunctionality::g_allFlags.Reset();

	TestToolkit::ReadOperationPerformer(read_buffer,
		bufSize * sizeof(TCHAR),
		TestToolkit::MockedFunctionality::FlaggedWriteCallback);

	EXPECT_EQ(read_buffer, TestToolkit::MockedFunctionality::g_allFlags.m_bufferAddress);
	EXPECT_EQ(bufSize * sizeof(TCHAR), TestToolkit::MockedFunctionality::g_allFlags.m_bufferSize);
	EXPECT_EQ(true, TestToolkit::MockedFunctionality::g_allFlags.m_wasFuctionCalled);
}

TEST(CustomAsynchClientTestCase, NonNullCallbackTransferWriteTestWithFlags)
{
	const DWORD bufSize = 512;
	const TCHAR* write_buffer = new TCHAR[bufSize]{ L"Hello world)))" };

	TestToolkit::MockedFunctionality::g_allFlags.Reset();

	TestToolkit::WriteOperationPerformer(write_buffer,
		sizeof(TCHAR) * bufSize,
		TestToolkit::MockedFunctionality::FlaggedWriteCallback);

	EXPECT_NE((write_buffer),
		TestToolkit::MockedFunctionality::g_allFlags.m_bufferAddress);
	EXPECT_EQ(bufSize * sizeof(TCHAR), TestToolkit::MockedFunctionality::g_allFlags.m_bufferSize);
	EXPECT_EQ(true, TestToolkit::MockedFunctionality::g_allFlags.m_wasFuctionCalled);
}


int main(int argc, char* argv[])
{
	/**
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
	**/

	
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
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

void TestToolkit::WriteOperationPerformer(const TCHAR* write_buffer,
										  DWORD bytes_written,
										  CustomAsynchClient::Callback callback_function)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	HANDLE server = CreateNamedPipe(L"\\\\.\\pipe\\mynamedpipe",
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		bufSize * sizeof(TCHAR),
		bufSize * sizeof(TCHAR),
		0,
		NULL);

	CustomAsynchClient client(pipePath);

	client.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	ConnectNamedPipe(server, nullptr);

	//DWORD bytes_written = bufSize * sizeof(TCHAR);
	//const TCHAR write_buffer[bufSize] = L"Hello world)))";
	client.write(write_buffer, bytes_written, callback_function);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	TCHAR read_buffer[bufSize];
	DWORD bytes_read = 0;
	EXPECT_EQ(ReadFile(server, read_buffer, sizeof(TCHAR) * 512, &bytes_read, nullptr), true);

	client.stop();

	EXPECT_EQ(true, true);
	EXPECT_EQ(bytes_written, bytes_read);
	EXPECT_EQ(bytes_written, sizeof(TCHAR) * bufSize);
	if (write_buffer != nullptr)
	{
		EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * bufSize), 0);
	}

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

void TestToolkit::ReadOperationPerformer(TCHAR* read_buffer,
	DWORD bytes_read,
	CustomAsynchClient::Callback callback_function)
{
	const LPCTSTR pipePath = L"\\\\.\\pipe\\mynamedpipe";
	const DWORD bufSize = 512;

	HANDLE server = CreateNamedPipe(pipePath,
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		bufSize * sizeof(TCHAR),
		bufSize * sizeof(TCHAR),
		0,
		NULL);

	CustomAsynchClient client(pipePath);

	client.run();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	ConnectNamedPipe(server, nullptr);

	DWORD bytes_written = 0;
	const TCHAR write_buffer[bufSize] = L"Hello world)))";
	WriteFile(server, write_buffer, bufSize * sizeof(TCHAR), &bytes_written, nullptr);

	//TCHAR* read_buffer = new TCHAR[bufSize];
	//DWORD bytes_read = bufSize * sizeof(TCHAR);
	client.read(read_buffer, bytes_read, callback_function);

	while (bytes_read != bytes_written)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	client.stop();

	EXPECT_EQ(true, true);
	EXPECT_EQ(bytes_written, bytes_read);
	EXPECT_EQ(bytes_written, sizeof(TCHAR) * bufSize);
	if (read_buffer != nullptr)
	{
		EXPECT_EQ(memcmp(read_buffer, write_buffer, sizeof(TCHAR) * bufSize), 0);
	}

	ConnectNamedPipe(server, nullptr);
	EXPECT_EQ(GetLastError(), ERROR_PIPE_CONNECTED);

	DisconnectNamedPipe(server);
	CloseHandle(server);
}

void TestToolkit::MockedFunctionality::FlaggedReadCallback(const TCHAR* l_buffer_read,
	const DWORD l_bytes_read)
{
	g_allFlags.m_wasFuctionCalled = true;

	g_allFlags.m_bufferAddress = const_cast<TCHAR*>(l_buffer_read);
	g_allFlags.m_bufferSize = l_bytes_read;
}
void TestToolkit::MockedFunctionality::FlaggedWriteCallback(const TCHAR* l_buffer_write,
	const DWORD l_bytes_written)
{
	g_allFlags.m_wasFuctionCalled = true;

	g_allFlags.m_bufferAddress = const_cast<TCHAR*>(l_buffer_write);
	g_allFlags.m_bufferSize = l_bytes_written;
}