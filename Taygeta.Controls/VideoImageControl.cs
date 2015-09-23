using System;
using System.Windows.Forms;
using Declarations;
using Taygeta.Core;

namespace Taygeta.Controls
{
    public partial class VideoImageControl : UserControl
    {
        D3D9Renderer m_render;
        BitmapFormat m_format;
        IMemoryRendererEx m_source;

        public VideoImageControl()
        {
            InitializeComponent();
        }

        protected override void OnResize(EventArgs e)
        {
            if (m_render != null)
            {
                m_render.OnWindowSizeChanged();
            }
            base.OnResize(e);
        }

        private void InitRenderer()
        {
            Invoke((Action)delegate
            {
                m_render = new D3D9Renderer(this.Handle);
            });           
            m_render.RegisterDeviceLostCallback(null, OnDeviceRestored);
            m_render.CreateVideoSurface(m_format.Width, m_format.Height, PixelFormat.YV12, m_source.SAR.Ratio);
        }

        public void Initialize(IMemoryRendererEx rend)
        {
            m_source = rend;
            m_source.SetCallback(OnNewFrame);
            m_source.SetFormatSetupCallback(OnFormatSetup);
        }

        private void OnNewFrame(PlanarFrame pData)
        {
            if (m_render.IsDeviceAvailable)
            {
                m_render.Display(pData.Planes[0], pData.Planes[2], pData.Planes[1], false);
            }
        }

        private BitmapFormat OnFormatSetup(BitmapFormat input)
        {
            m_format = new BitmapFormat(input.Width, input.Height, ChromaType.I420);
            InitRenderer();
            return m_format;
        }

        private void OnDeviceRestored()
        {
            DisposeRenderer();
            InitRenderer();
        }

        private void DisposeRenderer()
        {
            if (m_render != null)
            {
                m_render.Dispose();
                m_render = null;
            }
        }

        public D3D9Renderer RenderTarget
        {
            get
            {
                if (m_render == null)
                {
                    m_render = new D3D9Renderer(this.Handle);
                    m_render.RegisterDeviceLostCallback(null, OnDeviceRestored);
                }
                return m_render;
            }
        }
    }
}
