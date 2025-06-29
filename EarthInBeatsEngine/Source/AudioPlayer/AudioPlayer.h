#pragma once

#include "Audio.AudioPlayer.g.h"

#include "AudioEngine/XAudio2Player.h"
#include "AudioData/AudioReader/MFAudioReader.h"

#include <mutex>
#include <memory>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>

// TODO: refactor includes, get rid of extra winapi includes, maybe add pch.h to manage dependencies

namespace winrt::EarthInBeatsEngine::Audio::implementation
{
    struct AudioPlayer : AudioPlayerT<AudioPlayer>
    {
        AudioPlayer();
        ~AudioPlayer();

        void InitAudioPlayer(const IPlayList& playList);

        void Play();
        void Pause();
        void Stop();

        void Next();
        void Previous();
        void Rewind(double position);

        // TODO: fix these methods to return true TimeSpan in sec:min format
        winrt::Windows::Foundation::TimeSpan Duration() const;
        winrt::Windows::Foundation::TimeSpan GlobalDuration() const;
        winrt::Windows::Foundation::TimeSpan CurrentPosition() const;

        float Volume() const;
        void Volume(float volume);

        bool IsPlayingNow() const;

        void ClearPlayList();

        void Repeat(RepeatMode mode);

        void EndOfRewindingTrack();
        void EndOfPlayingTrack(int32_t nextIdx);

        winrt::event_token PlaybackEnded(const Windows::Foundation::EventHandler<bool>& handler);
        void PlaybackEnded(const winrt::event_token& token);

    private:
        winrt::com_ptr<IXAudio2> xAudio2 { nullptr };
        IPlayList currentPlayList { nullptr };
        std::shared_ptr<IAudioEvents> audioEvents { nullptr };
        std::mutex lockPlayList;
        std::vector<std::shared_ptr<XAudio2Player>> playersList;
        int64_t globalDuration { 0 };
        int32_t currentPlayerIndex { 0 };
        std::vector<winrt::hstring> tracksInfo;
        bool isPauseOccurs { false };
        bool isPlayingNow { false };
        RepeatMode repeatMode { RepeatMode::None };

        winrt::event<Windows::Foundation::EventHandler<bool>> playbackEndedEvent;

    private:
        void FindGlobalDuration();
        int64_t FindSongDurationFromPlayList(int trackIndex);
    };
}

namespace winrt::EarthInBeatsEngine::Audio::factory_implementation
{
    struct AudioPlayer : AudioPlayerT<AudioPlayer, implementation::AudioPlayer>
    {
    };
}