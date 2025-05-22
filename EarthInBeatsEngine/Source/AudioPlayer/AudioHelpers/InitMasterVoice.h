#pragma once

#include "..\AudioEngine\XAudio2Player.h"

// TODO: refactor all WRL::ComPtr to winrt::com_ptr
// based on http://stackoverflow.com/questions/1008019/c-singleton-design-pattern
class InitMasterVoice
{
	InitMasterVoice();
	~InitMasterVoice();

	InitMasterVoice(InitMasterVoice const&) = delete;
	void operator=(InitMasterVoice const&) = delete;

public:
	static InitMasterVoice &GetInstance();
	Microsoft::WRL::ComPtr<IXAudio2> GetXAudio();

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice *masterVoice;
};

