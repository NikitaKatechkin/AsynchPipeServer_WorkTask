#pragma once

#include <TestLib/CustomNetworkAgent.h>

class CustomAsynchServer : public CustomAsynchNetworkAgent
{
public:
	CustomAsynchServer(const std::wstring& pipe_path,
					   const DWORD capacity,
					   const DWORD bufsize = 512);
	virtual ~CustomAsynchServer() = default;

protected:
	virtual void initConnect(const DWORD index) override;
};