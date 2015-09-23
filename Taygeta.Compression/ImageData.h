#pragma once

#include "turbojpeg.h"
#include "windows.h"

struct ImageData
{
	int Height;
    int Lines[4];
    int Pitches[4];
    BYTE* Planes[4];
	int Components;
	TJSAMP Subsampling;
    int Width;
};