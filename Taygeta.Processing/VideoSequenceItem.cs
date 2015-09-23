using System;
using Declarations;
using Taygeta.Imaging;

namespace Taygeta.Processing
{
    public class VideoSequenceItem : IBaseItem
    {
        public bool ShouldContinueProcessing { get; set; }

        public PlanarImage Target { get; set; }

        public MetaData MetaDatum { get; set; }

        public VideoSequenceItem()
        {
            ShouldContinueProcessing = true;
        }

        public void Dispose()
        {
            if (Target != null)
            {
                Target.Dispose();
                Target = null;
            }
        }

        public object Clone()
        {
            VideoSequenceItem frame = new VideoSequenceItem();
            frame.Target = (PlanarImage)this.Target.Clone();
            frame.ShouldContinueProcessing = this.ShouldContinueProcessing;

            return frame;
        }

        public static VideoSequenceItem FromPlanarFrame(PlanarFrame data, BitmapFormat format)
        {
            VideoSequenceItem frame = new VideoSequenceItem();
            frame.Target = new PlanarImage(format.Width, format.Height, GetYuvType(format.ChromaType), data.Planes);
            return frame;
        }

        private static PixelAlignmentType GetYuvType(ChromaType chromaType)
        {
            switch (chromaType)
            {
                case ChromaType.YV12:
                    return PixelAlignmentType.YV12;
                case ChromaType.I420:
                    return PixelAlignmentType.I420;
                case ChromaType.NV12:
                    return PixelAlignmentType.NV12;
                case ChromaType.YUY2:
                    return PixelAlignmentType.YUY2;
                case ChromaType.UYVY:
                    return PixelAlignmentType.UYVY;
                default:
                    throw new ArgumentException("Invalid parameter " + chromaType.ToString());
            }
        }    
    }
}
