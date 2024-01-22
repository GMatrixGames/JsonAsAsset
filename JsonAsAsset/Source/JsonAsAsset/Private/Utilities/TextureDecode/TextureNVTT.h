// Copyright JAA Contributors 2023-2024

#pragma once

#include "nvimage/Image.h"
#include "nvimage/DirectDrawSurface.h"

#undef __FUNC__						// conflicted with our guard macros

void DecodeDDS(const unsigned char* Data, int SizeX, int SizeY, int SizeZ, nv::DDSHeader& Header, nv::Image& Image);
