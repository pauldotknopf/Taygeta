#include "StdAfx.h"
#include "D3D9RenderImpl.h"

D3D9RenderImpl::D3D9RenderImpl()
    : m_pVertexShader(0), m_pPixelShader(0), m_pD3D9(0), m_format(D3DFMT_UNKNOWN), m_pVertexConstantTable(0), 
	  m_pPixelConstantTable(0), m_fillMode(KeepAspectRatio), m_deviceIsLost(false), 
	  m_pResetCallback(NULL), m_pLostCallback(NULL), m_adapterId(0), m_showOverlays(true), m_aspectRatio(0)
{

}

static inline void ApplyLetterBoxing(RECT& rendertTargetArea, FLOAT frameWidth, FLOAT frameHeight, double aspectRatio)
{
    float ratio = frameWidth / frameHeight;
	if(aspectRatio > 0)
	{
		ratio *= aspectRatio;
	}
    const float targetW = fabs((FLOAT)(rendertTargetArea.right - rendertTargetArea.left));
    const float targetH = fabs((FLOAT)(rendertTargetArea.bottom - rendertTargetArea.top));
    float tempH = targetW / ratio;              
    if(tempH <= targetH)
    {               
        float deltaH = fabs(tempH - targetH) / 2;
        rendertTargetArea.top += deltaH;
        rendertTargetArea.bottom -= deltaH;
    }
    else
    {
        float tempW = targetH * ratio;    
        float deltaW = fabs(tempW - targetW) / 2;
        rendertTargetArea.left += deltaW;
        rendertTargetArea.right -= deltaW;
    }
}

HRESULT D3D9RenderImpl::Initialize(HWND hDisplayWindow, int adapterId)
{
	if(hDisplayWindow != NULL && IsWindow(hDisplayWindow))
	{
		m_hDisplayWindow = hDisplayWindow;
		m_renderModeIsWindowed = true;
	}
	else
	{
		CreateDummyWindow();
		m_renderModeIsWindowed = false;
	}
	m_adapterId = adapterId;
    HR(Direct3DCreate9Ex( D3D_SDK_VERSION, &m_pD3D9 ));   
	HR(m_pD3D9->GetAdapterDisplayMode(adapterId, &m_displayMode));

    D3DCAPS9 deviceCaps;
    HR(m_pD3D9->GetDeviceCaps(adapterId, D3DDEVTYPE_HAL, &deviceCaps));
	DWORD dwBehaviorFlags = D3DCREATE_MULTITHREADED;
	if(!m_renderModeIsWindowed)
	{
		dwBehaviorFlags |= D3DCREATE_FPU_PRESERVE;
	}

    if( deviceCaps.VertexProcessingCaps != 0 )
        dwBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        dwBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    
    HR(GetPresentParams(&m_presentParams, TRUE));   
    HR(m_pD3D9->CreateDeviceEx(adapterId, D3DDEVTYPE_HAL, m_hDisplayWindow, dwBehaviorFlags, &m_presentParams, NULL, &m_pDevice));
	HR(m_pDevice->GetRenderTarget(0, &m_pRenderTarget));   
	if(!m_renderModeIsWindowed)
	{
		return S_OK;
	}

    return CreateResources(m_presentParams.BackBufferWidth, m_presentParams.BackBufferHeight);
}

D3D9RenderImpl::~D3D9RenderImpl(void)
{
	DiscardResources();
	m_overlays.RemoveAll();
	if(!m_renderModeIsWindowed)
	{
		DestroyWindow(m_hDisplayWindow);
	}
	SafeRelease(m_pOffsceenSurface); 
	SafeRelease(m_pLastFrame);
	SafeRelease(m_pVertexShader); 
    SafeRelease(m_pVertexConstantTable); 
    SafeRelease(m_pPixelConstantTable); 
    SafeRelease(m_pPixelShader);
	SafeRelease(m_pRenderTarget);
	SafeRelease(m_pD3D9);
    SafeRelease(m_pDevice);   
}

