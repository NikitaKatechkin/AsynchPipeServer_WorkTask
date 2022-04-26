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

class CustomAsynchServer : public CustomAsynchNetworkAgent
{
public:
	CustomAsynchServer(const std::wstring& pipe_path,
		const DWORD capacity);
	~CustomAsynchServer();
protected:
	//void ConstructConnect(HANDLE& l_pipe);
};