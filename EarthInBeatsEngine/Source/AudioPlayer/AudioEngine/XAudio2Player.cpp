#include "XAudio2Player.h"
#include "..\AudioData\AudioSample\MFAudioSample.h"
#include "..\AudioData\AudioReader\MFAudioReader.h"
#include "..\AudioHelpers\Auto.h"
#include "..\AudioEvents\MFAudioEvents.h"

#include <ppltasks.h>
#include <combaseapi.h>
#include <future>

void SourceVoiceDeleter::operator()(IXAudio2SourceVoice *obj)
{
	obj->DestroyVoice();
}

XAudio2Player::XAudio2Player()
	: currentPosition(0), notifiedRewinding(false), 
	stopped(false), trackIndex(0)
{
}

XAudio2Player::~XAudio2Player()
{

}

void XAudio2Player::Initialize(std::shared_ptr<IAudioReader> iReader, Microsoft::WRL::ComPtr<IXAudio2> iXAudio2, std::shared_ptr<IAudioEvents> iEvents)
{
	this->audioEvents = iEvents;
	this->SetAudioData(iReader, iXAudio2);
}

void XAudio2Player::SetAudioData(std::shared_ptr<IAudioReader> iReader, Microsoft::WRL::ComPtr<IXAudio2> ixAudio2)
{
	HRESULT hr = S_OK;
	WAVEFORMATEX* wF;
	uint32_t wL;
	IXAudio2SourceVoice* tmpVoice = nullptr;

	this->xAudio2 = ixAudio2;
	this->audioReader = iReader;
	this->audioReader->GetWaveInfo(0, wF, wL);
	hr = this->xAudio2->CreateSourceVoice(&tmpVoice, wF, 0, 2.0f, this);
	this->sourceVoice = std::shared_ptr<IXAudio2SourceVoice>(tmpVoice, SourceVoiceDeleter());

	CoTaskMemFree(wF);

	this->stopped = false;
}

void XAudio2Player::Play()
{
	HRESULT hr = S_OK;

	if (this->sourceVoice)
	{
		this->stopped = false;
		this->SubmitBuffer();
		hr = this->sourceVoice->Start();
	}
}

void XAudio2Player::Pause()
{
	HRESULT hr = S_OK;

	if (this->sourceVoice)
	{
		hr = this->sourceVoice->Stop();
		this->audioReader->SetPosition(this->currentPosition);
	}
}

void XAudio2Player::Stop()
{
	HRESULT hr = S_OK;

	if (this->sourceVoice)
	{
		hr = this->sourceVoice->Stop();
		this->FlushSourceVoice();
		this->stopped = true;
	}
}

int64_t XAudio2Player::GetDuration()
{
	int64_t convertValue = 0;
	Int64Rational durationValue = this->audioReader->GetAudioDuration();
	Int64Rational pos(static_cast<Int64Rational::Type>(durationValue.Convert_cr(Int64Rational::Unit::SEC).value), Int64Rational::Unit::SEC);
	convertValue = pos.value;

	return convertValue;
}

int64_t XAudio2Player::GetCurrentPosition()
{
	int64_t physCurrPos = 0;
	Int64Rational tmpPos(this->currentPosition, Int64Rational::Unit::HNS);
	Int64Rational pos(static_cast<Int64Rational::Type>(tmpPos.Convert_cr(Int64Rational::Unit::SEC).value), Int64Rational::Unit::SEC);
	physCurrPos = pos.value;

	return physCurrPos;
}

void XAudio2Player::SetPosition(const Int64Rational& iPosition)
{
	{
		this->FlushSourceVoice();
		this->audioReader->SetPosition(iPosition);

		if (this->audioEvents)
		{
			this->audioEvents->EndOfRewinding();
		}
	}

	this->SubmitBuffer();
}

float XAudio2Player::GetVolume() const
{
	float volume = 0.0f; 
	this->sourceVoice->GetVolume(&volume);
	return volume;
}

void XAudio2Player::SetVolume(float iVolume)
{
	HRESULT hr = S_OK;
	hr = this->sourceVoice->SetVolume(iVolume);
}

void XAudio2Player::GoToNextSong()
{
	this->trackIndex++;
}

void XAudio2Player::SubmitBuffer()
{
	HRESULT hr = S_OK;

	if (this->stopped)
	{
		return;
	}

	if (this->notifiedRewinding == false)
	{
		// TODO: refactor to use MFAudioSample and make_unique
		std::unique_ptr<IAudioSample> sample = std::unique_ptr<IAudioSample>(this->audioReader->ReadAudioSample());

		if (sample)
		{
			sample->GetData([&](void *buffer, uint64_t size)
			{
				XAUDIO2_BUFFER xbuffer = { 0 };
				xbuffer.AudioBytes = (uint32_t)size;
				xbuffer.pAudioData = (BYTE*)buffer;
				xbuffer.pContext = nullptr;

				hr = this->sourceVoice->SubmitSourceBuffer(&xbuffer);
			});


			this->currentPosition = sample->GetSampleTime();
			this->audioSamples.push(std::move(sample));
		}
		else
		{
			if (this->audioEvents)
			{
				this->stopped = true;
				concurrency::create_task([=]()
				{
					this->sourceVoice->Stop();
					this->FlushSourceVoice();
					this->GoToNextSong();
					this->audioEvents->EndOfPlaying(this->trackIndex);
				});
			}
		}
	}
	else
	{
		XAUDIO2_VOICE_STATE voiceState;
		this->sourceVoice->GetState(&voiceState);

		if (voiceState.BuffersQueued == 0)
		{
			this->notifiedRewinding = false;
			this->condVar.notify_all();
		}
	}
}

void XAudio2Player::DeleteSamples()
{
	std::unique_lock<std::mutex> lock(this->samplesMutex);
	this->audioSamples.pop();
}

void XAudio2Player::FlushSourceVoice()
{
	std::unique_lock<std::mutex> lock(this->samplesMutex);

	this->sourceVoice->FlushSourceBuffers();

	XAUDIO2_VOICE_STATE voiceState;
	this->sourceVoice->GetState(&voiceState);

	if (voiceState.BuffersQueued != 0)
	{
		this->notifiedRewinding = true;

		while (this->notifiedRewinding == true)
		{
			this->condVar.wait(lock);
		}
	}

	this->audioSamples = std::queue<std::unique_ptr<IAudioSample>>();
}
