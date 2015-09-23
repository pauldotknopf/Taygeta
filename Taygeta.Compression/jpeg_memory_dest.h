// Taken from the Galapix project
//
// http://code.google.com/p/galapix/
//
//

#pragma once

#ifndef HEADER_JPEG_MEMORY_SRC_HPP
#define HEADER_JPEG_MEMORY_SRC_HPP

#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <jpeglib.h>
#include "windows.h"

void jpeg_memory_dest(j_compress_ptr cinfo, std::vector<BYTE>* data);

#endif