void D3D9RenderImpl::ReleaseInstance()
{
	delete this;
}

HRESULT D3D9RenderImpl::CheckFormatConversion(D3DFORMAT format)
{
    HR(m_pD3D9->CheckDeviceFormat(m_adapterId, D3DDEVTYPE_HAL, m_displayMode.Format, 0, D3DRTYPE_SURFACE, format));
    return m_pD3D9->CheckDeviceFormatConversion(m_adapterId, D3DDEVTYPE_HAL, format, m_displayMode.Format);
}


HRESULT D3D9RenderImpl::CreateRenderTarget(int width, int height)
{
    HR(m_pDevice->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, m_displayMode.Format, D3DPOOL_DEFAULT, &m_pTexture, NULL));
    HR(m_pTexture->GetSurfaceLevel(0, &m_pTextureSurface));
    HR(m_pDevice->CreateVertexBuffer(sizeof(VERTEX) * 4, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &m_pVertexBuffer, NULL));
    
    VERTEX vertexArray[] =
    {
        { D3DXVECTOR3(0, 0, 0),          D3DCOLOR_ARGB(255, 255, 255, 255), D3DXVECTOR2(0, 0) },  // top left
        { D3DXVECTOR3(width, 0, 0),      D3DCOLOR_ARGB(255, 255, 255, 255), D3DXVECTOR2(1, 0) },  // top right
        { D3DXVECTOR3(width, height, 0), D3DCOLOR_ARGB(255, 255, 255, 255), D3DXVECTOR2(1, 1) },  // bottom right
        { D3DXVECTOR3(0, height, 0),     D3DCOLOR_ARGB(255, 255, 255, 255), D3DXVECTOR2(0, 1) },  // bottom left
    };

    VERTEX *vertices;
    HR(m_pVertexBuffer->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD));
    memcpy(vertices, vertexArray, sizeof(vertexArray));
    return m_pVertexBuffer->Unlock();
}

HRESULT D3D9RenderImpl::Display(BYTE* pYplane, BYTE* pVplane, BYTE* pUplane)
{
	CAutoLock lock(&m_locker);
    if(!pYplane)
    {
        return E_POINTER;
    }

    if(m_format == D3DFMT_NV12 && !pVplane)
    {
        return E_POINTER;
    }

    if(m_format == D3DFMT_YV12 && (!pVplane || !pUplane))
    {
        return E_POINTER;
    }

    HR(FillBuffer(pYplane, pVplane, pUplane));
	HR(StretchSurface());
    HR(CreateScene());
	if(m_renderModeIsWindowed)
	{
		return Present();
	}

	return S_OK;
}

HRESULT D3D9RenderImpl::DisplayPitch(BYTE* pPlanes[4], int pitches[4])
{
	CAutoLock lock(&m_locker);
	HR(FillBuffer(pPlanes, pitches));
	HR(StretchSurface());
	HR(CreateScene());
	if(m_renderModeIsWindowed)
	{
		return Present();
	}

	return S_OK;
}

HRESULT D3D9RenderImpl::StretchSurface()
{
	HR(m_pDevice->ColorFill(m_pTextureSurface, NULL, D3DCOLOR_ARGB(0xFF, 0, 0, 0)));
	if(m_pLastFrame != NULL)
	{
		return m_pDevice->StretchRect(m_pLastFrame, NULL, m_pTextureSurface, &m_targetRect, D3DTEXF_LINEAR);
	}

	if(!m_renderModeIsWindowed)
	{
		return m_pDevice->StretchRect(m_pOffsceenSurface, NULL, m_pTextureSurface, NULL, D3DTEXF_LINEAR);
	}

	return m_pDevice->StretchRect(m_pOffsceenSurface, NULL, m_pTextureSurface, &m_targetRect, D3DTEXF_LINEAR);
}

