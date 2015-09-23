#pragma once

#include "XAudio2.h"
#include <atlbase.h>
#include "XA2Player.h"
#include <deque>
#include "IRenderable.h"

using namespace std;

class XA2Player : public ISoundPlayer
{
public:
	XA2Player(void);
	virtual ~XA2Player(void);
	HRESULT Initialize(const WAVEFORMATEX* format, int audioOutputDeviceIndex = 0, bool isMusicPlayback = false);
	void ReleaseInstance();
	HRESULT QueueSamples(BYTE* buffer, UINT32 size);
	HRESULT PlayQueuedSamples();
	HRESULT PlaySamples(BYTE* buffer, UINT32 size);
	float GetVolume();
	HRESULT SetVolume(float volume);
	void GetChannelVolumes(int channels, float* volumes);
	HRESULT SetChannelVolumes(int channels, const float* volumes);
	HRESULT FlushBuffers();
	vector<DeviceDetails*>& GetAudioDevices()
	{
		return m_devices;
	}

private:
	CComPtr<IXAudio2> m_xaudio2;
	IXAudio2MasteringVoice* m_masteringVoice;
	IXAudio2SourceVoice* m_soundVoice;
	WAVEFORMATEX m_format;
	
	deque<XAUDIO2_BUFFER*> m_playQueue;      
    deque<XAUDIO2_BUFFER*> m_releaseQueue; 
	vector<DeviceDetails*> m_devices;
};


