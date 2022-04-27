#pragma once

#include <TestLib/CustomNetworkAgent.h>

class CustomAsynchClient : public CustomAsynchNetworkAgent
{
public:
	CustomAsynchClient(const std::wstring& pipe_path, 
					   const DWORD bufsize = 512);
	virtual ~CustomAsynchClient() = default;

protected:
	virtual void initConnect(const DWORD index = 0) override;
};