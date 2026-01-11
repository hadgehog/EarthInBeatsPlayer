using EarthInBeatsEngine.Audio;

namespace EarthInBeatsApp.AudioData
{
    public sealed class Track : ITrack
    {
        public int Position { get; set; }
        public string Name { get; set; }

        public Track()
        {
            this.Position = 0;
            this.Name = string.Empty;
        }

        public Track(int position, string name)
        {
            this.Position = position;
            this.Name = name;
        }

        public int GetPosition() => Position;
        public string GetName() => Name;
    }
}