HRESULT D3D9RenderImpl::SetupMatrices(int width, int height)
{
    D3DXMATRIX matOrtho; 
    D3DXMATRIX matIdentity;

    D3DXMatrixOrthoOffCenterLH(&matOrtho, 0, width, height, 0, 0.0f, 1.0f);
    D3DXMatrixIdentity(&matIdentity);
    HR(m_pDevice->SetTransform(D3DTS_PROJECTION, &matOrtho));
    HR(m_pDevice->SetTransform(D3DTS_WORLD, &matIdentity));
    return m_pDevice->SetTransform(D3DTS_VIEW, &matIdentity);
}

HRESULT D3D9RenderImpl::CreateScene(void)
{
	if(NULL == m_pDevice)
	{
		return E_POINTER;
	}

    HRESULT hr = m_pDevice->Clear(m_adapterId, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    HR(m_pDevice->BeginScene());
	hr = m_pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    if(FAILED(hr))
    {
        m_pDevice->EndScene();
        return hr;
    }
    hr = m_pDevice->SetVertexShader(m_pVertexShader);
    if(FAILED(hr))
    {
        m_pDevice->EndScene();
        return hr;
    }   
    hr = m_pDevice->SetPixelShader(m_pPixelShader);
    if(FAILED(hr))
    {
        m_pDevice->EndScene();
        return hr;
    }
    hr = m_pDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(VERTEX));
    if(FAILED(hr))
    {
        m_pDevice->EndScene();
        return hr;
    }
    hr = m_pDevice->SetTexture(0, m_pTexture);
    if(FAILED(hr))
    {
        m_pDevice->EndScene();
        return hr;
    }
    hr = m_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
    if(FAILED(hr))
    {
        m_pDevice->EndScene();
        return hr;
    }
	if(m_showOverlays)
		m_overlays.Draw();

    return m_pDevice->EndScene();
}

HRESULT D3D9RenderImpl::OnWindowSizeChanged()
{
	CAutoLock lock(&m_locker);

	DiscardResources();
	D3DPRESENT_PARAMETERS pp;
	HR(GetPresentParams(&pp, TRUE));
	HR(m_pDevice->ResetEx(&pp, NULL));
	UpdateDisplayRect();
	HR(m_pDevice->GetRenderTarget(0, &m_pRenderTarget));
	return CreateResources(pp.BackBufferWidth, pp.BackBufferHeight);
}

HRESULT D3D9RenderImpl::CheckDevice(void)
{
	if(NULL == m_pDevice)
	{
		return E_POINTER;
	}
    HRESULT hr = m_pDevice->CheckDeviceState (m_hDisplayWindow);
	if(hr == D3DERR_DEVICELOST)
	{
		if(NULL != m_pLostCallback && !m_deviceIsLost)
		{
			(*m_pLostCallback)();
		}		
		m_deviceIsLost = true;
	}
	else if(hr == D3DERR_DEVICENOTRESET)
	{
		if(NULL != m_pResetCallback && m_deviceIsLost)
		{
			(*m_pResetCallback)();
		}		
		m_deviceIsLost = false;
	}
	return hr;
}

HRESULT D3D9RenderImpl::DiscardResources()
{
	SafeRelease(m_pRenderTarget);
    SafeRelease(m_pTextureSurface);
    SafeRelease(m_pTexture);
    SafeRelease(m_pVertexBuffer);
    return S_OK;
}

HRESULT D3D9RenderImpl::CreateResources(int width, int height)
{
    m_pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);
    m_pDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE);
    m_pDevice->SetRenderState( D3DRS_LIGHTING, FALSE);
    m_pDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE);
	m_pDevice->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    m_pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_SPECULAR);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	if(!m_renderModeIsWindowed && m_aspectRatio > 0)
	{
		width *= m_aspectRatio;
	}

    HR(CreateRenderTarget(width, height));   
    return SetupMatrices(width, height);
}

