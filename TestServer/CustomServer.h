#pragma once

/**
* 
* WORKING_VERSION
* 
#include <TestLib/CustomNetworkAgent.h>

#include <sstream>

enum class SERVER_STATE
{
	NON_INITIALIZED,
	DISCONNECTED,
	CONNECTION_IN_PROGRESS,
	CONNECTED,
	READING_IN_PROGRESS,
	READING_SIGNALED,
	WRITING_IN_PROGRESS,
	WRITING_SIGNALED,
};

class CustomServer final
{
public:
	CustomServer(const std::wstring& l_pipe_name);
	~CustomServer();

	void MainLoopVer2();
private:
	std::wstring m_pipe_name;

	SERVER_STATE m_state = SERVER_STATE::NON_INITIALIZED;

	HANDLE m_pipe = INVALID_HANDLE_VALUE;
	HANDLE m_event = nullptr;
	OVERLAPPED m_overlap;

	TCHAR* m_requests_buffer;
	DWORD m_bytes_read = 0;

	TCHAR* m_replies_buffer;
	DWORD m_bytes_write = 0;

	bool m_is_pended = false;

	bool CatchEvent(DWORD& l_index);

	BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo);

	void DisconnectAndReconnect();

	void GetAnswerToRequest();
};
**/

#include <TestLib/CustomNetworkAgent.h>

#include <sstream>

static const DWORD DEFAULT_CAPACITY = 1;

enum class SERVER_STATE
{
	NON_INITIALIZED,
	DISCONNECTED,
	CONNECTION_PENDED,
	CONNECTED,
	READING_PENDED,
	READING_SIGNALED,
	WRITING_PENDED,
	WRITING_SIGNALED
};

class CustomServer final
{
public:
	CustomServer(const std::wstring& l_pipe_path, 
				 const DWORD l_capacity = DEFAULT_CAPACITY);
	~CustomServer();

	//DEPRICATED PART STARTS
	/**
	void Connect(const DWORD& l_index);
	bool Connect(const DWORD& l_index);
	bool Disconnect(const DWORD& l_index);

	bool Recieve(const DWORD& l_index, std::wstring& l_buffer);
	bool Send(const DWORD& l_index, const std::wstring& l_buffer);

	void ProcessLoop();
	**/
	//DEPRICATED PART ENDS

	void processLoopV2();

	void run();
	void stop();

	//To make bool
	void adoptedRead(const DWORD l_index);
	void adoptedWrite(const DWORD l_index, const std::wstring& l_message);
private:
	bool catchEvent(DWORD l_index);

	void initConnect(const DWORD l_index);
	void pendedConnect(const DWORD l_index);

	void initRead(const DWORD l_index);
	void pendedRead(const DWORD l_index);

	void initWrite(const DWORD l_index);
	void pendedWrite(const DWORD l_index);

	std::thread* m_process_loop_th = nullptr;

	DWORD m_capacity = 0;
	std::wstring m_pipe_path;

	bool m_is_server_running = false;

	SERVER_STATE* m_state = nullptr; // = SERVER_STATE::NON_INITIALIZED;
	HANDLE* m_pipe = nullptr;
	OVERLAPPED* m_overlapped = NULL;
	HANDLE* m_event = nullptr;


	TCHAR** m_request_buffers = nullptr;
	DWORD* m_bytes_read = nullptr;

	TCHAR** m_reply_buffers = nullptr;
	DWORD* m_bytes_written = nullptr;
};