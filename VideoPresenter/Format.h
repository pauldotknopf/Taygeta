#pragma once

using namespace System;

namespace Taygeta { namespace Core 
{
	public enum class PixelFormat
	{
		YV12 = 0,
		NV12,
		YUY2,
		UYVY,

		RGB15,
		RGB16,
		RGB32,
		ARGB32
	};

	public enum class VideoDisplayMode
	{
		KeepAspectRatio = 0,
        Fill = 1
	};
}
}