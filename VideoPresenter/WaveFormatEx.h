#pragma once

using namespace System;

namespace Taygeta { namespace Core 
{
	public ref struct WaveFormatEx
	{
		UInt16 FormatTag;      
		UInt16 Channels;       
		UInt32 SamplesPerSec;  
		UInt32 AvgBytesPerSec; 
		UInt16 BlockAlign;     
		UInt16 BitsPerSample;  
		UInt16 Size; 

		WaveFormatEx()
		{
			FormatTag = 1; // PCM
			Size = 0;
		}

		WaveFormatEx(UInt16 channels, UInt32 samplesPerSecond, UInt32 bitsPerSample)
		{
			FormatTag = 1;
			Size = 0;
			Channels = channels;
			SamplesPerSec = samplesPerSecond;
			BitsPerSample = bitsPerSample;
			BlockAlign = bitsPerSample / 8 * Channels;
			AvgBytesPerSec = SamplesPerSec * BlockAlign;
		}
	};

	public ref class AudioDevice
	{
	public:
		String^ DeviceID;                
		String^ DisplayName; 
		int Index;
	};
}}

