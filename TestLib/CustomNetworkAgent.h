#pragma once 

#include <iostream>
#include <cstdio>
#include <thread>

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define DEFAULT_BUFSIZE 512

class CustomNetworkAgent
{
public:
	CustomNetworkAgent(const std::wstring& l_pipe_name);
	virtual ~CustomNetworkAgent() = default;

	virtual bool Connect() = 0;
	virtual bool Disconnect() = 0;

	bool Send(const std::wstring& message, const DWORD& l_buffer_size = DEFAULT_BUFSIZE);
	bool Recieve(std::wstring& l_buffer, const DWORD& l_buffer_size = DEFAULT_BUFSIZE);
protected:
	HANDLE m_pipe = INVALID_HANDLE_VALUE;
	std::wstring m_pipe_name;

	bool m_is_connected = false;
};