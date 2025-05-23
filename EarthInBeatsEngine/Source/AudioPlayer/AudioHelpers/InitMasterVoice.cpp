#include "InitMasterVoice.h"

#include <mfidl.h>
#include <mfapi.h>
#include <mfobjects.h>

InitMasterVoice::InitMasterVoice()
{
	HRESULT hr = S_OK;
	hr = XAudio2Create(this->xAudio2.put(), 0, XAUDIO2_DEFAULT_PROCESSOR);
	hr = this->xAudio2->CreateMasteringVoice(&this->masterVoice, XAUDIO2_DEFAULT_CHANNELS, 0, 0, nullptr, nullptr, AudioCategory_Media);
}

InitMasterVoice::~InitMasterVoice()
{
	this->masterVoice->DestroyVoice();
}

InitMasterVoice &InitMasterVoice::GetInstance()
{
	static InitMasterVoice instance;
	return instance;
}

winrt::com_ptr<IXAudio2> InitMasterVoice::GetXAudio()
{
	return this->xAudio2;
}
