/**
---------WORKING VERSION-----------
#pragma once 

#include <iostream>
#include <cstdio>
#include <thread>

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

const DWORD DEFAULT_BUFSIZE = 512;
const LPCTSTR DEFAULT_PIPE_NAME = TEXT("\\\\.\\pipe\\mynamedpipe");

class CustomClient final
{
public:
	CustomClient(const LPCTSTR l_pipe_name = nullptr);
	~CustomClient();

	void WaitConnection();

	std::wstring Recieve();
	void Send(const std::wstring& message);
private:
	HANDLE m_pipe = INVALID_HANDLE_VALUE;
	LPCTSTR m_pipe_name = nullptr;

	bool m_is_connected = false;
};
**/

#pragma once

#include <TestLib/CustomNetworkAgent.h>

class CustomAsynchClient : public CustomAsynchNetworkAgent
{
public:
	CustomAsynchClient(const std::wstring& pipe_path,
		const DWORD capacity = 1);
	~CustomAsynchClient();
protected:
	void initConnect(const DWORD index) override;
};