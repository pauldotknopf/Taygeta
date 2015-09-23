#include "StdAfx.h"
#include "LibjpegEncoderImpl.h"


CLibjpegEncoderImpl::CLibjpegEncoderImpl(void)
{
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo); 
}


CLibjpegEncoderImpl::~CLibjpegEncoderImpl(void)
{
	jpeg_destroy_compress(&cinfo);
}

void CLibjpegEncoderImpl::Save(ImageData& imgData, vector<BYTE>* byteArray, int quality)
{
	JSAMPROW y[16],cb[16],cr[16]; // y[2][5] = color sample of row 2 and pixel column 5; (one plane) 
	JSAMPARRAY data[3]; // t[0][2][5] = color sample 0 of row 2 and column 5  

	data[0] = y; 
	data[1] = cb; 
	data[2] = cr; 

	cinfo.image_width = imgData.Width; 
	cinfo.image_height = imgData.Height; 
	int blockSize;

	switch(imgData.Subsampling)
	{
	case TJSAMP_420:
		cinfo.input_components = 3; 
		cinfo.in_color_space = JCS_YCbCr;
		jpeg_set_defaults(&cinfo); 
		blockSize = 2 * DCTSIZE;

		cinfo.comp_info[0].h_samp_factor = 2; 
		cinfo.comp_info[0].v_samp_factor = 2; 
		cinfo.comp_info[1].h_samp_factor = 1; 
		cinfo.comp_info[1].v_samp_factor = 1; 
		cinfo.comp_info[2].h_samp_factor = 1; 
		cinfo.comp_info[2].v_samp_factor = 1;

		cinfo.smoothing_factor = 0;
		cinfo.raw_data_in = TRUE; 
		jpeg_set_quality(&cinfo, quality, TRUE); 
		cinfo.dct_method = JDCT_FASTEST; 
		
		jpeg_memory_dest(&cinfo, byteArray);
		jpeg_start_compress(&cinfo, TRUE); 

		for (int j = 0; j < imgData.Height; j += blockSize) 
		{ 
			for (int i = 0; i < blockSize; i++)
			{ 
				y[i] = imgData.Planes[0] + imgData.Pitches[0] * (i + j); 
				if (i % 2 == 0) 
				{ 
					cb[i / 2] = imgData.Planes[1] + imgData.Pitches[1] * (i + j) / 2; 
					cr[i / 2] = imgData.Planes[2] + imgData.Pitches[2] * (i + j) / 2; 
				} 
			} 
			jpeg_write_raw_data(&cinfo, data, blockSize); 
		} 
		break;

	case TJSAMP_444:
		cinfo.input_components = 3; 
		cinfo.in_color_space = JCS_YCbCr;
		jpeg_set_defaults(&cinfo); 
		blockSize = DCTSIZE;

		cinfo.comp_info[0].h_samp_factor = 1; 
		cinfo.comp_info[0].v_samp_factor = 1; 
		cinfo.comp_info[1].h_samp_factor = 1; 
		cinfo.comp_info[1].v_samp_factor = 1; 
		cinfo.comp_info[2].h_samp_factor = 1; 
		cinfo.comp_info[2].v_samp_factor = 1;

		blockSize = DCTSIZE;
		cinfo.smoothing_factor = 0;
		cinfo.raw_data_in = TRUE; 
		jpeg_set_quality(&cinfo, quality, TRUE); 
		cinfo.dct_method = JDCT_FASTEST; 

		jpeg_memory_dest(&cinfo, byteArray);
		jpeg_start_compress(&cinfo, TRUE); 

		for (int j = 0; j < imgData.Height; j += blockSize) 
		{ 
			for (int i = 0; i < blockSize; i++)
			{ 
				y[i] = imgData.Planes[0] + imgData.Pitches[0] * (i + j); 
				cb[i] = imgData.Planes[1] + imgData.Pitches[1] * (i + j); 
				cr[i] = imgData.Planes[2] + imgData.Pitches[2] * (i + j); 
			} 
			jpeg_write_raw_data(&cinfo, data, blockSize); 
		} 
		break;

	case TJSAMP_GRAY:
		cinfo.input_components = 1; 
		cinfo.in_color_space = JCS_GRAYSCALE;
		jpeg_set_defaults(&cinfo); 
		blockSize = DCTSIZE;

		cinfo.smoothing_factor = 0;
		cinfo.raw_data_in = TRUE; 
		jpeg_set_quality(&cinfo, quality, TRUE); 
		cinfo.dct_method = JDCT_FASTEST; 

		jpeg_memory_dest(&cinfo, byteArray);
		jpeg_start_compress(&cinfo, TRUE); 

		for (int j = 0; j < imgData.Height; j += blockSize) 
		{ 
			for (int i = 0; i < blockSize; i++)
			{ 
				y[i] = imgData.Planes[0] + imgData.Pitches[0] * (i + j); 
			} 
			jpeg_write_raw_data(&cinfo, data, blockSize); 
		} 
		break;
	}

	jpeg_finish_compress(&cinfo); 
}
