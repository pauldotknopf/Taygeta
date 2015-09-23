using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Threading;
using Declarations;
using Taygeta.Core;
using System.Diagnostics;

namespace Taygeta.Controls
{
    public class VideoImageSource : Image
    {
        private D3D9Renderer m_render;
        private D3DImage m_image;
        private BitmapFormat m_format;
        private IMemoryRendererEx m_source;
        
        public VideoImageSource()
        {
            m_image = new D3DImage();
            m_image.IsFrontBufferAvailableChanged += new DependencyPropertyChangedEventHandler(m_image_IsFrontBufferAvailableChanged);
            Source = m_image;
        }

        void m_image_IsFrontBufferAvailableChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (!m_image.IsFrontBufferAvailable)
            {
                DiscardRenderer();
            }
            else
            {
                SetSurface(m_format);
            }
        }

        private void DiscardRenderer()
        {
            if (m_render != null)
            {
                m_render.Dispose();
                m_render = null;
            }
        }

        private void CreateRenderer()
        {
            DiscardRenderer();
            m_render = new D3D9Renderer(IntPtr.Zero);
            m_render.DisplayMode = VideoDisplayMode.KeepAspectRatio;
        }

        public void Initialize(IMemoryRendererEx renderer)
        {
            m_source = renderer;
            m_source.SetFormatSetupCallback(OnFormatSetup);
            m_source.SetCallback(OnNewFrame);
        }

        private BitmapFormat OnFormatSetup(BitmapFormat format)
        {
            m_format = format;
            SetSurface(format);
            return new BitmapFormat(format.Width, format.Height, ChromaType.I420);
        }

        private void SetSurface(BitmapFormat format)
        {
            CreateRenderer();
            m_render.CreateVideoSurface(format.Width, format.Height, PixelFormat.YV12, m_source.SAR.Ratio);
            m_image.Dispatcher.Invoke(new Action(delegate
            {
                m_image.Lock();
                m_image.SetBackBuffer(D3DResourceType.IDirect3DSurface9, m_render.RenderTargetSurface);
                m_image.Unlock();
            }), DispatcherPriority.Send);   
        }

        private void OnNewFrame(PlanarFrame frame)
        {
            m_image.Dispatcher.Invoke(new Action(delegate
            {
                if (!m_image.IsFrontBufferAvailable)
                {
                    return;
                }

                m_image.Lock();
                m_render.Display(frame.Planes[0], frame.Planes[2], frame.Planes[1], false);
                m_image.AddDirtyRect(new Int32Rect(0, 0, m_image.PixelWidth, m_image.PixelHeight));
                m_image.Unlock();
            }), DispatcherPriority.Send);  
        }

        public D3D9Renderer RenderTarget
        {
            get
            {
                return m_render;
            }
        }

        public void Clear()
        {
            m_image.Dispatcher.BeginInvoke(new Action(delegate
            {
                m_image.Lock();
                m_render.Clear(System.Drawing.Color.Black);
                m_image.SetBackBuffer(D3DResourceType.IDirect3DSurface9, IntPtr.Zero);
                m_image.Unlock();
            }), DispatcherPriority.Send);  
        }
    }
}
