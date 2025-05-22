#pragma once

#include "IAudioReader.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>

#include <wrl.h>

#include <vector>

// TODO: refactor all WRL::ComPtr to winrt::com_ptr
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

	// TODO: refactor to winrt::
	void Initialize(Windows::Storage::Streams::IRandomAccessStream^ stream);

private:
	Microsoft::WRL::ComPtr<IMFSourceReader> audioReader;
	std::vector<int32_t> audioStreamsArray;

private:
	Microsoft::WRL::ComPtr<IMFMediaType> GetType(int32_t index);
	void FindAudioStreamIndexes();
	Microsoft::WRL::ComPtr<IMFSample> ReadSample();
	int64_t FindAudioDuration();
};