HRESULT D3D9RenderImpl::FillBuffer(BYTE* pPlanes[4], int pitches[4])
{
	if(NULL == m_pOffsceenSurface)
	{
		return E_POINTER;
	}
	D3DLOCKED_RECT d3drect;
	HR(m_pOffsceenSurface->LockRect(&d3drect, NULL, 0));
	int newHeight  = m_videoHeight;
	int newWidth  = m_videoWidth;
	BYTE* pict = (BYTE*)d3drect.pBits;
	BYTE* Y = pPlanes[0];
	BYTE* V = pPlanes[1];
	BYTE* U = pPlanes[2];
	switch(m_format)
	{
	case D3DFMT_YV12:
		for (int y = 0 ; y < newHeight ; y++)
		{
			memcpy(pict, Y, newWidth);
			pict += d3drect.Pitch;
			Y += pitches[0];
		}
		for (int y = 0 ; y < newHeight >> 1 ; y++)
		{
			memcpy(pict, V, newWidth >> 1);
			pict += d3drect.Pitch >> 1;
			V += pitches[1];
		}
		for (int y = 0 ; y < newHeight >> 1; y++)
		{
			memcpy(pict, U, newWidth >> 1);
			pict += d3drect.Pitch >> 1;
			U += pitches[2];
		}	
		break;
	case D3DFMT_NV12:
		for (int y = 0 ; y < newHeight ; y++)
		{
			memcpy(pict, Y, newWidth);
			pict += d3drect.Pitch;
			Y += pitches[0];
		}
		for (int y = 0 ; y < newHeight >> 1 ; y++)
		{
			memcpy(pict, V, newWidth);
			pict += d3drect.Pitch;
			V += pitches[1];
		}
		break;
	case D3DFMT_YUY2:
	case D3DFMT_UYVY:
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		memcpy(pict, Y, d3drect.Pitch * newHeight);
		break;
	}
	return m_pOffsceenSurface->UnlockRect();
}

HRESULT D3D9RenderImpl::FillBuffer(BYTE* pY, BYTE* pV, BYTE* pU)
{
	if(NULL == m_pOffsceenSurface)
	{
		return E_POINTER;
	}
    D3DLOCKED_RECT d3drect;
    HR(m_pOffsceenSurface->LockRect(&d3drect, NULL, 0));
    int newHeight  = m_videoHeight;
    int newWidth  = m_videoWidth;
    BYTE* pict = (BYTE*)d3drect.pBits;
    BYTE* Y = pY;
    BYTE* V = pV;
    BYTE* U = pU;
    switch(m_format)
    {
        case D3DFMT_YV12:
            for (int y = 0 ; y < newHeight ; y++)
            {
                memcpy(pict, Y, newWidth);
                pict += d3drect.Pitch;
                Y += newWidth;
            }
            for (int y = 0 ; y < newHeight >> 1 ; y++)
            {
                memcpy(pict, V, newWidth >> 1);
                pict += d3drect.Pitch >> 1;
                V += newWidth >> 1;
            }
            for (int y = 0 ; y < newHeight >> 1; y++)
            {
                memcpy(pict, U, newWidth >> 1);
                pict += d3drect.Pitch >> 1;
                U += newWidth >> 1;
            }	
            break;
        case D3DFMT_NV12:           
            for (int y = 0 ; y < newHeight ; y++)
            {
                memcpy(pict, Y, newWidth);
                pict += d3drect.Pitch;
                Y += newWidth;
            }
            for (int y = 0 ; y < newHeight >> 1 ; y++)
            {
                memcpy(pict, V, newWidth);
                pict += d3drect.Pitch;
                V += newWidth;
            }
            break;
        case D3DFMT_YUY2:
        case D3DFMT_UYVY:
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            memcpy(pict, Y, d3drect.Pitch * newHeight);
            break;
    }  
    return m_pOffsceenSurface->UnlockRect();
}

HRESULT D3D9RenderImpl::Present(void)
{
	if(NULL == m_pDevice)
	{
		return E_POINTER;
	}
			
	HRESULT hr = m_pDevice->PresentEx(NULL, NULL, NULL, NULL, NULL);
	if(FAILED(hr))
	{
		return CheckDevice();
	}

	return S_OK;
}

