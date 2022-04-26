#pragma once

#include <iostream>
#include <cstdio>
#include <thread>

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include <sstream>
#include <mutex>

class CustomAsynchNetworkAgent
{
public:
	using ReadCallback = void(*)(const std::wstring& buffer_read, const DWORD bytes_read);
	using WriteCallback = void(*)(const DWORD bytes_written);

public:
	CustomAsynchNetworkAgent(const std::wstring& pipe_path,
							 const DWORD capacity,
							 const DWORD bufsize = 512);
	virtual ~CustomAsynchNetworkAgent();

	void run();
	void stop();

	//ALL METHODS BELOW SHOULD BE MUTEX

	bool read(const DWORD index,
			  std::wstring* buffer = nullptr, //Change type -> TCHAR*
			  DWORD* bytes_read = nullptr,
			  ReadCallback read_callback = nullptr);

	bool write(const DWORD index, 
			   const std::wstring& message, //Change type -> TCHAR*
			   DWORD* bytes_written = nullptr,
			   WriteCallback write_callback = nullptr);

protected:
	enum class Server_State
	{
		Non_Initialized,
		Disconnected,
		Connection_Pended,
		Connected,
		Reading_Pended,
		Reading_Signaled,
		Writing_Pended,
		Writing_Signaled
	};

protected:
	//ALL METHODS BELOW SHOULD BE MUTEX

	void processLoop();

	bool waitForEvent(DWORD index);

	virtual void initConnect(const DWORD index) = 0;
	void OnConnect(const DWORD index);

	void initRead(const DWORD index);
	void OnRead(const DWORD index);

	void initWrite(const DWORD index);
	void OnWrite(const DWORD index);

protected:
	//MULTITHREADING PART

	std::thread m_process_loop_th;
	std::mutex m_mutex;

	//SERVER STATE PART

	DWORD m_capacity = 0;
	std::wstring m_pipe_path;

	bool m_is_server_running = false;

	//PIPE SERVER PART

	Server_State* m_state = nullptr;
	HANDLE* m_pipe = nullptr;
	OVERLAPPED* m_overlapped = NULL;
	HANDLE* m_event = nullptr;

	//READ || WRITE PART

	DWORD m_bufsize_tchar = 0;

	TCHAR** m_request_buffers = nullptr;
	DWORD* m_bytes_read = nullptr;

	TCHAR** m_reply_buffers = nullptr;
	DWORD* m_bytes_written = nullptr;

	//CALLBACKS PART

	ReadCallback m_read_callback = nullptr;
	std::wstring* m_read_callback_dst_buffer = nullptr;
	DWORD* m_callback_dst_bytes_read = nullptr;

	WriteCallback m_write_callback = nullptr;
	//std::wstring* m_write_callback_buffer = nullptr;
	DWORD* m_callback_dst_bytes_written = nullptr;

};