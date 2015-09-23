using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Taygeta.Imaging;

namespace Taygeta.Processing
{
    /// <summary>
    /// Base type for processing items
    /// </summary>
    public interface IBaseItem : IDisposable, ICloneable
    {
        /// <summary>
        /// Gets or sets value indication whether successor nodes should handle this frame
        /// </summary>
        bool ShouldContinueProcessing { get; set; }

        /// <summary>
        /// Gets or sets current video frame
        /// </summary>
        PlanarImage Target { get; set; }

        /// <summary>
        /// Gets or sets collections of video meta data objects
        /// </summary>
        MetaData MetaDatum { get; set; }
    }
}
