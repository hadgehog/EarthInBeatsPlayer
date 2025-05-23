#pragma once

#include "IAudioReader.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <vector>
#include <winrt/base.h>
#include <winrt/Windows.Storage.Streams.h>

class MFAudioReader : public IAudioReader
{
public:
	MFAudioReader();
	~MFAudioReader() override;

	size_t GetAudioStreamCount() override;
	uint32_t GetSampleRate(int32_t index) override;
	uint32_t GetAudioChannelCount(int32_t index) override;
	AudioSampleType GetStreamType(int32_t index) override;
	void GetWaveInfo(int32_t audioIdx, WAVEFORMATEX* &waveType, uint32_t& waveLength) override;
	Int64Rational GetAudioDuration() override;
	// TODO: refactor to unique_ptr ?
	IAudioSample* ReadAudioSample() override;
	void SetPosition(const Int64Rational& position) override;

	void Initialize(winrt::Windows::Storage::Streams::IRandomAccessStream stream);

private:
	winrt::com_ptr<IMFSourceReader> audioReader;
	std::vector<int32_t> audioStreamsArray;

private:
	winrt::com_ptr<IMFMediaType> GetType(int32_t index);
	void FindAudioStreamIndexes();
	winrt::com_ptr<IMFSample> ReadSample();
	int64_t FindAudioDuration();
};
