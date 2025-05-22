#pragma once

#include "IAudioEvents.h"
#include "..\AudioPlayer.h"

using namespace winrt::EarthInBeatsEngine::Audio::implementation;

class MFAudioEvents : public IAudioEvents
{
public:
	MFAudioEvents();
	~MFAudioEvents() override;

	void EndOfPlaying(int32_t idx) override;
	void EndOfRewinding() override;

	void InitEvent(AudioPlayer* player);

private:
	AudioPlayer* audioPlayer = nullptr;
};

