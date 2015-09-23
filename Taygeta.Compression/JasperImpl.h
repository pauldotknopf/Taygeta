#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#include "jasper\jasper.h"
#include "turbojpeg.h"
#include "ImageData.h"

class CJasperImpl
{
public:
	CJasperImpl(void);
	virtual ~CJasperImpl(void);

private:
	jas_image_t* m_image;
	jas_stream_t* m_stream;
	jas_matrix_t* m_cmpts[3];
	jas_image_cmptparm_t m_cmptparms[3];
public:
	void Save(ImageData& data, BYTE** buffer, int* size, double quality);
	void Load(BYTE* buffer, int size, ImageData& data);
};

class CTurboJpegDecoderImpl
{
public:
	CTurboJpegDecoderImpl(void)
	{
		m_handle = tjInitDecompress();
	}

	virtual ~CTurboJpegDecoderImpl(void)
	{
		tjDestroy(m_handle);
	}

	void Load(BYTE* buffer, int size, ImageData& data)
	{
		int w, h, smp;
		int res = tjDecompressHeader2(m_handle, buffer, size, &w, &h, &smp);
		if(res == -1)
		{
			throw tjGetErrorStr();
		}

		unsigned long outSize = tjBufSizeYUV(w, h, smp);
		if(outSize == -1)
		{
			throw "Arguments are out of bounds";
		}
		BYTE* outBuffer = tjAlloc(outSize);
		res = tjDecompressToYUV(m_handle, buffer, size, outBuffer, TJFLAG_BOTTOMUP);
		if(res == -1)
		{
			throw tjGetErrorStr();
		}

		data.Width = w;
		data.Height = h;
		data.Subsampling = (TJSAMP)smp;
		data.Planes[0] = outBuffer;
		data.Pitches[0] = outSize;
	}

private:
	tjhandle m_handle;
};

