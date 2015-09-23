#pragma once

#include "JasperImpl.h"
#include "jasper\jasper.h"
#include "turbojpeg.h"
#include "LibjpegEncoderImpl.h"

using namespace System; 
using namespace System::IO;
using namespace System::Runtime::InteropServices;
using namespace Taygeta::Imaging;

namespace Taygeta { namespace Compression 
{
	public ref class Jpeg2000Compressor
	{
	public:
		Jpeg2000Compressor()
		{
			jas_init();
		}

		virtual ~Jpeg2000Compressor()
		{
			jas_cleanup();
		}

		void Save(PlanarImage^ image, Stream^ stream, double quality, bool bLossless)
		{
			if(image->PixelType != PixelAlignmentType::YUV &&
               image->PixelType != PixelAlignmentType::I420 &&
			   image->PixelType != PixelAlignmentType::Y800)
			{
				throw gcnew ArgumentException("Pixel type not supported");
			}

			if(stream == nullptr)
			{
				throw gcnew ArgumentNullException("stream");
			}

			if(quality < 0 || quality > 1)
			{
				throw gcnew ArgumentException("Quality must be in range of [0,1]");
			}

			jas_image_t* jimage = NULL;
			jas_matrix_t* cmpts[3];
			for(int z = 0; z< 3; z++)
			{
				cmpts[z] = NULL;
			}
			jas_image_cmptparm_t cmptparm[3];
			BYTE* planes[3];
			int bufferSize = image->Width * image->Height;
			long* plane = new long[bufferSize];

			switch(image->PixelType)
			{
			case PixelAlignmentType::YUV:

				for (int c = 0; c < 3; c++) 
				{
					cmptparm[c].tlx = 0;
					cmptparm[c].tly = 0;
					cmptparm[c].hstep = 1;
					cmptparm[c].vstep = 1;
					cmptparm[c].width = image->Pitches[c];
					cmptparm[c].height = image->Lines[c];
					cmptparm[c].prec = 8;
					cmptparm[c].sgnd = false;
					cmpts[c] = jas_matrix_create(1, image->Pitches[c]);
				}

				jimage = jas_image_create(3, cmptparm, JAS_CLRSPC_SYCBCR);
				if(!jimage)
				{
					throw gcnew InvalidOperationException("Failed to create image");
				}
				jas_image_setcmpttype(jimage, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_Y));
				jas_image_setcmpttype(jimage, 1, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CB));
				jas_image_setcmpttype(jimage, 2, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CR));

				planes[0] = (BYTE*)image->Planes[0].ToPointer();
				planes[1] = (BYTE*)image->Planes[1].ToPointer();
				planes[2] = (BYTE*)image->Planes[2].ToPointer();

				for (int y = 0; y < image->Lines[0]; ++y) 
				{
					for (int x = 0; x < image->Pitches[0]; ++x) 
					{
						jas_matrix_setv(cmpts[0], x, planes[0][x + y * image->Pitches[0]]);	
						jas_matrix_setv(cmpts[1], x, planes[1][x + y * image->Pitches[1]]);	
						jas_matrix_setv(cmpts[2], x, planes[2][x + y * image->Pitches[2]]);	
					}
		
					for (int c = 0; c < 3; ++c) 
					{
						jas_image_writecmpt(jimage, c, 0, y, image->Pitches[0], 1, cmpts[c]);
					}					
				}

				break;

			case PixelAlignmentType::I420:

				for (int c = 0; c < 3; c++) 
				{
					cmptparm[c].tlx = 0;
					cmptparm[c].tly = 0;
					cmptparm[c].width = image->Pitches[c];
					cmptparm[c].height = image->Lines[c];
					cmptparm[c].prec = 8;
					cmptparm[c].sgnd = false;
					cmpts[c] = jas_matrix_create(1, image->Pitches[c]);
				}

				cmptparm[0].hstep = 1;
				cmptparm[0].vstep = 1;
				cmptparm[1].hstep = 2;
				cmptparm[1].vstep = 2;
				cmptparm[2].hstep = 2;
				cmptparm[2].vstep = 2;