HRESULT D3D9RenderImpl::CreateVideoSurface(int width, int height, D3DFORMAT format, double storageAspectRatio)
{
    m_videoWidth = width;
    m_videoHeight = height;
    m_format = format;
	m_aspectRatio = storageAspectRatio;
    HR(m_pDevice->CreateOffscreenPlainSurfaceEx(width, height, format, D3DPOOL_DEFAULT, &m_pOffsceenSurface, NULL, NULL));
    HR(m_pDevice->ColorFill(m_pOffsceenSurface, NULL, D3DCOLOR_ARGB(0xFF, 0, 0, 0)));
	UpdateDisplayRect();
	if(m_renderModeIsWindowed)
	{
		return S_OK;
	}
	return CreateResources(width, height);
}

HRESULT D3D9RenderImpl::GetPresentParams(D3DPRESENT_PARAMETERS* params, BOOL bWindowed)
{
    UINT height, width;
    if(bWindowed) // windowed mode
    {
        RECT rect;
        ::GetClientRect(m_hDisplayWindow, &rect);
        height = rect.bottom - rect.top;
        width = rect.right - rect.left;
    }
    else   // fullscreen mode
    {
        width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }
    D3DPRESENT_PARAMETERS presentParams = {0};
	presentParams.Flags                  = D3DPRESENTFLAG_VIDEO | D3DPRESENTFLAG_OVERLAY_YCbCr_BT709;
    presentParams.Windowed               = bWindowed;
    presentParams.hDeviceWindow          = m_hDisplayWindow;
    presentParams.BackBufferWidth        = width == 0 ? 1 : width;
    presentParams.BackBufferHeight       = height == 0 ? 1 : height;
    presentParams.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    presentParams.MultiSampleType        = D3DMULTISAMPLE_NONMASKABLE;
	presentParams.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;
    presentParams.BackBufferFormat       = m_displayMode.Format;
    presentParams.BackBufferCount        = 1;
    presentParams.EnableAutoDepthStencil = FALSE;
    memcpy(params, &presentParams, sizeof(D3DPRESENT_PARAMETERS));
    return S_OK;
}

HRESULT D3D9RenderImpl::SetVertexShader(LPCWSTR pVertexShaderName, LPCSTR entryPoint, LPCSTR shaderModel, LPSTR* ppError)
{
    CComPtr<ID3DXBuffer> code;
    CComPtr<ID3DXBuffer> errMsg;
    HRESULT hr = D3DXCompileShaderFromFile(pVertexShaderName, NULL, NULL, entryPoint, shaderModel, 0, &code, &errMsg, &m_pVertexConstantTable);
    if (FAILED(hr))
    {	
        if(errMsg != NULL)
        {
            size_t len = errMsg->GetBufferSize() + 1;
            *ppError = new CHAR[len];		
            memcpy(*ppError, errMsg->GetBufferPointer(), len);			
        }
        return hr;
    }
    return m_pDevice->CreateVertexShader((DWORD*)code->GetBufferPointer(), &m_pVertexShader);
}

HRESULT D3D9RenderImpl::ApplyWorldViewProj(LPCSTR matrixName)
{
    D3DXMATRIX matOrtho;
    HR(m_pDevice->GetTransform(D3DTS_PROJECTION, &matOrtho));
    return m_pVertexConstantTable->SetMatrix(m_pDevice, matrixName, &matOrtho);
}

HRESULT D3D9RenderImpl::SetVertexShader(DWORD* buffer)
{
    HR(D3DXGetShaderConstantTable(buffer, &m_pVertexConstantTable));
    return m_pDevice->CreateVertexShader(buffer, &m_pVertexShader);
}

HRESULT D3D9RenderImpl::SetVertexShaderConstant(LPCSTR name, LPVOID value, UINT size)
{
    return m_pVertexConstantTable->SetValue(m_pDevice, name, value, size);
}

