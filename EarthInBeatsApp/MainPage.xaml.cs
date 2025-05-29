using EarthInBeatsApp.AudioData;
using EarthInBeatsEngine.Audio;
using ProtoBuf.WellKnownTypes;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.ConstrainedExecution;
using System.Threading.Tasks;
using Windows.ApplicationModel.Core;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;
using Windows.UI.Core;
using Windows.UI.Input;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;

namespace EarthInBeatsApp
{
    /// <summary>
    /// Earth In Beats Player main page
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private enum RepeatMode : int
        {
            None,
            RepeateOne,
            RepeateAll
        }

        private RepeatMode repeatMode = RepeatMode.None;
        private bool isHoveringVolume = false;

        private AudioPlayer audioPlayer = null;
        private PlayList playList = null;

        private CoreDispatcher dispatcher = CoreApplication.MainView.Dispatcher;
        private bool isDragging = false;

        public MainPage()
        {
            this.InitializeComponent();

            this.VolumeSlider.Value = 70.0;
            this.VolumeSlider.Maximum = 100.0;
            this.ProgressSlider.Value = 0.0;
            this.ProgressSlider.Maximum = 100.0;

            this.TrackTimeText.Text = "00:00/00:00";

            this.ProgressSlider.AddHandler(PointerPressedEvent, new PointerEventHandler(ProgressSlider_PointerPressed), true);
            this.ProgressSlider.AddHandler(PointerReleasedEvent, new PointerEventHandler(ProgressSlider_PointerReleased), true);
        }

        private async void StartProgress()
        {
            while (this.audioPlayer != null &&
                   this.audioPlayer.IsPlayingNow &&
                   this.audioPlayer.Duration.Ticks > 0 &&
                   this.audioPlayer.CurrentPosition.Ticks <= this.audioPlayer.Duration.Ticks)
            {
                await this.dispatcher.RunAsync(CoreDispatcherPriority.High, () =>
                {
                    if (!this.isDragging)
                    {
                        this.UpdatePlaybackProgress();
                    }

                });

                await Task.Delay(TimeSpan.FromSeconds(1.0));
            }
        }

        private void StopProgress()
        {
            this.dispatcher.StopProcessEvents();
        }

        private void UpdatePlaybackProgress()
        {
            var duration = this.audioPlayer.Duration.Ticks;
            var currentPos = this.audioPlayer.CurrentPosition.Ticks;

            this.ProgressSlider.Value = (currentPos * 100.0) / duration;

            TimeSpan timeCurr = TimeSpan.FromSeconds(currentPos);
            TimeSpan timeOverall = TimeSpan.FromSeconds(duration);

            string currStr = timeCurr.ToString(@"mm\:ss");
            string overallStr = timeOverall.ToString(@"mm\:ss");

            this.TrackTimeText.Text = currStr + "/" + overallStr;
        }

        private void Previous_Click(object sender, RoutedEventArgs e)
        {
            if (this.audioPlayer != null)
            {
                this.audioPlayer.Previous();
            }
        }

