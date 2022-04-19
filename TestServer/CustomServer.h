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
#include <mutex>

static const DWORD DEFAULT_CAPACITY = 1;

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

class CustomServer final
{
public:
	CustomServer(const std::wstring& pipe_path, 
				 const DWORD capacity = DEFAULT_CAPACITY);
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


	void run();
	void stop();

	//ALL METHODS BELOW SHOULD BE MUTEX
	bool read(const DWORD index, 
					 void(*read_callback)(std::wstring* dst_buffer, DWORD* dst_bytes_read,
						  const std::wstring& src_buffer, const DWORD src_bytes_read) = nullptr,
					 std::wstring* buffer = nullptr,
					 DWORD* bytes_read = nullptr);

	bool write(const DWORD index, const std::wstring& message,
					  void(*write_callback)(DWORD* bytes_written, 
											const DWORD src_bytes) = nullptr,
					  DWORD* bytes_written = nullptr);
private:
	//ALL METHODS BELOW SHOULD BE MUTEX

	void processLoopV2();

	bool catchEvent(DWORD index);

	void initConnect(const DWORD index);
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

	void(*read_callback)(std::wstring* dst_buffer, DWORD* dst_bytes_read,
						 const std::wstring& src_buffer, const DWORD src_bytes) = nullptr;
	std::wstring* m_read_callback_dst_buffer = nullptr;
	DWORD* m_callback_dst_bytes_read = nullptr;

	void(*write_callback)(DWORD* bytes_written, const DWORD src_bytes) = nullptr;
	//std::wstring* m_write_callback_buffer = nullptr;
	DWORD* m_callback_dst_bytes_written = nullptr;

};