HRESULT D3D9RenderImpl::SetPixelShader(LPCWSTR pPixelShaderName, LPCSTR entryPoint, LPCSTR shaderModel, LPSTR* ppError)
{
    CComPtr<ID3DXBuffer> code;
    CComPtr<ID3DXBuffer> errMsg;
    HRESULT hr = D3DXCompileShaderFromFile(pPixelShaderName, NULL, NULL, entryPoint, shaderModel, 0, &code, &errMsg, &m_pPixelConstantTable);
    if (FAILED(hr))
    {	
        if(errMsg != NULL)
        {
            size_t len = errMsg->GetBufferSize() + 1;
            *ppError = new CHAR[len];		
            memcpy(*ppError, errMsg->GetBufferPointer(), len);	
        }
        return hr;
    }
	return m_pDevice->CreatePixelShader((DWORD*)code->GetBufferPointer(), &m_pPixelShader);
}

HRESULT D3D9RenderImpl::SetPixelShader(DWORD* buffer)
{
    HR(D3DXGetShaderConstantTable(buffer, &m_pPixelConstantTable));
    return m_pDevice->CreatePixelShader(buffer, &m_pPixelShader);
}

HRESULT D3D9RenderImpl::SetPixelShaderConstant(LPCSTR name, LPVOID value, UINT size)
{
    return m_pPixelConstantTable->SetValue(m_pDevice, name, value, size);
}

HRESULT D3D9RenderImpl::SetVertexShaderMatrix(D3DXMATRIX* matrix, LPCSTR name)
{
    return m_pVertexConstantTable->SetMatrix(m_pDevice, name, matrix);
}

HRESULT D3D9RenderImpl::SetPixelShaderMatrix(D3DXMATRIX* matrix, LPCSTR name)
{
    return m_pPixelConstantTable->SetMatrix(m_pDevice, name, matrix);
}
    
HRESULT D3D9RenderImpl::SetVertexShaderVector(D3DXVECTOR4* vector, LPCSTR name)
{
    return m_pVertexConstantTable->SetVector(m_pDevice, name, vector);
}
    
HRESULT D3D9RenderImpl::SetPixelShaderVector(D3DXVECTOR4* vector, LPCSTR name)
{
    return m_pPixelConstantTable->SetVector(m_pDevice, name, vector);
}

HRESULT D3D9RenderImpl::DrawLine(SHORT key, POINT p1, POINT p2, FLOAT width, D3DCOLOR color, BYTE opacity)
{
    m_overlays.AddOverlay(new LineOverlay(m_pDevice, p1, p2, width, color, opacity), key);
    return S_OK;
}

HRESULT D3D9RenderImpl::DrawRectangle(SHORT key, RECT rectangle, FLOAT width, D3DCOLOR color, BYTE opacity)
{
    m_overlays.AddOverlay(new RectangleOverlay(m_pDevice, rectangle, width, color, opacity), key);
    return S_OK;
}

HRESULT D3D9RenderImpl::DrawPolygon(SHORT key, POINT* points, INT pointsLen, FLOAT width, D3DCOLOR color, BYTE opacity)
{
    m_overlays.AddOverlay(new PolygonOverlay(m_pDevice, points, pointsLen, width, color, opacity), key);
    return S_OK;
}

HRESULT D3D9RenderImpl::DrawText(SHORT key, LPCWSTR text, RECT pos, INT size, D3DCOLOR color, LPCWSTR font, BYTE opacity)
{
    m_overlays.AddOverlay(new TextOverlay(m_pDevice, text, pos, size, color, font, opacity), key);
    return S_OK;
}

HRESULT D3D9RenderImpl::DrawBitmap(SHORT key, RECT rectangle, LPCWSTR path, D3DCOLOR colorKey, BYTE opacity)
{
    m_overlays.AddOverlay(new FileOverlay(m_pDevice, rectangle, path, colorKey, opacity), key);
    return S_OK;
}

