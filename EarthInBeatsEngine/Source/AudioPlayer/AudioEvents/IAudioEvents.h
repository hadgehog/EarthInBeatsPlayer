#pragma once

#include <cstdint>
//#include "..\AudioPlayer.h"

class IAudioEvents
{
public:
	IAudioEvents() = default;
	virtual ~IAudioEvents() = default;

	//virtual void InitEvent() = 0;

	virtual void EndOfRewinding() = 0;
	virtual void EndOfPlaying(int32_t idx) = 0;
};

