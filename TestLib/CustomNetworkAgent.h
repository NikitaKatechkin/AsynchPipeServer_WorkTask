#pragma once

#include <iostream>
#include <cstdio>
#include <thread>

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include <sstream>
#include <mutex>
#include <memory>

class CustomAsynchNetworkAgent
{
public:
	using ReadCallback = void(*)(const std::wstring& bufferRead, const DWORD bytesRead);
	using WriteCallback = void(*)(const DWORD bytesWritten);

public:
	CustomAsynchNetworkAgent(const std::wstring& pipePath,
							 const DWORD capacity,
							 const DWORD bufsize = 512);
	virtual ~CustomAsynchNetworkAgent();

	void run();
	void stop();

	//ALL METHODS BELOW SHOULD BE MUTEX

	bool read(const DWORD index,
			  std::wstring* buffer = nullptr, //Change type -> TCHAR*
			  DWORD* bytesRead = nullptr,
			  ReadCallback readCallback = nullptr);

	bool write(const DWORD index, 
			   const std::wstring& message, //Change type -> TCHAR*
			   DWORD* bytesWritten = nullptr,
			   WriteCallback writeCallback = nullptr);

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

	void initRead(const DWORD index);

	void initWrite(const DWORD index);

	void OnPended(const DWORD index);
protected:
	//MULTITHREADING PART

	std::thread m_processLoopThread;

	std::mutex m_serviceOperationMutex;
	std::mutex m_printLogMutex;

	//SERVER STATE PART

	DWORD m_capacity = 0;
	std::wstring m_pipePath;

	bool m_isServerRunning = false;

	//PIPE SERVER PART

	std::unique_ptr<Server_State[]> m_state;
	std::unique_ptr<HANDLE[]> m_pipe;
	std::unique_ptr<OVERLAPPED[]> m_overlapped;
	std::unique_ptr<HANDLE[]> m_event;

	//READ || WRITE PART

	DWORD m_bufsize = 0;

	std::unique_ptr<std::unique_ptr<TCHAR[]>[]> m_requestBuffers;
	std::unique_ptr<DWORD[]> m_bytesRead;

	std::unique_ptr<std::unique_ptr<TCHAR[]>[]> m_replyBuffers;
	std::unique_ptr<DWORD[]> m_bytesWritten;

	//CALLBACKS PART

	ReadCallback m_readCallback = nullptr;
	std::wstring* m_readCallbackDstBuffer = nullptr;
	DWORD* m_callbackDstBytesRead = nullptr;

	WriteCallback m_writeCallback = nullptr;
	//std::wstring* m_write_callback_buffer = nullptr;
	DWORD* m_callbackDstBytesWritten = nullptr;

};