#include "..\..\AudioHelpers\Auto.h"
#include "MFAudioSample.h"

MFAudioSample::MFAudioSample()
	: sampleTime(0), sampleDuration(0)
{

}

MFAudioSample::~MFAudioSample()
{

}

int64_t MFAudioSample::GetDuration()
{
	return this->sampleDuration;
}

int64_t MFAudioSample::GetSampleTime()
{
	return this->sampleTime;
}

void MFAudioSample::Lock(void** buffer, uint64_t* size)
{
	HRESULT hr = S_OK;
	DWORD sizeTmp;
	BYTE *bufTmp;

	hr = this->sampleBuffer->Lock(&bufTmp, NULL, &sizeTmp);

	*size = sizeTmp;
	*buffer = bufTmp;
}

void MFAudioSample::Unlock(void* buffer, uint64_t size)
{
	HRESULT hr = S_OK;
	hr = this->sampleBuffer->Unlock();
}

void MFAudioSample::Initialize(winrt::com_ptr<IMFSample> sample)
{
	HRESULT hr = S_OK;
	Auto::getInstance();
	hr = sample->GetSampleDuration(&this->sampleDuration);
	hr = sample->GetSampleTime(&this->sampleTime);
	hr = sample->ConvertToContiguousBuffer(this->sampleBuffer.put());
}