        private async void PlayPauseBtn_Checked(object sender, RoutedEventArgs e)
        {
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                this.PlayPauseBtn.Icon = new SymbolIcon(Symbol.Pause);
            });

            if (this.audioPlayer != null)
            {
                this.audioPlayer.Volume = (float)this.VolumeSlider.Value / 100;
                this.audioPlayer.Play();
                this.StartProgress();
            }
        }

        private async void PlayPauseBtn_Unchecked(object sender, RoutedEventArgs e)
        {
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                this.PlayPauseBtn.Icon = new SymbolIcon(Symbol.Play);
            });

            if (this.audioPlayer != null)
            {
                this.audioPlayer.Pause();
                this.StopProgress();
            }
        }

        private async void Stop_Click(object sender, RoutedEventArgs e)
        {
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                this.ProgressSlider.Value = 0;
                this.PlayPauseBtn.IsChecked = false;

                var duration = this.audioPlayer.Duration.Ticks;
                TimeSpan timeOverall = TimeSpan.FromSeconds(duration);
                string overallStr = timeOverall.ToString(@"mm\:ss");

                this.TrackTimeText.Text = "00:00/" + overallStr;
            });

            if (this.audioPlayer != null)
            {
                this.audioPlayer.Stop();
                this.StopProgress();
            }
        }

        private void Next_Click(object sender, RoutedEventArgs e)
        {
            if (this.audioPlayer != null)
            {
                this.audioPlayer.Next();
            }
        }

        private async void NewPlaylist_Click(object sender, RoutedEventArgs e)
        {
            FileOpenPicker filePicker = new FileOpenPicker();

            filePicker.ViewMode = PickerViewMode.List;
            filePicker.SuggestedStartLocation = PickerLocationId.ComputerFolder;

            filePicker.FileTypeFilter.Add(".mp3");
            filePicker.FileTypeFilter.Add(".wav");
            filePicker.FileTypeFilter.Add(".wma");
            filePicker.FileTypeFilter.Add(".aac");
            filePicker.FileTypeFilter.Add(".flac");
            filePicker.FileTypeFilter.Add(".mp4");
            filePicker.FileTypeFilter.Add(".wave");

            //TODO correct conversion to List !!!!
            var pickedFilesTmp = await filePicker.PickMultipleFilesAsync();
            var pickedFiles = new List<StorageFile>(pickedFilesTmp);

            if (pickedFiles != null && pickedFiles.Count != 0)
            {
                var names = new List<string>();
                var streams = new List<IRandomAccessStream>();

                foreach (var file in pickedFiles)
                {
                    var path = file.Path;
                    var name = file.Name;

                    names.Add(name);

                    var stream = await file.OpenStreamForReadAsync();
                    IRandomAccessStream resultStream = stream.AsRandomAccessStream();
                    streams.Add(resultStream);
                }

                if (this.audioPlayer == null)
                {
                    //create new playlist
                    this.playList = new PlayList();
                    this.playList.CreateNewPlayList(names, streams, pickedFiles);

                    //init players list
                    this.audioPlayer = new AudioPlayer();
                    this.audioPlayer.InitAudioPlayer(this.playList);

                    this.audioPlayer.PlaybackEnded += AudioPlayer_PlaybackEnded;
                }
                else
                {
                    // add tracks to existing playlist
                    this.audioPlayer.Stop();  // TODO: add new tracks without stopping

                    // dumb check
                    if (this.playList == null)
                    {
                        this.playList = new PlayList();
                    }

                    this.playList.AddTracksToExistingPlayList(names, streams, pickedFiles);
                    this.audioPlayer.InitAudioPlayer(this.playList);
                }
            }
        }

        private void ClearPlaylist_Click(object sender, RoutedEventArgs e)
        {
            if (this.audioPlayer != null)
            {
                if (this.playList != null)
                {
                    this.audioPlayer.Stop();
                    this.audioPlayer.ClearPlayList();

                    this.audioPlayer.PlaybackEnded -= AudioPlayer_PlaybackEnded;

                    this.ProgressSlider.Value = 0.0;
                    this.PlayPauseBtn.IsChecked = false;

                    GC.Collect();

                    this.playList = null;
                    this.audioPlayer = null;
                }
            }
        }

        private void ShowCurrentPlaylist_Click(object sender, RoutedEventArgs e)
        {

        }

        private async void VolumeBtn_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            this.isHoveringVolume = true;

            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                this.VolumeSlider.Visibility = Visibility.Visible;
            });
        }

        private void VolumeBtn_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            this.isHoveringVolume = false;
            this.HideVolumeSliderWithDelay();
        }

        private void VolumeSlider_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            this.isHoveringVolume = true;
        }

        private async void Repeat_Click(object sender, RoutedEventArgs e)
        {
            this.repeatMode++;

            if (this.repeatMode > RepeatMode.RepeateAll)
            {
                this.repeatMode = RepeatMode.None;
            }

            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                switch (this.repeatMode)
                {
                    case RepeatMode.RepeateOne:
                        this.Repeat.Icon = new SymbolIcon(Symbol.RepeatOne);
                        this.Repeat.IsChecked = true;
                        break;
                    case RepeatMode.RepeateAll:
                        this.Repeat.Icon = new SymbolIcon(Symbol.RepeatAll);
                        this.Repeat.IsChecked = true;
                        break;
                    case RepeatMode.None:
                    default:
                        this.Repeat.Icon = new SymbolIcon(Symbol.RepeatAll);
                        this.Repeat.IsChecked = false;
                        break;
                }
            });

            if (this.audioPlayer != null)
            {
                this.audioPlayer.Repeat((EarthInBeatsEngine.Audio.RepeatMode)this.repeatMode);
            }
        }

        private void VolumeSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (this.audioPlayer != null)
            {
                this.audioPlayer.Volume = (float)e.NewValue / 100;
            }
        }

        private void ProgressSlider_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            this.isDragging = true;
        }

        private void ProgressSlider_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (this.audioPlayer != null)
            {
                var pointerId = e.Pointer.PointerId;
                var point = e.GetCurrentPoint(this.ProgressSlider);
                var position = point.Position.X >= 0 ? point.Position.X : 0.0;

                var newPosition = (position / this.ProgressSlider.ActualWidth) * this.audioPlayer.Duration.Ticks;

                this.audioPlayer.Rewind(newPosition);
            }

            this.isDragging = false;
        }

        private void Shuffle_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void Shuffle_Unchecked(object sender, RoutedEventArgs e)
        {

        }

        private async void HideVolumeSliderWithDelay()
        {
            await Task.Delay(TimeSpan.FromSeconds(2.0));

            if (!this.isHoveringVolume)
            {
                await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                {
                    this.VolumeSlider.Visibility = Visibility.Collapsed;
                });
            }
        }

        private async void AudioPlayer_PlaybackEnded(object sender, bool e/*isPlayingNow*/)
        {
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                this.ProgressSlider.Value = 0;
                this.PlayPauseBtn.IsChecked = e;

                if (this.audioPlayer != null)
                {
                    var duration = this.audioPlayer.Duration.Ticks;
                    TimeSpan timeOverall = TimeSpan.FromSeconds(duration);
                    string overallStr = timeOverall.ToString(@"mm\:ss");

                    this.TrackTimeText.Text = "00:00/" + overallStr;
                }
                else
                {
                    this.TrackTimeText.Text = "00:00/00:00";
                }
            });
        }

        private void PlayPauseBtn_Click(object sender, RoutedEventArgs e)
        {
            if (this.audioPlayer == null || this.playList == null)
            {
                this.PlayPauseBtn.IsChecked = false;
            }
        }
    }
}
