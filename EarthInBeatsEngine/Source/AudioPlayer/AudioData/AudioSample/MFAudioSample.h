#pragma once

#include "IAudioSample.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <wrl.h>

class MFAudioSample : public IAudioSample
{
public:
	MFAudioSample();
	~MFAudioSample() override;

	int64_t GetDuration() override;
	int64_t GetSampleTime() override;

	void Initialize(Microsoft::WRL::ComPtr<IMFSample> sample);

protected:
	void Lock(void** buffer, uint64_t* size) override;
	void Unlock(void* buffer, uint64_t size) override;

private:
	int64_t sampleTime;
	int64_t sampleDuration;
	Microsoft::WRL::ComPtr<IMFMediaBuffer> sampleBuffer;
};