				jimage = jas_image_create(3, cmptparm, JAS_CLRSPC_SYCBCR);
				if(!jimage)
				{
					throw gcnew InvalidOperationException("Failed to create image");
				}
				jas_image_setcmpttype(jimage, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_Y));
				jas_image_setcmpttype(jimage, 1, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CB));
				jas_image_setcmpttype(jimage, 2, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CR));

				planes[0] = (BYTE*)image->Planes[0].ToPointer();
				planes[1] = (BYTE*)image->Planes[1].ToPointer();
				planes[2] = (BYTE*)image->Planes[2].ToPointer();
			
				memset(plane, 0, bufferSize);

				for(int i = 0; i < 3; i++)
				{		
					int size = image->Pitches[i] * image->Lines[i];
					for(int j = 0; j < size ; j++)
					{
						plane[j] = (long)planes[i][j];
					}

					int res = jas_image_writecmpt2(jimage, i, 0, 0, image->Pitches[i], image->Lines[i], plane);
					if(res < 0)
					{
						char szoutopts[255];
						sprintf_s(szoutopts,"Failed to write component %d data", i);
						throw gcnew InvalidOperationException(gcnew String(szoutopts));
					}
				}
				break;

			case PixelAlignmentType::Y800:

				for (int c = 0; c < 1; c++) 
				{
					cmptparm[c].tlx = 0;
					cmptparm[c].tly = 0;
					cmptparm[c].hstep = 1;
					cmptparm[c].vstep = 1;
					cmptparm[c].width = image->Pitches[c];
					cmptparm[c].height = image->Lines[c];
					cmptparm[c].prec = 8;
					cmptparm[c].sgnd = false;
					cmpts[c] = jas_matrix_create(1, image->Pitches[c]);
				}

				jimage = jas_image_create(1, cmptparm, JAS_CLRSPC_SGRAY);
				if(!jimage)
				{
					throw gcnew InvalidOperationException("Failed to create image");
				}
				jas_image_setcmpttype(jimage, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));

				BYTE* planes[3];
				planes[0] = (BYTE*)image->Planes[0].ToPointer();

				for (int y = 0; y < image->Lines[0]; ++y) 
				{
					for (int x = 0; x < image->Pitches[0]; ++x) 
					{
						jas_matrix_setv(cmpts[0], x, planes[0][x + y * image->Pitches[0]]);	
					}
		
					jas_image_writecmpt(jimage, 0, 0, y, image->Pitches[0], 1, cmpts[0]);					
				}
				break;
			}

			int outfmt = jas_image_strtofmt("jp2");

			char szoutopts[40];
			char* mode = bLossless == true ? "int" : "real";
			sprintf_s(szoutopts,"rate=%.3f mode=%s", quality, mode);

			jas_stream_t* jstream = jas_stream_memopen(NULL, 0);
			if(!stream)
			{
				throw gcnew InvalidOperationException("Failed to create output stream");
			}

			int res = jas_image_encode(jimage, jstream, outfmt, szoutopts);
			if(res < 0)
			{
				jas_stream_close(jstream);
				throw gcnew InvalidOperationException("Failed to encode image");
			}
			jas_stream_flush(jstream);
			jas_stream_memobj_t* jmem = (jas_stream_memobj_t*)jstream->obj_;
			
			array<byte>^ buf = gcnew array<byte>(jmem->len_);
			Marshal::Copy(IntPtr(jmem->buf_), buf, 0, jmem->len_);
			stream->Write(buf, 0, jmem->len_);

			delete plane;
			jas_image_destroy(jimage);
			jas_stream_close(jstream);
			for(int z = 0; z< 3; z++)
			{
				if(cmpts[z] != NULL)
				{
					jas_matrix_destroy(cmpts[z]);
				}
			}
		}
	};

	public ref class Jpeg2000Decomressor
	{
	public:
		Jpeg2000Decomressor()
		{
			m_impl = new CJasperImpl();
		}

		virtual ~Jpeg2000Decomressor(void)
		{
			delete m_impl;
		}

		PlanarImage^ Load(array<byte>^ buffer)
		{
			BYTE* pBuf = new BYTE[buffer->Length];
			Marshal::Copy(buffer, 0, IntPtr(pBuf), buffer->Length);
			PlanarImage^ image = Load(IntPtr(pBuf), buffer->Length);
			delete pBuf;
			return image;
		}

		PlanarImage^ Load(IntPtr pBuffer, int bufferSize)
		{
			ImageData data;
			try
			{
				m_impl->Load((BYTE*)pBuffer.ToPointer(), bufferSize, data);
			}
			catch(char* msg)
			{
				throw gcnew InvalidOperationException(gcnew String(msg));
			}
			array<IntPtr>^ buffers = gcnew array<IntPtr>(data.Components);
			for(int i=0; i<data.Components; i++)
			{
				buffers[i] = IntPtr(data.Planes[i]);
			}
			PixelAlignmentType pType = data.Components == 3 ? PixelAlignmentType::I420 : PixelAlignmentType::Y800;
			PlanarImage^ image = gcnew PlanarImage(data.Width, data.Height, pType, buffers);
			for(int i=0; i<data.Components; i++)
			{
				delete data.Planes[i];
			}
			return image;
		}

	private:
		CJasperImpl* m_impl;
	};

	public ref class JpegDecompressor
	{
	public:
		JpegDecompressor()
		{
			m_impl = new CTurboJpegDecoderImpl();
		}

		virtual ~JpegDecompressor(void)
		{
			delete m_impl;
		}

		PlanarImage^ Load(array<byte>^ buffer)
		{
			pin_ptr<BYTE> pBuf = &buffer[0];
			
			ImageData data;
			m_impl->Load(pBuf, buffer->Length, data);
			PlanarImage^ img = gcnew PlanarImage(data.Width, data.Height, 
				GetPixelAlignmentType(data.Subsampling), IntPtr(data.Planes[0]), data.Pitches[0]);

			tjFree(data.Planes[0]);
			return img;
		}

	private:
		CTurboJpegDecoderImpl* m_impl;

		static inline PixelAlignmentType GetPixelAlignmentType(TJSAMP samp)
		{
			switch(samp)
			{
			case TJSAMP_444:
				return PixelAlignmentType::YUV;
			case TJSAMP_422:
				return PixelAlignmentType::YUY2;
			case TJSAMP_420:
				return PixelAlignmentType::I420;
			case TJSAMP_GRAY:
				return PixelAlignmentType::Y800;
			case TJSAMP_440:
			default:
				throw "Unsupported subsampling YUV440";  
			}
		}
	};

	public ref class JpegCompressor
	{
	public:
		JpegCompressor()
		{
			m_impl = new CLibjpegEncoderImpl();
		}

		virtual ~JpegCompressor(void)
		{
			delete m_impl;
		}

		void Save(PlanarImage^ image, Stream^ stream, int quality)
		{
			ImageData iData;
			iData.Width = image->Width;
			iData.Height = image->Height;
			for(int i=0; i< image->NumberOfPlanes; i++)
			{
				iData.Pitches[i] = image->Pitches[i];
				iData.Lines[i] = image->Lines[i];
				iData.Planes[i] = (BYTE*)image->Planes[i].ToPointer();
			}

			iData.Components = image->NumberOfPlanes;
			iData.Subsampling = GetSubsampingType(image->PixelType);

			std::vector<BYTE> buffer;
			m_impl->Save(iData, &buffer, quality);
			int outputDataSize = buffer.size();
			array<byte>^ buf = gcnew array<byte>(outputDataSize);
			Marshal::Copy(IntPtr(&buffer[0]), buf, 0, outputDataSize);
			stream->Write(buf, 0, outputDataSize);
		}

	private:
		CLibjpegEncoderImpl* m_impl;

		static inline TJSAMP GetSubsampingType(PixelAlignmentType pType)
		{
			switch(pType)
			{
			case PixelAlignmentType::YUV:
				return TJSAMP_444;
			case PixelAlignmentType::YUY2:
				return TJSAMP_422;
			case PixelAlignmentType::I420:
				return TJSAMP_420;
			case PixelAlignmentType::Y800:
				return TJSAMP_GRAY;
			default:
				throw gcnew InvalidOperationException("Unsupported subsampling type");
			}
		}
	};
}}

