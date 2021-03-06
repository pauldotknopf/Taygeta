#pragma once

#include "d3d9.h"
#include "Vertex.h"
#include "atlbase.h"
#include "Macros.h"
#include <windows.h>
#include "Overlay.h"
#include "IRenderable.h"
#include "Locks.h"

class D3D9RenderImpl : public IRenderable
{
public:
	D3D9RenderImpl();
	virtual ~D3D9RenderImpl(void);
	void ReleaseInstance();

	HRESULT Initialize(HWND hDisplayWindow, int adapterId = 0);
	HRESULT CheckFormatConversion(D3DFORMAT format);
	HRESULT CreateVideoSurface(int width, int height, D3DFORMAT format = D3DFMT_YV12, double storageAspectRatio = 0);
	HRESULT Display(BYTE* pYplane, BYTE* pVplane, BYTE* pUplane);
	HRESULT DisplayPitch(BYTE* pPlanes[4], int pitches[4]);
	HRESULT DisplaySurface(IDirect3DSurface9* pSurface);

	void SetDisplayMode(FillMode mode);
    FillMode GetDisplayMode();
	HRESULT CheckDevice(void);

	HRESULT SetPixelShader(LPCWSTR pPixelShaderName, LPCSTR entryPoint, LPCSTR shaderModel, LPSTR* ppError);
	HRESULT SetPixelShader(DWORD* buffer);
	HRESULT SetPixelShaderConstant(LPCSTR name, LPVOID value, UINT size);

	HRESULT SetVertexShader(LPCWSTR pVertexShaderName, LPCSTR entryPoint, LPCSTR shaderModel, LPSTR* ppError);
	HRESULT SetVertexShader(DWORD* buffer);
	HRESULT SetVertexShaderConstant(LPCSTR name, LPVOID value, UINT size);
	HRESULT ApplyWorldViewProj(LPCSTR matrixName);

	HRESULT SetVertexShaderMatrix(D3DXMATRIX* matrix, LPCSTR name);
	HRESULT SetPixelShaderMatrix(D3DXMATRIX* matrix, LPCSTR name);
	HRESULT SetVertexShaderVector(D3DXVECTOR4* vector, LPCSTR name);
	HRESULT SetPixelShaderVector(D3DXVECTOR4* vector, LPCSTR name);

	HRESULT ClearPixelShader();
	HRESULT ClearVertexShader();

	HRESULT CaptureDisplayFrame(BYTE* pBuffer, INT* width, INT* height, INT* stride);
	HRESULT CaptureVideoFrame(BYTE* pBuffer, INT* width, INT* height, INT* stride);
	HRESULT GetRenderTargetSurface(IDirect3DSurface9** ppSurface);
	HRESULT GetDevice(IDirect3DDevice9** ppDevice);

	void RegisterDeviceLostCallback(DEVICECALLBACK pCallback, DEVICECALLBACK pResetCallback)
	{
		m_pLostCallback = pCallback;
		m_pResetCallback = pResetCallback;
	}

	HRESULT OnWindowSizeChanged();
	HRESULT Repaint();

private:
	CComPtr<IDirect3D9Ex>           m_pD3D9;
	CComPtr<IDirect3DDevice9Ex>     m_pDevice;
	CComPtr<IDirect3DSurface9>      m_pOffsceenSurface;
	CComPtr<IDirect3DSurface9>      m_pTextureSurface;
	CComPtr<IDirect3DTexture9>      m_pTexture;
	CComPtr<IDirect3DVertexBuffer9> m_pVertexBuffer;
	CComPtr<IDirect3DVertexShader9> m_pVertexShader; 
	CComPtr<ID3DXConstantTable>     m_pVertexConstantTable; 
	CComPtr<ID3DXConstantTable>     m_pPixelConstantTable; 
	CComPtr<IDirect3DPixelShader9>  m_pPixelShader;
	CComPtr<IDirect3DSurface9>      m_pRenderTarget;
	CComPtr<IDirect3DSurface9>      m_pLastFrame;

	D3DDISPLAYMODE m_displayMode;
	HWND m_hDisplayWindow;
	int m_videoWidth;
	int m_videoHeight;
	D3DFORMAT m_format;
	OverlayStore m_overlays;
	FillMode m_fillMode;
	D3DPRESENT_PARAMETERS m_presentParams;
	bool m_renderModeIsWindowed;
	volatile bool m_deviceIsLost;
	DEVICECALLBACK m_pLostCallback;
	DEVICECALLBACK m_pResetCallback;
	CCritSec m_locker;
	RECT m_targetRect;
	int m_adapterId;
	bool m_showOverlays;
	double m_aspectRatio;

private:
	HRESULT SetupMatrices(int width, int height);
	HRESULT CreateScene(void);
	HRESULT FillBuffer(BYTE* pY, BYTE* pV, BYTE* pU);
	HRESULT FillBuffer(BYTE* pPlanes[4], int pitches[4]);
	HRESULT CreateRenderTarget(int width, int height);
	HRESULT Present(void);	
	HRESULT GetPresentParams(D3DPRESENT_PARAMETERS* params, BOOL bWindowed);
	void CreateDummyWindow();
	HRESULT DiscardResources();
	HRESULT CreateResources(int width, int height);
	void UpdateDisplayRect();
	HRESULT StretchSurface();
	HRESULT CopySurface(IDirect3DSurface9* pSurface);
	
public:
	HRESULT DrawLine(     SHORT key, POINT p1, POINT p2, FLOAT width = 2, D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255), BYTE opacity = 255);
	HRESULT DrawRectangle(SHORT key, RECT rectangle, FLOAT width, D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255), BYTE opacity = 255);
	HRESULT DrawPolygon  (SHORT key, POINT* points, INT pointsLen, FLOAT width, D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255), BYTE opacity = 255);
	HRESULT DrawText(     SHORT key, LPCWSTR text, RECT pos, INT size = 20, D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255), LPCWSTR font = L"Ariel", BYTE opacity = 255);
	HRESULT DrawBitmap(   SHORT key, RECT rectangle, LPCWSTR path, D3DCOLOR colorKey = D3DCOLOR_XRGB(255, 255, 255), BYTE opacity = 255);
	HRESULT DrawBitmap(   SHORT key, RECT rectangle, BYTE* pixelData, UINT stride, INT width, INT height, D3DCOLOR colorKey = D3DCOLOR_XRGB(255, 255, 255), BYTE opacity = 255);
	virtual HRESULT RemoveOverlay(SHORT key);
	virtual HRESULT RemoveAllOverlays();
	virtual void SetShowOverlays(bool bShow);
	virtual bool GetShowOverlays();

public:
	HRESULT AddOverlayStream(SHORT key, int videoWidth, int videoHeight, D3DFORMAT videoFormat, RECT targetRect, BYTE opacity);
	HRESULT UpdateOverlayFrame(SHORT key, BYTE* pYplane, BYTE* pVplane, BYTE* pUplane);
	HRESULT UpdateOverlayOpacity(SHORT key, BYTE opacity);
	HRESULT Clear(D3DCOLOR color);
};

