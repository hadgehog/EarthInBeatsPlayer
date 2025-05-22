#include "XAudio2Player.h"
#include "..\AudioData\AudioReader\MFAudioReader.h"
#include "..\AudioData\AudioSample\MFAudioSample.h"
#include "..\AudioHelpers\Auto.h"

void XAudio2Player::OnBufferEnd(void* pContext)
{
	if (!this->audioSamples.empty())
	{
		this->DeleteSamples();
	}

	SubmitBuffer();
}
