#include "MFAudioReader.h"
#include "..\..\AudioHelpers\Auto.h"
#include "..\AudioSample\MFAudioSample.h"
#include "..\..\AudioEvents\MFAudioEvents.h"

MFAudioReader::MFAudioReader()
{

}

MFAudioReader::~MFAudioReader()
{

}

size_t MFAudioReader::GetAudioStreamCount()
{
	return this->audioStreamsArray.size();
}

uint32_t MFAudioReader::GetAudioChannelCount(int32_t index)
{
	uint32_t channelCount;
	Microsoft::WRL::ComPtr<IMFMediaType> type = GetType(index);
	type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channelCount);
	return channelCount;
}

uint32_t MFAudioReader::GetSampleRate(int32_t index)
{
	uint32_t sampleRate;
	Microsoft::WRL::ComPtr<IMFMediaType> type = GetType(index);
	type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
	return sampleRate;
}

AudioSampleType MFAudioReader::GetStreamType(int32_t audioIdx)
{
	HRESULT hr = S_OK;
	GUID tmpGuidType;
	Microsoft::WRL::ComPtr<IMFMediaType> realStreamType = GetType(audioIdx);

	hr = realStreamType->GetGUID(MF_MT_SUBTYPE, &tmpGuidType);

	if (tmpGuidType == MFAudioFormat_Float)
	{
		return AudioSampleType::FloatType;
	}
	else
	{
		return AudioSampleType::OtherType;
	}
}

void MFAudioReader::GetWaveInfo(int32_t audioIdx, WAVEFORMATEX* &waveType, uint32_t& waveLength)
{
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<IMFMediaType> streamType;
	int32_t audioStreamIndex = this->audioStreamsArray.at(audioIdx);
	hr = this->audioReader->GetCurrentMediaType(audioStreamIndex, &streamType);
	hr = MFCreateWaveFormatExFromMFMediaType(streamType.Get(), &waveType, &waveLength);
}

Int64Rational MFAudioReader::GetAudioDuration()
{
	Int64Rational realDur;
	LONGLONG realDuration = this->FindAudioDuration();
	realDur.SetValue(realDuration);
	realDur.SetRational(Int64Rational::Unit::HNS);
	return realDur;
}

IAudioSample* MFAudioReader::ReadAudioSample()
{
	MFAudioSample* sample = nullptr;
	Microsoft::WRL::ComPtr<IMFSample> audioSample = ReadSample();

	if (audioSample)
	{
		sample = new MFAudioSample();
		sample->Initialize(audioSample);
	}

	return sample;
}

void MFAudioReader::SetPosition(const Int64Rational& position)
{
	HRESULT hr = S_OK;
	PROPVARIANT varPos;
	ULONGLONG hnsPos;

	hnsPos = position.Convert_cr(Int64Rational::Unit::HNS).value;

	varPos.uhVal.QuadPart = hnsPos;
	varPos.vt = VARENUM::VT_I8;

	hr = this->audioReader->SetCurrentPosition(GUID_NULL, varPos);
}

void MFAudioReader::Initialize(Windows::Storage::Streams::IRandomAccessStream^ stream)
{
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<IMFByteStream> byteStream;
	Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
	Microsoft::WRL::ComPtr<IMFAttributes> mfAttributes;

	Auto::getInstance();

	hr = MFCreateMFByteStreamOnStreamEx((IUnknown*)stream, byteStream.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = MFCreateAttributes(&mfAttributes, 1);
		hr = mfAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
		hr = MFCreateSourceReaderFromByteStream(byteStream.Get(), mfAttributes.Get(), this->audioReader.GetAddressOf());
	}

	this->FindAudioStreamIndexes();

	if (!this->audioStreamsArray.empty())
	{
		hr = MFCreateMediaType(mediaType.GetAddressOf());
		hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);

		if (SUCCEEDED(hr))
		{
			hr = this->audioReader->SetCurrentMediaType(0, NULL, mediaType.Get());
		}

		if (FAILED(hr))
		{
			throw new std::exception("hr = this->audioReader->SetCurrentMediaType(0, NULL, mediaType.Get());  -  this string is failed");
		}
	}
}

Microsoft::WRL::ComPtr<IMFMediaType> MFAudioReader::GetType(int32_t index)
{
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<IMFMediaType> type;
	int32_t audioStreamIndex = this->audioStreamsArray.at(index);
	hr = this->audioReader->GetCurrentMediaType(audioStreamIndex, type.GetAddressOf());
	return type;
}

void MFAudioReader::FindAudioStreamIndexes()
{
	HRESULT hr = S_OK;
	DWORD dwStreamIndex = 0;
	Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
	GUID type;

	while (SUCCEEDED(hr))
	{
		hr = this->audioReader->GetCurrentMediaType(dwStreamIndex, &mediaType);

		if (SUCCEEDED(hr))
		{
			hr = mediaType->GetGUID(MF_MT_MAJOR_TYPE, &type);
		}

		if (type == MFMediaType_Audio && mediaType != NULL)
		{
			this->audioStreamsArray.push_back(dwStreamIndex);
			dwStreamIndex++;
		}
	}
}

Microsoft::WRL::ComPtr<IMFSample> MFAudioReader::ReadSample()
{
	HRESULT hr = S_OK;
	DWORD streamIndex, flags;
	LONGLONG timeStamp;
	Microsoft::WRL::ComPtr<IMFSample> sample;
	Microsoft::WRL::ComPtr<IMFMediaType> tmpType;

	hr = this->audioReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &timeStamp, sample.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		hr = this->audioReader->GetCurrentMediaType(streamIndex, &tmpType);
	}
	else
	{
		sample = nullptr;
	}

	return sample;
}

int64_t MFAudioReader::FindAudioDuration()
{
	HRESULT hr = S_OK;
	PROPVARIANT var;
	int64_t duration = 0;

	hr = this->audioReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);

	if (SUCCEEDED(hr) && var.vt == VARENUM::VT_UI8)
	{
		duration = static_cast<LONGLONG>(var.uhVal.QuadPart);
	}

	return duration;
}
