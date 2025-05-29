#include "AudioPlayer.h"
#include "Audio.AudioPlayer.g.cpp"
#include "AudioEngine/XAudio2Player.h"
#include "AudioHelpers/InitMasterVoice.h"
#include "AudioEvents/MFAudioEvents.h"

namespace winrt::EarthInBeatsEngine::Audio::implementation
{
    AudioPlayer::AudioPlayer()
        : globalDuration(0), currentPlayerIndex(0), 
        isPauseOccurs(false), isPlayingNow(false), repeatMode(RepeatMode::None)
    {
        OutputDebugString(L"AudioPlayer created\n");
    }

    AudioPlayer::~AudioPlayer()
    {
        OutputDebugString(L"AudioPlayer destroyed\n");
    }

	void AudioPlayer::InitAudioPlayer(const IPlayList& playList)
	{
        this->playersList.clear();
        this->tracksInfo.clear();
        this->globalDuration = 0;
        this->currentPlayerIndex = 0;
        this->isPauseOccurs = false;
        this->isPlayingNow = false;
        repeatMode = RepeatMode::None;

        this->currentPlayList = playList;
        this->currentPlayList.SortPlaylist();
        this->FindGlobalDuration();

        for (int32_t i = 0; i < this->currentPlayList.GetPlayListLength(); i++)
        {
            auto reader = std::make_shared<MFAudioReader>();
            auto player = std::make_shared<XAudio2Player>();        

            this->xAudio2 = InitMasterVoice::GetInstance().GetXAudio();
            
            {
                auto tmpEvents = std::make_shared<MFAudioEvents>();

                tmpEvents->InitEvent(this);

                this->audioEvents = std::shared_ptr<MFAudioEvents>(tmpEvents);
            }
            // TODO: rework to get rid of tmp pointers
            //std::dynamic_pointer_cast<MFAudioEvents>(this->audioEvents)->InitEvent(this);

            winrt::Windows::Storage::Streams::IRandomAccessStream stream = this->currentPlayList.GetStream(i);

            reader->Initialize(stream);
            player->Initialize(reader, this->xAudio2, this->audioEvents);

            std::lock_guard<std::mutex> lock(this->lockPlayList);
            this->playersList.push_back(player);
        }
	}

    void AudioPlayer::Play()
    {
        if (this->playersList.at(this->currentPlayerIndex))
        {
            if (!this->isPauseOccurs) 
            {
                this->playersList.at(this->currentPlayerIndex)->Stop();
            }

            this->playersList.at(this->currentPlayerIndex)->Play();
            this->isPauseOccurs = false;
            this->isPlayingNow = true;
        }
    }

    void AudioPlayer::Pause()
    {
        if (this->playersList.at(this->currentPlayerIndex) && this->isPlayingNow)
        {
            this->isPauseOccurs = true;
            this->playersList.at(this->currentPlayerIndex)->Pause();
            this->isPlayingNow = false;
        }
    }

    void AudioPlayer::Stop()
    {
        for (size_t i = 0; i < this->playersList.size(); i++)
        {
            if (this->playersList.at(i))
            {
                this->playersList.at(i)->Stop();

                DoubleRational tmpPos(static_cast<double>(0.0), DoubleRational::Unit::SEC);
                Int64Rational pos(static_cast<Int64Rational::Type>(tmpPos.Convert_cr(DoubleRational::Unit::HNS).value), Int64Rational::Unit::HNS);

                this->playersList.at(i)->SetPosition(pos);
            }
        }

        this->isPlayingNow = false;
    }

    void AudioPlayer::Next()
    {
        if (this->currentPlayerIndex + 1 < (int32_t)playersList.size())
        {
            if (this->playersList.at(this->currentPlayerIndex + 1))
            {
                this->playersList.at(this->currentPlayerIndex)->Stop();

                DoubleRational tmpPos(static_cast<double>(0.0), DoubleRational::Unit::SEC);
                Int64Rational pos(static_cast<Int64Rational::Type>(tmpPos.Convert_cr(DoubleRational::Unit::HNS).value), Int64Rational::Unit::HNS);

                this->playersList.at(this->currentPlayerIndex + 1)->SetPosition(pos);
                this->currentPlayerIndex++;

                if (this->isPlayingNow)
                {
                    this->playersList.at(this->currentPlayerIndex + 1)->Play();
                }
            }
        }
    }

    void AudioPlayer::Previous()
    {
        if (this->currentPlayerIndex - 1 >= 0)
        {
            if (this->playersList.at(this->currentPlayerIndex - 1))
            {
                this->playersList.at(this->currentPlayerIndex)->Stop();

                DoubleRational tmpPos(static_cast<double>(0.0), DoubleRational::Unit::SEC);
                Int64Rational pos(static_cast<Int64Rational::Type>(tmpPos.Convert_cr(DoubleRational::Unit::HNS).value), Int64Rational::Unit::HNS);

                this->playersList.at(this->currentPlayerIndex)->SetPosition(pos);
                this->currentPlayerIndex--;

                if (isPlayingNow) 
                {
                    this->playersList.at(this->currentPlayerIndex - 1)->Play();
                }
            }
        }
    }

