using EarthInBeatsEngine.Audio;
using System;
using System.Collections.Generic;
using System.IO;
using Windows.Storage;
using Windows.Storage.FileProperties;
using Windows.Storage.Streams;

namespace EarthInBeatsApp.AudioData
{
    public sealed class PlayList : IPlayList
    {
        private List<Track> trackList = new List<Track>();
        private List<StorageFile> files = new List<StorageFile>();
        private List<IRandomAccessStream> streamsToSongs = new List<IRandomAccessStream>();

        // custom methods of the PlayList class
        public PlayList() { }

        public PlayList(List<Track> tracks)
        {
            if (tracks != null)
            {
                this.trackList = tracks;
            }
        }

        public void CreateNewPlayList(List<string> songNames, List<IRandomAccessStream> streams, List<StorageFile> inputFiles)
        {
            this.trackList.Clear();

            if (inputFiles != null)
            {
                this.files = inputFiles;
            }

            if (streams != null)
            {
                this.streamsToSongs = streams;
            }

            for (int i = 0; i < songNames.Count; i++)
            {
                this.AddTrackToPlaylist(i, songNames[i]);
            }
        }

        public void AddTracksToExistingPlayList(List<string> newSongs, List<IRandomAccessStream> newStreams, List<StorageFile> newFiles)
        {
            if (newSongs != null && newStreams != null && newFiles != null)
            {
                foreach (var stream in newStreams)
                {
                    this.streamsToSongs.Add(stream);
                }

                foreach (var file in newFiles)
                {
                    this.files.Add(file);
                }

                for (int i = 0; i < newSongs.Count; i++)
                {
                    this.AddTrackToPlaylist(this.trackList.Count + i, newSongs[i]);
                }
            }
        }

        private void AddTrackToPlaylist(int position, string name)
        {
            this.trackList.Add(new Track(position, name));
        }

        // implementation of IPlayList interface methods
        public void SortPlaylist()
        {
            this.trackList.Sort((track1, track2) => track1.Position - track2.Position);
        }

        public ITrack GetTrack(int index)
        {
            if (index >= 0 && index < this.trackList.Count)
            {
                return this.trackList[index];
            }

            return null;
        }

        public IRandomAccessStream GetStream(int index)
        {
            if (index >= 0 && index < this.streamsToSongs.Count)
            {
                return this.streamsToSongs[index];
            }

            return null;
        }

        public string GetInfoAboutTrack(int index)
        {
            if (index >= 0 && index < this.files.Count)
            {
                StorageItemContentProperties contentProperties = this.files[index].Properties;
                var musicPropertiesTask = contentProperties.GetMusicPropertiesAsync().AsTask();
                musicPropertiesTask.Wait();
                MusicProperties musicProperties = musicPropertiesTask.Result;

                string infoAboutTracks = "Track name: " + musicProperties.Title +
                                  ", Artist: " + musicProperties.Artist +
                                  ", Album: " + musicProperties.Album;

                return infoAboutTracks;
            }

            return String.Empty;
        }

        public bool CheckNext(int currentIndex)
        {
            if (currentIndex + 1 < this.trackList.Count &&
                this.trackList[currentIndex + 1] != null &&
                this.trackList[currentIndex + 1].GetName() != "")  // not sure about name
            {
                return true;
            }

            return false;
        }

        public int GetPlayListLength()
        {
            return (trackList != null) ? trackList.Count : 0;
        }
    }
}