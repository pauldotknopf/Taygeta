#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include "ImageData.h"
#include "jpeglib.h"
#include "jpeg_memory_dest.h"
#include <vector>

using namespace std;

class CLibjpegEncoderImpl
{
public:
	CLibjpegEncoderImpl(void);
	virtual ~CLibjpegEncoderImpl(void);
	void Save(ImageData& imgData, vector<BYTE>* byteArray , int quality);

private:
	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;
};