    void AudioPlayer::Rewind(double position)
    {
        if (this->playersList.at(this->currentPlayerIndex))
        {
            DoubleRational tmpPos(static_cast<double>(position), DoubleRational::Unit::SEC);
            Int64Rational pos(static_cast<Int64Rational::Type>(tmpPos.Convert_cr(DoubleRational::Unit::HNS).value), Int64Rational::Unit::HNS);

            this->playersList.at(this->currentPlayerIndex)->SetPosition(pos);
        }
    }

    winrt::Windows::Foundation::TimeSpan AudioPlayer::Duration() const
    {        
        if (this->playersList.at(this->currentPlayerIndex))
        {
            winrt::Windows::Foundation::TimeSpan winrtDuration { this->playersList.at(this->currentPlayerIndex)->GetDuration() };
            return winrtDuration;
        }

        return winrt::Windows::Foundation::TimeSpan { 0 };
    }

    winrt::Windows::Foundation::TimeSpan AudioPlayer::GlobalDuration() const
    {
        winrt::Windows::Foundation::TimeSpan winrtGlobalDuration { this->globalDuration };
        return winrtGlobalDuration;
    }

    winrt::Windows::Foundation::TimeSpan AudioPlayer::CurrentPosition() const
    {
        if (this->playersList.at(this->currentPlayerIndex))
        {
            winrt::Windows::Foundation::TimeSpan winrtCurrPos { this->playersList.at(this->currentPlayerIndex)->GetCurrentPosition() };
            return winrtCurrPos;
        }

        return winrt::Windows::Foundation::TimeSpan(0);
    }

    float AudioPlayer::Volume() const
    {
        return this->playersList.at(this->currentPlayerIndex)->GetVolume();
    }

    void AudioPlayer::Volume(float volume)
    {
        for (size_t i = 0; i < this->playersList.size(); i++)
        {
            if (this->playersList.at(i))
            {
                this->playersList.at(i)->SetVolume(volume);
            }
        }
    }

    bool AudioPlayer::IsPlayingNow() const
    {
        return this->isPlayingNow;
    }

    void AudioPlayer::ClearPlayList()
    {
        if (this->isPlayingNow)
        {
            this->Stop();
        }

        this->currentPlayList = nullptr;
        this->playersList.clear();
        this->tracksInfo.clear();
    }

    void AudioPlayer::Repeat(RepeatMode mode)
    {
        this->repeatMode = mode;
    }

    void AudioPlayer::EndOfRewindingTrack()
    {

    }

    void AudioPlayer::EndOfPlayingTrack(int32_t nextIdx)
    {
        if (this->playersList.at(this->currentPlayerIndex))
        {
            this->Stop();

            switch (this->repeatMode)
            {
                case RepeatMode::RepeateOne:
                {
                    // play again the current track
                    this->Play();
                    break;
                }
                case RepeatMode::RepeateAll:
                {
                    // play till the end, then go to the first track
                    this->currentPlayerIndex++;

                    if (this->currentPlayerIndex >= (int32_t)this->playersList.size())
                    {
                        this->currentPlayerIndex = 0;
                    }

                    this->Play();

                    break;
                }
                case RepeatMode::None:
                default:
                {
                    // go ahead to the next track
                    this->currentPlayerIndex++;

                    if (this->currentPlayerIndex < (int32_t)this->playersList.size())
                    {
                        this->Play();
                    }
                    else
                    {
                        this->currentPlayerIndex--;
                    }

                    break;
                }
            }

            this->playbackEndedEvent(*this, this->isPlayingNow);
        }
    }

    winrt::event_token AudioPlayer::PlaybackEnded(const Windows::Foundation::EventHandler<bool>& handler)
    {
        return this->playbackEndedEvent.add(handler);
    }

    void AudioPlayer::PlaybackEnded(const winrt::event_token& token)
    {
        this->playbackEndedEvent.remove(token);
    }

    void AudioPlayer::FindGlobalDuration()
    {
        for (int32_t i = 0; i < this->currentPlayList.GetPlayListLength(); i++)
        {
            this->globalDuration += this->FindSongDurationFromPlayList(i);
        }
    }

    int64_t AudioPlayer::FindSongDurationFromPlayList(int trackIndex)
    {
        int64_t convertSongDuration = 0;
        auto reader = std::make_unique<MFAudioReader>(MFAudioReader());
        auto stream = this->currentPlayList.GetStream(trackIndex);

        reader->Initialize(stream);

        Int64Rational songDuration = reader->GetAudioDuration();
        Int64Rational pos(songDuration.Convert_cr(Int64Rational::Unit::SEC).value, Int64Rational::Unit::HNS);
        convertSongDuration = pos.value;

        return convertSongDuration;
    }
}