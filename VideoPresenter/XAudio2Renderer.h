#pragma once

#include "XA2Player.h"
#include "WaveFormatEx.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Runtime::InteropServices;

namespace Taygeta { namespace Core 
{	
	/// <summary>
    /// XAudio2 sound rendering implementation class
    /// </summary>
    public ref class XAudio2Renderer
	{
	public:
		XAudio2Renderer(WaveFormatEx^ format, bool isMusicPlayback, int audioOutputDeviceIndex)
		{
			m_impl = new XA2Player();
			WAVEFORMATEX frm = {0};
			frm.wFormatTag      = format->FormatTag;
			frm.nChannels       = format->Channels;
			frm.nSamplesPerSec  = format->SamplesPerSec;
			frm.nAvgBytesPerSec = format->AvgBytesPerSec;
			frm.nBlockAlign     = format->BlockAlign;
			frm.wBitsPerSample  = format->BitsPerSample;
			frm.cbSize          = format->Size;

			HRESULT hr = m_impl->Initialize(&frm, audioOutputDeviceIndex, isMusicPlayback);
			Marshal::ThrowExceptionForHR(hr);
		}

		virtual ~XAudio2Renderer()
		{
			if(m_impl)
			{
				delete m_impl;
				m_impl = NULL;
			}
		}

		!XAudio2Renderer()
		{
			if(m_impl)
			{
				delete m_impl;
				m_impl = NULL;
			}
		}

		void QueueSamples(IntPtr pBuffer, UInt32 size)
		{
			HRESULT hr = m_impl->QueueSamples((BYTE*)pBuffer.ToPointer(), size);
			Marshal::ThrowExceptionForHR(hr);
		}

		void PlayQueuedSamples()
		{
			HRESULT hr = m_impl->PlayQueuedSamples();
			Marshal::ThrowExceptionForHR(hr);
		}

		void PlaySamples(IntPtr pBuffer, UInt32 size)
		{
			HRESULT hr = m_impl->PlaySamples((BYTE*)pBuffer.ToPointer(), size);
			Marshal::ThrowExceptionForHR(hr);
		}

		void FlushBuffers()
		{
			HRESULT hr = m_impl->FlushBuffers();
			Marshal::ThrowExceptionForHR(hr);
		}

		property float MasterVolume
		{
		   float get()
		   {
		  	   return m_impl->GetVolume();
		   }
		   void set(float value)
		   {
			   HRESULT hr = m_impl->SetVolume(value);
			   Marshal::ThrowExceptionForHR(hr);
		   }
		}

		array<float>^ GetChannelVolumes(int channels)
		{
		   array<float>^ arr = gcnew array<float>(channels);
		   pin_ptr<float> pBuffer = &arr[0];
		   m_impl->GetChannelVolumes(channels, pBuffer);
		   return arr;
		}

		void SetChannelVolumes(array<float>^ volumes)
		{
			int size = volumes->Length;
			pin_ptr<float> pBuffer = &volumes[0];
			HRESULT hr = m_impl->SetChannelVolumes(size, pBuffer);
			Marshal::ThrowExceptionForHR(hr);
		}

		static array<AudioDevice^>^ EnumDevices()
		{
			System::Collections::Generic::List<AudioDevice^>^ list = gcnew System::Collections::Generic::List<AudioDevice^>();
			XA2Player player;
			vector<DeviceDetails*>::iterator i;
			vector<DeviceDetails*> devices = player.GetAudioDevices();
			for(i = devices.begin(); i != devices.end(); i++)
			{
				AudioDevice^ dev = gcnew AudioDevice();
				dev->DeviceID = gcnew String((*i)->DeviceID);
				dev->DisplayName = gcnew String((*i)->DisplayName);
				dev->Index = (*i)->Index;
				list->Add(dev);
			}

			return list->ToArray();
		}

	private:
		XA2Player* m_impl;	
	};
}
}
		