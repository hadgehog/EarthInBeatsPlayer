using EarthInBeatsApp.AudioData;
using EarthInBeatsEngine.Audio;
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
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

        public MainPage()
        {
            this.InitializeComponent();
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
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                this.PlayPauseBtn.Icon = new SymbolIcon(Symbol.Pause);
            });

            if (this.audioPlayer != null)
            {
                this.audioPlayer.Volume = (float)this.VolumeSlider.Value / 100;
                this.audioPlayer.Play();
            }
        }

        private async void PlayPauseBtn_Unchecked(object sender, RoutedEventArgs e)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                this.PlayPauseBtn.Icon = new SymbolIcon(Symbol.Play);
            });

            if (this.audioPlayer != null)
            {
                this.audioPlayer.Pause();
            }
        }

        private void Stop_Click(object sender, RoutedEventArgs e)
        {
            if (this.audioPlayer != null)
            {
                this.audioPlayer.Stop();
            }

            this.ProgressSlider.Value = 0;
            this.PlayPauseBtn.IsChecked = false;
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

            //TODO correct convertion to List !!!!
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
                }
                else
                {
                    // add tracks to existing playlist
                    this.audioPlayer.Stop();  // TODO: add new tracks without stopping
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

                    this.ProgressSlider.Value = 0.0;

                    GC.Collect();

                    this.audioPlayer = null;
                    this.playList = null;
                }
            }
        }

        private void MyPlaylists_Click(object sender, RoutedEventArgs e)
        {

        }

        private async void VolumeBtn_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            this.isHoveringVolume = true;

            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
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
            repeatMode++;

            if (repeatMode > RepeatMode.RepeateAll)
            {
                repeatMode = RepeatMode.None;
            }

            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                switch (repeatMode)
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
        }

        private void VolumeSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {

        }

        private void VolumeSlider_ManipulationStarted(object sender, ManipulationStartedRoutedEventArgs e)
        {

        }

        private void VolumeSlider_ManipulationCompleted(object sender, ManipulationCompletedRoutedEventArgs e)
        {

        }

        private void ProgressSlider_PointerPressed(object sender, PointerRoutedEventArgs e)
        {

        }

        private void ProgressSlider_PointerReleased(object sender, PointerRoutedEventArgs e)
        {

        }

        private void ProgressSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {

        }

        private void ProgressSlider_ManipulationCompleted(object sender, ManipulationCompletedRoutedEventArgs e)
        {

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
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    this.VolumeSlider.Visibility = Visibility.Collapsed;
                });
            }
        }
    }
}