HRESULT D3D9RenderImpl::DrawBitmap(SHORT key, RECT rectangle, BYTE* pixelData, UINT stride, INT width, INT height, D3DCOLOR colorKey, BYTE opacity)
{
	m_overlays.AddOverlay(new MemoryBitmapOverlay(m_pDevice, rectangle, pixelData, stride, width, height, colorKey, opacity), key);
	return S_OK;
}
	
	
HRESULT D3D9RenderImpl::RemoveOverlay(SHORT key)
{
    m_overlays.RemoveOverlay(key);
    return S_OK;
}

HRESULT D3D9RenderImpl::RemoveAllOverlays()
{
    m_overlays.RemoveAll();
    return S_OK;
}

HRESULT D3D9RenderImpl::CaptureDisplayFrame(BYTE* pBuffer, INT* width, INT* height, INT* stride)
{
    CComPtr<IDirect3DSurface9> pTargetSurface;	
    CComPtr<IDirect3DSurface9> pTempSurface;
    HR(m_pDevice->GetRenderTarget(0, &pTargetSurface));	
    D3DSURFACE_DESC desc;		
    HR(pTargetSurface->GetDesc(&desc));	
    if(!pBuffer)
    {
        *width = desc.Width;
        *height = desc.Height;
        *stride = desc.Width * 4; // Always ARGB32
        return S_OK;
    }
    HR(m_pDevice->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pTempSurface, NULL));				
    HR(m_pDevice->GetRenderTargetData(pTargetSurface, pTempSurface));					 
    D3DLOCKED_RECT d3drect;
    HR(pTempSurface->LockRect(&d3drect, NULL, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY));		
    BYTE* pFrame = (BYTE*)d3drect.pBits;
    memcpy(pBuffer, pFrame, desc.Height * d3drect.Pitch);
    return pTempSurface->UnlockRect();
}

HRESULT D3D9RenderImpl::CaptureVideoFrame(BYTE* pBuffer, INT* width, INT* height, INT* stride)
{
    if(!pBuffer)
    {
        *width = m_videoWidth;
        *height = m_videoHeight;
        *stride = m_videoWidth * 4; // Always ARGB32
        return S_OK;
    }
    CComPtr<IDirect3DSurface9> pTempSurface;			
    HR(m_pDevice->CreateOffscreenPlainSurface(m_videoWidth, m_videoHeight, m_displayMode.Format, D3DPOOL_DEFAULT, &pTempSurface, NULL));
    HR(m_pDevice->StretchRect(m_pOffsceenSurface, NULL, pTempSurface, NULL, D3DTEXF_LINEAR))	;  
    D3DLOCKED_RECT d3drect;
    HR(pTempSurface->LockRect(&d3drect, NULL, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY));		   
    BYTE* pFrame = (BYTE*)d3drect.pBits;
    BYTE* pBuf = pBuffer;
    memcpy(pBuf, pFrame, m_videoHeight * d3drect.Pitch);   
    return pTempSurface->UnlockRect();
}

HRESULT D3D9RenderImpl::ClearPixelShader()
{
    SafeRelease(m_pPixelConstantTable); 
    SafeRelease(m_pPixelShader);
    return S_OK;
}

HRESULT D3D9RenderImpl::ClearVertexShader()
{
    SafeRelease(m_pVertexConstantTable); 
    SafeRelease(m_pVertexShader);   
    return S_OK;
}

void D3D9RenderImpl::SetDisplayMode(FillMode mode)
{
    m_fillMode = mode;
	UpdateDisplayRect();
}

FillMode D3D9RenderImpl::GetDisplayMode()
{
    return m_fillMode;
}

