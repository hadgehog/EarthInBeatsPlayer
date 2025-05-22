#pragma once

#include <cstdint>

enum class AudioSampleType	
{ 
	FloatType, 
	OtherType
};

class IAudioSample
{
public:
	IAudioSample() = default;
	virtual ~IAudioSample() = default;

	virtual int64_t GetDuration() = 0;
	virtual int64_t GetSampleTime() = 0;

	template<class Callable>
	void GetData(Callable accessor)
	{
		void *buffer = nullptr;
		uint64_t size;

		this->Lock(&buffer, &size);

		accessor(buffer, size);

		this->Unlock(buffer, size);
	}

protected:
	virtual void Lock(void** buffer, uint64_t* size) = 0;
	virtual void Unlock(void* buffer, uint64_t size) = 0;
};