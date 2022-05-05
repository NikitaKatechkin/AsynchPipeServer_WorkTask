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
	using Callback = void(*)(const TCHAR* buffer, const DWORD bytesProcessed);

public:
	CustomAsynchNetworkAgent(const std::wstring& pipePath,
		const DWORD capacity,
		const DWORD bufsize = 512);
	virtual ~CustomAsynchNetworkAgent();

	void run();
	void stop();

	//ALL METHODS BELOW SHOULD BE MUTEX

	bool read(TCHAR* buffer, //Change type -> TCHAR*
			  DWORD bytesRead,
			  Callback readCallback = nullptr, 
			  const DWORD index = 0);

	bool write(const TCHAR* message, //Change type -> TCHAR*
			   DWORD bytesWritten,
			   Callback writeCallback = nullptr, 
			   const DWORD index = 0);

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

	bool waitForEvent(DWORD& index);

	virtual void initConnect(const DWORD index = 0) = 0;

	void initRead(const DWORD index = 0);

	void initWrite(const DWORD index = 0);

	//void OnPended(const DWORD index = 0);

	bool OnPended(DWORD* bytes_processed, const DWORD index = 0);

	void OnConnect(const DWORD index = 0);

	void OnRead(const DWORD index = 0);

	void OnWrite(const DWORD index = 0);

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

	Callback m_readCallback = nullptr;
	TCHAR* m_readCallbackDstBuffer = nullptr;

	Callback m_writeCallback = nullptr;

};