#pragma once

#include "d3d9.h"
#include "d3dx9.h"
#include "Vertex.h"
#include "atlbase.h"
#include "Macros.h"
#include <windows.h>
#include <vector>
#include <iostream>
#include "MMsystem.h"

#if defined(LIBRARY_EXPORT)
#   define EXPORTLIB  __declspec(dllexport)
#else 
#   define EXPORTLIB  __declspec(dllimport)
#endif 

#ifdef __cplusplus

using namespace std;
enum FillMode
{
    KeepAspectRatio = 0,
    Fill = 1
};

struct DeviceDetails
{
	WCHAR DeviceID[256];                
	WCHAR DisplayName[256]; 
	int Index;
};

typedef void (__stdcall *DEVICECALLBACK)(void);

struct IDisplable
{
	virtual HRESULT Initialize(HWND hDisplayWindow, int adapterId) PURE;
	virtual HRESULT CheckFormatConversion(D3DFORMAT format) PURE;
	virtual HRESULT CreateVideoSurface(int width, int height, D3DFORMAT format = D3DFMT_YV12, double storageAspectRatio = 0) PURE;
	virtual HRESULT Display(BYTE* pYplane, BYTE* pVplane, BYTE* pUplane) PURE;
	virtual HRESULT DisplayPitch(BYTE* pPlanes[4], int pitches[4]) PURE;
	virtual HRESULT DisplaySurface(IDirect3DSurface9* pSurface) PURE;
	virtual HRESULT CaptureDisplayFrame(BYTE* pBuffer, INT* width, INT* height, INT* stride) PURE;
	virtual HRESULT CaptureVideoFrame(BYTE* pBuffer, INT* width, INT* height, INT* stride) PURE;
	virtual void SetDisplayMode(FillMode mode) PURE;
    virtual FillMode GetDisplayMode() PURE;
	virtual HRESULT GetRenderTargetSurface(IDirect3DSurface9** ppSurface) PURE;
	virtual HRESULT GetDevice(IDirect3DDevice9** ppDevice) PURE;
	virtual void RegisterDeviceLostCallback(DEVICECALLBACK pLostCallback, DEVICECALLBACK pResetCallback) PURE;
	virtual HRESULT Clear(D3DCOLOR color) PURE;
	virtual HRESULT OnWindowSizeChanged() PURE;
	virtual void ReleaseInstance() PURE;
	virtual HRESULT Repaint() PURE;
};

struct IShaderable
{
	virtual HRESULT SetPixelShader(LPCWSTR pPixelShaderName, LPCSTR entryPoint, LPCSTR shaderModel, LPSTR* ppError) PURE;
	virtual HRESULT SetPixelShader(DWORD* buffer) PURE;
	virtual HRESULT SetPixelShaderConstant(LPCSTR name, LPVOID value, UINT size) PURE;

	virtual HRESULT SetVertexShader(LPCWSTR pVertexShaderName, LPCSTR entryPoint, LPCSTR shaderModel, LPSTR* ppError) PURE;
	virtual HRESULT SetVertexShader(DWORD* buffer) PURE;
	virtual HRESULT SetVertexShaderConstant(LPCSTR name, LPVOID value, UINT size) PURE;
	virtual HRESULT ApplyWorldViewProj(LPCSTR matrixName) PURE;

	virtual HRESULT SetVertexShaderMatrix(D3DXMATRIX* matrix, LPCSTR name) PURE;
	virtual HRESULT SetPixelShaderMatrix(D3DXMATRIX* matrix, LPCSTR name) PURE;
	virtual HRESULT SetVertexShaderVector(D3DXVECTOR4* vector, LPCSTR name) PURE;
	virtual HRESULT SetPixelShaderVector(D3DXVECTOR4* vector, LPCSTR name) PURE;

	virtual HRESULT ClearPixelShader() PURE;
	virtual HRESULT ClearVertexShader() PURE;
};

struct IOverlable
{
	virtual HRESULT DrawLine(SHORT key, POINT p1, POINT p2, FLOAT width, D3DCOLOR color, BYTE opacity) PURE;
	virtual HRESULT DrawRectangle(SHORT key, RECT rectangle, FLOAT width, D3DCOLOR color, BYTE opacity) PURE;
	virtual HRESULT DrawPolygon(SHORT key, POINT* points, INT pointsLen, FLOAT width, D3DCOLOR color, BYTE opacity ) PURE;
	virtual HRESULT DrawText(SHORT key, LPCWSTR text, RECT pos, INT size, D3DCOLOR color, LPCWSTR font, BYTE opacity) PURE;
	virtual HRESULT DrawBitmap(SHORT key, RECT rectangle, LPCWSTR path, D3DCOLOR colorKey, BYTE opacity) PURE;
	virtual HRESULT DrawBitmap(SHORT key, RECT rectangle, BYTE* pixelData, UINT stride, INT width, INT height, D3DCOLOR colorKey, BYTE opacity) PURE;	
	virtual HRESULT RemoveOverlay(SHORT key) PURE;
	virtual HRESULT RemoveAllOverlays() PURE;
	virtual void SetShowOverlays(bool bShow) PURE;
	virtual bool GetShowOverlays() PURE;
};

struct IVideoMixer
{
	virtual HRESULT AddOverlayStream(SHORT key, int videoWidth, int videoHeight, D3DFORMAT videoFormat, RECT targetRect, BYTE opacity) PURE;
	virtual HRESULT UpdateOverlayFrame(SHORT key, BYTE* pYplane, BYTE* pVplane, BYTE* pUplane) PURE;
	virtual HRESULT UpdateOverlayOpacity(SHORT key, BYTE opacity) PURE;
};

struct IRenderable : public IDisplable, public IShaderable, public IOverlable, public IVideoMixer
{
};

struct ISoundPlayer
{
	virtual HRESULT Initialize(const WAVEFORMATEX* format, int audioOutputDeviceIndex = 0, bool isMusicPlayback = false) PURE;
	virtual HRESULT QueueSamples(BYTE* buffer, UINT32 size) PURE;
	virtual HRESULT PlayQueuedSamples() PURE;
	virtual HRESULT PlaySamples(BYTE* buffer, UINT32 size) PURE;
	virtual float GetVolume() PURE;
	virtual HRESULT SetVolume(float volume) PURE;
	virtual void GetChannelVolumes(int channels, float* volumes) PURE;
	virtual HRESULT SetChannelVolumes(int channels, const float* volumes) PURE;
	virtual vector<DeviceDetails*>& GetAudioDevices() PURE;
	virtual void ReleaseInstance() PURE;
	virtual HRESULT FlushBuffers() PURE;
};

extern "C" EXPORTLIB IRenderable*  APIENTRY GetImplementation();
extern "C" EXPORTLIB ISoundPlayer* APIENTRY GetSoundImplementation();

#endif // __cplusplus