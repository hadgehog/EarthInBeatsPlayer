#pragma once

#include "..\AudioSample\IAudioSample.h"
#include "..\..\AudioEvents\IAudioEvents.h"
#include "..\..\AudioHelpers\Rational.h"

#include <cstdint>
#include <windows.h>
#include <mmreg.h>

class IAudioReader
{
public:
	IAudioReader() = default;
	virtual ~IAudioReader() = default;

	virtual size_t GetAudioStreamCount() = 0;
	virtual uint32_t GetSampleRate(int32_t index) = 0;
	virtual uint32_t GetAudioChannelCount(int32_t index) = 0;
	virtual AudioSampleType GetStreamType(int32_t index) = 0;
	virtual void GetWaveInfo(int32_t index, WAVEFORMATEX* &waveType, uint32_t &waveLength) = 0;
	virtual Int64Rational GetAudioDuration() = 0;
	virtual IAudioSample* ReadAudioSample() = 0;
	virtual void SetPosition(const Int64Rational &position) = 0;
};