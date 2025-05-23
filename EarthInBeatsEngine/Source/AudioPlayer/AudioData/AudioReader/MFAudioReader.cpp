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
	winrt::com_ptr<IMFMediaType> type = GetType(index);
	type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channelCount);
	return channelCount;
}

uint32_t MFAudioReader::GetSampleRate(int32_t index)
{
	uint32_t sampleRate;
	winrt::com_ptr<IMFMediaType> type = GetType(index);
	type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
	return sampleRate;
}

AudioSampleType MFAudioReader::GetStreamType(int32_t audioIdx)
{
	HRESULT hr = S_OK;
	GUID tmpGuidType;
	winrt::com_ptr<IMFMediaType> realStreamType = GetType(audioIdx);

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
	winrt::com_ptr<IMFMediaType> streamType;
	int32_t audioStreamIndex = this->audioStreamsArray.at(audioIdx);
	hr = this->audioReader->GetCurrentMediaType(audioStreamIndex, streamType.put());
	hr = MFCreateWaveFormatExFromMFMediaType(streamType.get(), &waveType, &waveLength);
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
	winrt::com_ptr<IMFSample> audioSample = ReadSample();

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

void MFAudioReader::Initialize(winrt::Windows::Storage::Streams::IRandomAccessStream stream)
{
	HRESULT hr = S_OK;
	winrt::com_ptr<IMFByteStream> byteStream;
	winrt::com_ptr<IMFMediaType> mediaType;
	winrt::com_ptr<IMFAttributes> mfAttributes;

	Auto::getInstance();

	hr = MFCreateMFByteStreamOnStreamEx(winrt::get_unknown(stream), byteStream.put());

	if (SUCCEEDED(hr))
	{
		hr = MFCreateAttributes(mfAttributes.put(), 1);
		hr = mfAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
		hr = MFCreateSourceReaderFromByteStream(byteStream.get(), mfAttributes.get(), this->audioReader.put());
	}

	this->FindAudioStreamIndexes();

	if (!this->audioStreamsArray.empty())
	{
		hr = MFCreateMediaType(mediaType.put());
		hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);

		if (SUCCEEDED(hr))
		{
			hr = this->audioReader->SetCurrentMediaType(0, NULL, mediaType.get());
		}

		if (FAILED(hr))
		{
			throw new std::exception("hr = this->audioReader->SetCurrentMediaType(0, NULL, mediaType.Get());  -  this string is failed");
		}
	}
}

winrt::com_ptr<IMFMediaType> MFAudioReader::GetType(int32_t index)
{
	HRESULT hr = S_OK;
	winrt::com_ptr<IMFMediaType> type;
	int32_t audioStreamIndex = this->audioStreamsArray.at(index);
	hr = this->audioReader->GetCurrentMediaType(audioStreamIndex, type.put());
	return type;
}

void MFAudioReader::FindAudioStreamIndexes()
{
	HRESULT hr = S_OK;
	DWORD dwStreamIndex = 0;
	winrt::com_ptr<IMFMediaType> mediaType;
	GUID type;

	while (SUCCEEDED(hr))
	{
		hr = this->audioReader->GetCurrentMediaType(dwStreamIndex, mediaType.put());

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

winrt::com_ptr<IMFSample> MFAudioReader::ReadSample()
{
	HRESULT hr = S_OK;
	DWORD streamIndex, flags;
	LONGLONG timeStamp;
	winrt::com_ptr<IMFSample> sample;
	winrt::com_ptr<IMFMediaType> tmpType;

	hr = this->audioReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &timeStamp, sample.put());

	if (SUCCEEDED(hr))
	{
		hr = this->audioReader->GetCurrentMediaType(streamIndex, tmpType.put());
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
