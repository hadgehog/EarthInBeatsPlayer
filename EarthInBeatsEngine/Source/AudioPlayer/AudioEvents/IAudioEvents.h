#pragma once

#include <cstdint>

class IAudioEvents
{
public:
	IAudioEvents() = default;
	virtual ~IAudioEvents() = default;

	virtual void EndOfRewinding() = 0;
	virtual void EndOfPlaying(int32_t idx) = 0;
};

