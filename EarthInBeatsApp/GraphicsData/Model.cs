using EarthInBeatsEngine.Graphics;
using System.Collections.Generic;

namespace EarthInBeatsApp.GraphicsData
{
    public sealed class Model   // : IModel
    {
        public string ModelPath { get; set; }

        public string ModelName { get; set; }

        public ModelType ModelType { get; set; }

        public List<string> TexturePaths { get; set; }

        public Model()
        {
            this.ModelPath = string.Empty;
            this.ModelName = string.Empty;
            this.ModelType = ModelType.Earth;
        }

        public string GetModelPath() => this.ModelPath;

        public string GetModelName() => this.ModelName;

        public ModelType GetModelType() => this.ModelType;

        public List<string> GetTexturePaths() => this.TexturePaths;
    }
}
