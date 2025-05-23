#pragma once

#include "..\AudioEngine\XAudio2Player.h"

// based on http://stackoverflow.com/questions/1008019/c-singleton-design-pattern
class InitMasterVoice
{
	InitMasterVoice();
	~InitMasterVoice();

	InitMasterVoice(InitMasterVoice const&) = delete;
	void operator=(InitMasterVoice const&) = delete;

public:
	static InitMasterVoice &GetInstance();
	winrt::com_ptr<IXAudio2> GetXAudio();

private:
	winrt::com_ptr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;
};

