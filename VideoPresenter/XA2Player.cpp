#include "StdAfx.h"
#include "XA2Player.h"

XA2Player::XA2Player(void)
	: m_soundVoice(NULL)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	HRESULT hr = XAudio2Create( &m_xaudio2, 0) ;

	UINT32 deviceCount;
	hr = m_xaudio2->GetDeviceCount(&deviceCount);
 
	XAUDIO2_DEVICE_DETAILS deviceDetails;
	for (unsigned int i = 0; i < deviceCount; i++)
	{
		m_xaudio2->GetDeviceDetails(i, &deviceDetails);	
		DeviceDetails* dev = new DeviceDetails();
		wmemcpy(dev->DeviceID, deviceDetails.DeviceID, 256);
		wmemcpy(dev->DisplayName, deviceDetails.DisplayName, 256);
		dev->Index = i;
		m_devices.push_back(dev);
	}
}

void XA2Player::ReleaseInstance()
{
	delete this;
}

XA2Player::~XA2Player(void)
{
	if(m_soundVoice)
	{
		m_soundVoice->Stop();
	}
	m_xaudio2.Release();
}

HRESULT XA2Player::Initialize(const WAVEFORMATEX* format, int audioOutputDeviceIndex, bool isMusicPlayback)
{
	memcpy(&m_format, format, sizeof(WAVEFORMATEX));
	UINT32 flags = isMusicPlayback == true ? XAUDIO2_VOICE_MUSIC : 0;
	HRESULT hr = m_xaudio2->CreateMasteringVoice(&m_masteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, audioOutputDeviceIndex);
	hr = m_xaudio2->CreateSourceVoice(&m_soundVoice, format, flags, XAUDIO2_DEFAULT_FREQ_RATIO);
	return m_soundVoice->Start(0);
}


HRESULT XA2Player::QueueSamples(BYTE* buffer, UINT32 size)
{
	if(!buffer)
	{
		return E_POINTER;
	}

	XAUDIO2_BUFFER* buf = new XAUDIO2_BUFFER();
	buf->pAudioData = new BYTE[size];
	memcpy((void*)buf->pAudioData, buffer, size);
	buf->AudioBytes = size;
	m_playQueue.push_back(buf);

	return S_OK;
}

HRESULT XA2Player::PlayQueuedSamples()
{
	XAUDIO2_VOICE_STATE VoiceState;
    m_soundVoice->GetState(&VoiceState);

	while (m_releaseQueue.size() > VoiceState.BuffersQueued) 
	{
        XAUDIO2_BUFFER* tmp = m_releaseQueue.front();
        m_releaseQueue.pop_front();
        if(tmp != NULL)
		{
			delete tmp->pAudioData;
			delete tmp;  
		}          
    }

	while (!m_playQueue.empty())
	{
		XAUDIO2_BUFFER* tmp = m_playQueue.front();
        m_releaseQueue.push_back(tmp);
        m_soundVoice->SubmitSourceBuffer(tmp);
        m_playQueue.pop_front();
		VoiceState.BuffersQueued++;
    }                
		
	return S_OK;
}

HRESULT XA2Player::PlaySamples(BYTE* buffer, UINT32 size)
{
	XAUDIO2_BUFFER tmp = {0};
	tmp.pAudioData = buffer;
	tmp.AudioBytes = size;

	return m_soundVoice->SubmitSourceBuffer(&tmp);
}

float XA2Player::GetVolume()
{
	float vol;
	m_soundVoice->GetVolume(&vol);
	return vol;
}
	
HRESULT XA2Player::SetVolume(float volume)
{
	return m_soundVoice->SetVolume(volume);
}

void XA2Player::GetChannelVolumes(int channels, float* volumes)
{
	m_soundVoice->GetChannelVolumes(channels, volumes);
}
	
HRESULT XA2Player::SetChannelVolumes(int channels, const float* volumes)
{
	return m_soundVoice->SetChannelVolumes(channels, volumes);
}

HRESULT XA2Player::FlushBuffers()
{
	while (!m_releaseQueue.empty()) 
	{
        XAUDIO2_BUFFER* tmp = m_releaseQueue.front();
        m_releaseQueue.pop_front();
        if(tmp != NULL)
		{
			delete tmp->pAudioData;
			delete tmp;  
		}        
    }

	while (!m_playQueue.empty())
	{
		XAUDIO2_BUFFER* tmp = m_playQueue.front();
		m_playQueue.pop_front();
		if(tmp != NULL)
		{
			delete tmp->pAudioData;
			delete tmp;  
		}
    }   

	return m_soundVoice->FlushSourceBuffers();
}
