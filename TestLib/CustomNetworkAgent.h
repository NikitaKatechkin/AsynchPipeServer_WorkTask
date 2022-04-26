#pragma once

#include <iostream>
#include <cstdio>
#include <thread>

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define DEFAULT_BUFSIZE 512

#include <sstream>
#include <mutex>

//static const DWORD DEFAULT_CAPACITY = 1;

typedef void(*READ_CALLBACK)(std::wstring* dst_buffer, DWORD* dst_bytes_read,
	const std::wstring& src_buffer, const DWORD src_bytes);
typedef void(*WRITE_CALLBACK)(DWORD* bytes_written, const DWORD src_bytes);

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

class CustomAsynchNetworkAgent
{
public:
	CustomAsynchNetworkAgent(const std::wstring& pipe_path,
		const DWORD capacity);
	~CustomAsynchNetworkAgent();

	void run();
	void stop();

	//ALL METHODS BELOW SHOULD BE MUTEX
	/**
	bool read(const DWORD index,
		void(*read_callback)(std::wstring* dst_buffer, DWORD* dst_bytes_read,
			const std::wstring& src_buffer, const DWORD src_bytes_read) = nullptr,
		std::wstring* buffer = nullptr,
		DWORD* bytes_read = nullptr);
	**/

	bool read(const DWORD index,
		READ_CALLBACK read_callback = nullptr,
		std::wstring* buffer = nullptr,
		DWORD* bytes_read = nullptr);

	/**
	bool write(const DWORD index, const std::wstring& message,
		void(*write_callback)(DWORD* bytes_written,
			const DWORD src_bytes) = nullptr,
		DWORD* bytes_written = nullptr);
	**/

	bool write(const DWORD index, const std::wstring& message,
		WRITE_CALLBACK write_callback = nullptr,
		DWORD* bytes_written = nullptr);
protected:
	//EXPERIMENTAL PART

	/**
	virtual void ConstructConnect(HANDLE& pipe) = 0;
	**/

	//ALL METHODS BELOW SHOULD BE MUTEX

	void processLoop();

	bool catchEvent(DWORD index);

	virtual void initConnect(const DWORD index);
	//void initConnect(const DWORD index);
	void pendedConnect(const DWORD index);

	void initRead(const DWORD index);
	void pendedRead(const DWORD index);

	void initWrite(const DWORD index);
	void pendedWrite(const DWORD index);

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

	TCHAR** m_request_buffers = nullptr;
	DWORD* m_bytes_read = nullptr;

	TCHAR** m_reply_buffers = nullptr;
	DWORD* m_bytes_written = nullptr;

	//CALLBACKS PART

	//void(*read_callback)(std::wstring* dst_buffer, DWORD* dst_bytes_read,
	//	const std::wstring& src_buffer, const DWORD src_bytes) = nullptr;
	READ_CALLBACK m_read_callback = nullptr;
	std::wstring* m_read_callback_dst_buffer = nullptr;
	DWORD* m_callback_dst_bytes_read = nullptr;

	//void(*write_callback)(DWORD* bytes_written, const DWORD src_bytes) = nullptr;
	WRITE_CALLBACK m_write_callback = nullptr;
	//std::wstring* m_write_callback_buffer = nullptr;
	DWORD* m_callback_dst_bytes_written = nullptr;

};