void D3D9RenderImpl::UpdateDisplayRect()
{
	if(m_renderModeIsWindowed)
	{
		::GetClientRect(m_hDisplayWindow, &m_targetRect);
	}
	else
	{
		memset(&m_targetRect, 0, sizeof(RECT));
		m_targetRect.right = m_videoWidth;
		m_targetRect.bottom = m_videoHeight;
	}

	if(m_fillMode == FillMode::KeepAspectRatio)
	{
		ApplyLetterBoxing(m_targetRect, (FLOAT)m_videoWidth, (FLOAT)m_videoHeight, m_aspectRatio);
	}
}

HRESULT D3D9RenderImpl::GetRenderTargetSurface(IDirect3DSurface9** ppSurface)
{
	if(m_pTextureSurface == NULL)
	{
		*ppSurface = NULL;
		return E_POINTER;
	}

	*ppSurface = m_pTextureSurface;
	return S_OK;
}

HRESULT D3D9RenderImpl::GetDevice(IDirect3DDevice9** ppDevice)
{
	*ppDevice = m_pDevice;
	return S_OK;
}

void D3D9RenderImpl::CreateDummyWindow()
{
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = DefWindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = NULL;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = TEXT("D3DImageSample");
	RegisterClass(&wndclass);
	m_hDisplayWindow = CreateWindow(TEXT("D3DImageSample"),
		TEXT("D3DImageSample"),
		WS_OVERLAPPEDWINDOW,
		0,                   // X
		0,                   // Y
		0,                   // Width
		0,                   // Height
		NULL,
		NULL,
		NULL,
		NULL);
}

HRESULT D3D9RenderImpl::AddOverlayStream(SHORT key, int videoWidth, int videoHeight, D3DFORMAT videoFormat, RECT targetRect, BYTE opacity)
{
	auto overlay = new VideoOverlay(m_pDevice, videoWidth, videoHeight, videoFormat, targetRect, opacity);
	m_overlays.AddOverlay(overlay, key);
	return S_OK;
}

HRESULT D3D9RenderImpl::UpdateOverlayFrame(SHORT key, BYTE* pYplane, BYTE* pVplane, BYTE* pUplane)
{
	return m_overlays.UpdateOverlay(key, pYplane, pVplane, pUplane);
}

HRESULT D3D9RenderImpl::UpdateOverlayOpacity(SHORT key, BYTE opacity)
{
	return m_overlays.UpdateOverlayOpacity(key, opacity);
}

HRESULT D3D9RenderImpl::Clear(D3DCOLOR color)
{
	m_pDevice->ColorFill(m_pTextureSurface, NULL, color);
	HR(CreateScene());
	return Present();
}

void D3D9RenderImpl::SetShowOverlays(bool bShow)
{
	m_showOverlays = bShow;
}

bool D3D9RenderImpl::GetShowOverlays()
{
	return m_showOverlays;
}

HRESULT D3D9RenderImpl::DisplaySurface(IDirect3DSurface9* pSurface)
{
	CAutoLock lock(&m_locker);
	if(NULL == pSurface)
	{
		return E_POINTER;
	}
	//HR(CopySurface(pSurface));
	HR(m_pDevice->StretchRect(pSurface, NULL, m_pTextureSurface, &m_targetRect, D3DTEXF_LINEAR));
	HR(m_pDevice->SetRenderTarget(0, m_pRenderTarget));
	HR(CreateScene());
	return Present();
}

HRESULT D3D9RenderImpl::Repaint()
{
	CAutoLock lock(&m_locker);	
	HR(StretchSurface());
	HR(CreateScene());
	return Present(); 
}

HRESULT D3D9RenderImpl::CopySurface(IDirect3DSurface9* pSurface)
{
	if(m_pLastFrame == NULL)
	{
		D3DSURFACE_DESC desc;
		HRESULT hr = pSurface->GetDesc(&desc);
		hr = m_pDevice->CreateOffscreenPlainSurfaceEx(desc.Width, desc.Height, desc.Format, desc.Pool, &m_pLastFrame, NULL, 0);
		if(FAILED(hr))
		{
			return hr;
		}
	}

	return m_pDevice->StretchRect(pSurface, NULL, m_pLastFrame, NULL, D3DTEXF_NONE);
}
