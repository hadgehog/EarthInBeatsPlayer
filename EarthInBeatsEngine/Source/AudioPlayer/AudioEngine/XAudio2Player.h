#pragma once

#include "..\AudioData\AudioReader\IAudioReader.h"

#include <wrl.h>
#include <xaudio2.h>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>

struct SourceVoiceDeleter
{
	void operator()(IXAudio2SourceVoice* obj);
};

// TODO: refactor all WRL::ComPtr to winrt::com_ptr
class XAudio2Player : public IXAudio2VoiceCallback
{
public:
	XAudio2Player();
	~XAudio2Player();

	void Initialize(std::shared_ptr<IAudioReader> iReader, Microsoft::WRL::ComPtr<IXAudio2> ixAudio2, std::shared_ptr<IAudioEvents> iEvents);
	void SetAudioData(std::shared_ptr<IAudioReader> iReader, Microsoft::WRL::ComPtr<IXAudio2> ixAudio2);

	void Play();
	void Pause();
	void Stop();
	int64_t GetDuration();
	int64_t GetCurrentPosition();
	void SetPosition(const Int64Rational& iPosition);
	float GetVolume() const;
	void SetVolume(float iVolume);
	void GoToNextSong();

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	std::shared_ptr<IXAudio2SourceVoice> sourceVoice;
	std::shared_ptr<IAudioReader> audioReader;
	int64_t currentPosition;
	bool notifiedRewinding;
	std::condition_variable condVar;
	std::shared_ptr<IAudioEvents> audioEvents;
	std::mutex samplesMutex;
	std::queue<std::unique_ptr<IAudioSample>> audioSamples;
	bool stopped;
	int32_t trackIndex = 0;

private:
	void SubmitBuffer();
	void DeleteSamples();
	void FlushSourceVoice();

	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 bytesRequired) override {}
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd(){}
	virtual void STDMETHODCALLTYPE OnStreamEnd(){}
	virtual void STDMETHODCALLTYPE OnBufferStart(void* pContext){}
	virtual void STDMETHODCALLTYPE OnBufferEnd(void* pContext) override;
	virtual void STDMETHODCALLTYPE OnLoopEnd(void*){}
	virtual void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT){}
};

