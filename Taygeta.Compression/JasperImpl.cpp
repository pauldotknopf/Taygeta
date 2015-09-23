#include "StdAfx.h"
#include "JasperImpl.h"


CJasperImpl::CJasperImpl(void)
{
	jas_init();
}


CJasperImpl::~CJasperImpl(void)
{
	jas_cleanup();
}

void CJasperImpl::Save(ImageData& data, BYTE** buffer, int* size, double quality)
{
	jas_image_cmptparm_t *cmptparm;
	uint_fast16_t cmptno, numcmpts=3;

	for (cmptno = 0, cmptparm = m_cmptparms; cmptno < numcmpts; ++cmptno, ++cmptparm) 
	{
		cmptparm->tlx = 0;
		cmptparm->tly = 0;
		cmptparm->width = data.Pitches[cmptno];
		cmptparm->height = data.Lines[cmptno];
		cmptparm->prec = 8;
		cmptparm->sgnd = false;
	}

	m_cmptparms[0].hstep = 1;
	m_cmptparms[0].vstep = 1;
	m_cmptparms[1].hstep = 2;
	m_cmptparms[1].vstep = 2;
	m_cmptparms[2].hstep = 2;
	m_cmptparms[2].vstep = 2;

	m_image = jas_image_create(numcmpts, m_cmptparms, JAS_CLRSPC_SYCBCR);
	if(!m_image)
	{
		throw "Failed to create image";
	}
	jas_image_setcmpttype(m_image, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_Y));
	jas_image_setcmpttype(m_image, 1, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CB));
	jas_image_setcmpttype(m_image, 2, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CR));

	int bufferSize = data.Width * data.Height;
	long* plane = new long[bufferSize];
	memset(plane, 0, bufferSize);

	for(int i = 0; i < data.Components; i++)
	{		
		int size = data.Pitches[i] * data.Lines[i];
		for(int j = 0; j < size ; j++)
		{
			plane[j] = (long)data.Planes[i][j];
		}

		int res = jas_image_writecmpt2(m_image, i, 0, 0, data.Pitches[i], data.Lines[i], plane);
		if(res < 0)
		{
			char szoutopts[255];
			sprintf_s(szoutopts,"Failed to write component %d data", i);
			throw szoutopts;
		}
	}
	delete plane;
	int outfmt = jas_image_strtofmt("jp2");

	char szoutopts[32];
	sprintf_s(szoutopts,"rate=%.3f", quality);

	jas_stream_t* stream = jas_stream_memopen(NULL, 0);
	if(!stream)
	{
		throw "Failed to create output stream";
	}

	int res = jas_image_encode(m_image, stream, outfmt, szoutopts);
	if(res < 0)
	{
		jas_stream_close(stream);
		throw "Failed to encode image";
	}
	jas_stream_flush(stream);
	jas_stream_memobj_t * jmem = (jas_stream_memobj_t *)stream->obj_;
	*size = jmem->len_;
	*buffer = jmem->buf_;
	jmem->myalloc_ = 0;

	jas_stream_close(stream);
}

void CJasperImpl::Load(BYTE* buffer, int size, ImageData& data)
{
	jas_stream_t* stream = jas_stream_memopen((char*)buffer, size);
	if(!stream)
	{
		throw "Failed to create input stream";
	}

	int outfmt = jas_image_getfmt(stream);
	if(outfmt == -1)
	{
		outfmt = jas_image_strtofmt("jp2");
	}

	jas_image_t* img = jas_image_decode(stream, outfmt, "");
	if(!m_image)
	{
		jas_stream_close(stream);
		throw "Failed to decode image";
	}
	data.Width = jas_image_width(img);
	data.Height = jas_image_height(img);
	data.Components = jas_image_numcmpts(img);

	if(data.Components == 3)
	{
		long* buf = new long[data.Width * data.Height + data.Width * data.Height / 2];
		memset(buf, 0, data.Width * data.Height + data.Width * data.Height / 2);

		data.Planes[0] = new BYTE[data.Width * data.Height];
		data.Planes[1] = new BYTE[data.Width * data.Height / 4];
		data.Planes[2] = new BYTE[data.Width * data.Height / 4];

		int res = jas_image_readcmpt2(img, 0, 0, 0, data.Width, data.Height, buf);
		res = jas_image_readcmpt2(img, 1, 0, 0, data.Width / 2, data.Height / 2, buf + data.Width * data.Height);
		res = jas_image_readcmpt2(img, 2, 0, 0, data.Width / 2, data.Height / 2, buf + (data.Width * data.Height + data.Width * data.Height / 4));
		if(res < 0)
		{
			delete buf;
			jas_stream_close(stream);
			jas_image_destroy(img);
			throw "Failed to read component data";
		}

		for(int i = 0; i < data.Width * data.Height; i++)
		{
			data.Planes[0][i] = (BYTE)buf[i];
		}

		for(int i = 0; i < data.Width * data.Height / 4; i++)
		{
			data.Planes[1][i] = (BYTE)buf[i + data.Width * data.Height];
			data.Planes[2][i] = (BYTE)buf[i + (data.Width * data.Height + data.Width * data.Height / 4)];
		}

		delete buf;
	}
	else
	{
		long* buf = new long[data.Width * data.Height];
		memset(buf, 0, data.Width * data.Height);
		data.Planes[0] = new BYTE[data.Width * data.Height];
		
		int res = jas_image_readcmpt2(img, 0, 0, 0, data.Width, data.Height, buf);
		if(res < 0)
		{
			delete buf;
			jas_stream_close(stream);
			jas_image_destroy(img);
			throw "Failed to read component data";
		}

		for(int i = 0; i < data.Width * data.Height; i++)
		{
			data.Planes[0][i] = (BYTE)buf[i];
		}

		delete buf;
	}

	jas_stream_close(stream);
	jas_image_destroy(img);
}
