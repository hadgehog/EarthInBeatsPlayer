#include "MFAudioEvents.h"

using namespace winrt::EarthInBeatsEngine::Audio::implementation;

MFAudioEvents::MFAudioEvents()
{

}

MFAudioEvents::~MFAudioEvents()
{

}

void MFAudioEvents::EndOfPlaying(int32_t idx)
{
	if (this->audioPlayer)
	{
		this->audioPlayer->EndOfPlayingTrack(idx);
	}
}

void MFAudioEvents::EndOfRewinding()
{
	if (this->audioPlayer)
	{
		this->audioPlayer->EndOfRewindingTrack();
	}
}

void MFAudioEvents::InitEvent(AudioPlayer* player)
{
	if (player)
	{
		this->audioPlayer = player;
	}
}
