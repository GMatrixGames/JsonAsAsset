// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef NV_IMAGE_DIRECTDRAWSURFACE_H
#define NV_IMAGE_DIRECTDRAWSURFACE_H

#include <nvimage/nvimage.h>

#if !defined(MAKEFOURCC)
#	define MAKEFOURCC(ch0, ch1, ch2, ch3) \
(uint(uint8(ch0)) | (uint(uint8(ch1)) << 8) | \
(uint(uint8(ch2)) << 16) | (uint(uint8(ch3)) << 24 ))
#endif

static const uint FOURCC_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
static const uint FOURCC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
static const uint FOURCC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
static const uint FOURCC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
static const uint FOURCC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
static const uint FOURCC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');
static const uint FOURCC_RXGB = MAKEFOURCC('R', 'X', 'G', 'B');
static const uint FOURCC_ATI1 = MAKEFOURCC('A', 'T', 'I', '1');
static const uint FOURCC_ATI2 = MAKEFOURCC('A', 'T', 'I', '2');

static const uint FOURCC_A2XY = MAKEFOURCC('A', '2', 'X', 'Y');

static const uint FOURCC_DX10 = MAKEFOURCC('D', 'X', '1', '0');

// 32 bit RGB formats.
static const uint D3DFMT_R8G8B8 = 20;
static const uint D3DFMT_A8R8G8B8 = 21;
static const uint D3DFMT_X8R8G8B8 = 22;
static const uint D3DFMT_R5G6B5 = 23;
static const uint D3DFMT_X1R5G5B5 = 24;
static const uint D3DFMT_A1R5G5B5 = 25;
static const uint D3DFMT_A4R4G4B4 = 26;
static const uint D3DFMT_R3G3B2 = 27;
static const uint D3DFMT_A8 = 28;
static const uint D3DFMT_A8R3G3B2 = 29;
static const uint D3DFMT_X4R4G4B4 = 30;
static const uint D3DFMT_A2B10G10R10 = 31;
static const uint D3DFMT_A8B8G8R8 = 32;
static const uint D3DFMT_X8B8G8R8 = 33;
static const uint D3DFMT_G16R16 = 34;
static const uint D3DFMT_A2R10G10B10 = 35;

static const uint D3DFMT_A16B16G16R16 = 36;

// Palette formats.
static const uint D3DFMT_A8P8 = 40;
static const uint D3DFMT_P8 = 41;

// Luminance formats.
static const uint D3DFMT_L8 = 50;
static const uint D3DFMT_A8L8 = 51;
static const uint D3DFMT_A4L4 = 52;
static const uint D3DFMT_L16 = 81;

// Floating point formats
static const uint D3DFMT_R16F = 111;
static const uint D3DFMT_G16R16F = 112;
static const uint D3DFMT_A16B16G16R16F = 113;
static const uint D3DFMT_R32F = 114;
static const uint D3DFMT_G32R32F = 115;
static const uint D3DFMT_A32B32G32R32F = 116;

static const uint DDSD_CAPS = 0x00000001U;
static const uint DDSD_PIXELFORMAT = 0x00001000U;
static const uint DDSD_WIDTH = 0x00000004U;
static const uint DDSD_HEIGHT = 0x00000002U;
static const uint DDSD_PITCH = 0x00000008U;
static const uint DDSD_MIPMAPCOUNT = 0x00020000U;
static const uint DDSD_LINEARSIZE = 0x00080000U;
static const uint DDSD_DEPTH = 0x00800000U;

static const uint DDSCAPS_COMPLEX = 0x00000008U;
static const uint DDSCAPS_TEXTURE = 0x00001000U;
static const uint DDSCAPS_MIPMAP = 0x00400000U;
static const uint DDSCAPS2_VOLUME = 0x00200000U;
static const uint DDSCAPS2_CUBEMAP = 0x00000200U;

static const uint DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U;
static const uint DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U;
static const uint DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U;
static const uint DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U;
static const uint DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U;
static const uint DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U;
static const uint DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000FC00U;

static const uint DDPF_ALPHAPIXELS = 0x00000001U;
static const uint DDPF_ALPHA = 0x00000002U;
static const uint DDPF_FOURCC = 0x00000004U;
static const uint DDPF_RGB = 0x00000040U;
static const uint DDPF_PALETTEINDEXED1 = 0x00000800U;
static const uint DDPF_PALETTEINDEXED2 = 0x00001000U;
static const uint DDPF_PALETTEINDEXED4 = 0x00000008U;
static const uint DDPF_PALETTEINDEXED8 = 0x00000020U;
static const uint DDPF_LUMINANCE = 0x00020000U;
static const uint DDPF_ALPHAPREMULT = 0x00008000U;
static const uint DDPF_NORMAL = 0x80000000U; // @@ Custom nv flag.

namespace nv {
	class Image;
	class Stream;
	struct ColorBlock;

	struct NVIMAGE_CLASS DDSPixelFormat {
		uint size;
		uint flags;
		uint fourcc;
		uint bitcount;
		uint rmask;
		uint gmask;
		uint bmask;
		uint amask;
	};

	struct NVIMAGE_CLASS DDSCaps {
		uint caps1;
		uint caps2;
		uint caps3;
		uint caps4;
	};

	/// DDS file header for DX10.
	struct NVIMAGE_CLASS DDSHeader10 {
		uint dxgiFormat;
		uint resourceDimension;
		uint miscFlag;
		uint arraySize;
		uint reserved;
	};

	/// DDS file header.
	struct NVIMAGE_CLASS DDSHeader {
		uint fourcc;
		uint size;
		uint flags;
		uint height;
		uint width;
		uint pitch;
		uint depth;
		uint mipmapcount;
		uint reserved[11];
		DDSPixelFormat pf;
		DDSCaps caps;
		uint notused;
		DDSHeader10 header10;


		// Helper methods.
		NVTT_API DDSHeader();

		NVTT_API void setWidth(uint w);
		NVTT_API void setHeight(uint h);
		NVTT_API void setDepth(uint d);
		void setMipmapCount(uint count);
		void setTexture2D();
		void setTexture3D();
		NVTT_API void setTextureCube();
		void setLinearSize(uint size);
		void setPitch(uint pitch);
		NVTT_API void setFourCC(uint32 FourCC);
		void setFourCC(uint8 c0, uint8 c1, uint8 c2, uint8 c3);
		void setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask);
		void setDX10Format(uint format);
		NVTT_API void setNormalFlag(bool b);

		void swapBytes();

		bool hasDX10Header() const;
	};

	NVIMAGE_API Stream& operator<<(Stream& s, DDSHeader& header);


	/// DirectDraw Surface. (DDS)
	class NVIMAGE_CLASS DirectDrawSurface {
	public:
		NVTT_API DirectDrawSurface(const char* file);
		NVTT_API DirectDrawSurface(Stream* stream); // added implicit stream version, like in recent NVTT code
		NVTT_API ~DirectDrawSurface();

		bool isValid() const;
		bool isSupported() const;

		uint mipmapCount() const;
		uint width() const;
		uint height() const;
		uint depth() const;
		bool isTexture1D() const;
		bool isTexture2D() const;
		bool isTexture3D() const;
		bool isTextureCube() const;

		void setNormalFlag(bool b);

		NVTT_API void mipmap(Image* img, uint f, uint m);
		//	void mipmap(FloatImage * img, uint f, uint m);

		void printInfo() const;

	private:
		uint blockSize() const;
		uint faceSize() const;
		uint mipmapSize(uint m) const;

		uint offset(uint f, uint m);

		void readLinearImage(Image* img);
		void readBlockImage(Image* img);
		void readBlock(ColorBlock* rgba);

	private:
		Stream* const stream;
		DDSHeader10 header10;

	public:
		DDSHeader header;
	};
} // nv namespace

#endif // NV_IMAGE_DIRECTDRAWSURFACE_H
