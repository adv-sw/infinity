/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_pixel.h

Description : Pixel manipulation definition.

License : Copyright (c) 2002 - 2021, Advance Software Limited.

Redistribution and use in source and binary forms, with or without modification are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */


#ifndef INF_PIXEL_H
#define INF_PIXEL_H

#include "inf_basic_types.h"

#include <DXGIFormat.h>
#include <assert.h>

namespace Infinity
{

#if defined _WIN32 || defined _WIN64

// Pixelformat currently has a 1-1 match with DXGI in low order bits for optimal DX11 performance.
typedef uint32            PixelFormat;
const uint32              PixelFormat_DXGI = 0xfff;

#define INF_DXGI(p) ((DXGI_FORMAT) (##p## & Infinity::PixelFormat_DXGI))

// Some compressed texture formats (e.g. BC7) can optionally contain
// alpha data. We set this flag if we know the image is opaque.
#define DXGI_FORMAT_OPAQUE   (Infinity::PixelFormat) 0x2000

// Placeholder definitions for formats not supported by DXGI.
// Used by tools & DX9 backend.
// Only supported by tools (software sysmem textures).

#define DXGI_FORMAT_R8G8B8_UNORM   (Infinity::PixelFormat) 0x1000
#define DXGI_FORMAT_R8G8B8X8_UNORM (Infinity::PixelFormat) 0x1001
#define DXGI_FORMAT_R8A8_UNORM     DXGI_FORMAT_R8G8_UNORM


// Required to compile on XP/2010
#if _MSC_VER <= 1600
#define DXGI_FORMAT_B4G4R4A4_UNORM  (Infinity::PixelFormat) 0x1002
#define DXGI_FORMAT_P8   (Infinity::PixelFormat) 0x1003
#endif


#else

#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */


// TODO: Rename to Infinity types & reduce to minimal set.
// Note: These values do *not* currently match DX numeric equivalents so check we have no dependancies on exact values.
typedef enum _PixelFormat
{
    DXGI_FORMAT_UNKNOWN =  0,

    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_A8_UNORM,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_B8G8R8X8_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_R8G8B8_UNORM,
    DXGI_FORMAT_B5G5R5A1_UNORM,
    DXGI_FORMAT_B5G6R5_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC2_UNORM,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC6_UNORM,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,

    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC7_UNORM_SRGB,

    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,

    DXGI_FORMAT_B4G4R4A4_UNORM,
    DXGI_FORMAT_R8A8_UNORM,

    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_P8,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_SNORM,

    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32G32_UINT,

    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_FLOAT,

    DXGI_FORMAT_UNKNOWN_FORCE_DWORD = 0x7fffffff
} PixelFormat;

#endif


inline bool IsMono(PixelFormat pf)
{
   switch (pf)
	{
      case DXGI_FORMAT_R8_UNORM:
      case DXGI_FORMAT_BC4_UNORM:
      case DXGI_FORMAT_R8G8_UNORM:  // because we use R8G8 as R8A8
         return true;
   }

   return false;
}


inline bool IsCompressed(PixelFormat pf)
{
	assert(pf != DXGI_FORMAT_UNKNOWN);

   switch (pf)
	{
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC4_UNORM:
      case DXGI_FORMAT_BC5_UNORM:
      case DXGI_FORMAT_BC7_UNORM:
      case DXGI_FORMAT_BC7_UNORM_SRGB:
         return true;

 		default:
         return false;
	}

   return false;
}



inline uint32 GetBitsPerPixel(PixelFormat pf)
{
	assert(pf != DXGI_FORMAT_UNKNOWN);

   switch (pf)
	{
      case DXGI_FORMAT_R10G10B10A2_UNORM:
      case DXGI_FORMAT_R8G8B8A8_UNORM:
      case DXGI_FORMAT_B8G8R8X8_UNORM:
      case DXGI_FORMAT_B8G8R8A8_UNORM:
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      case DXGI_FORMAT_R32_FLOAT:
         return 32;

      case DXGI_FORMAT_R8G8B8_UNORM:
         return 24;

      case DXGI_FORMAT_B5G5R5A1_UNORM:
      case DXGI_FORMAT_B5G6R5_UNORM:
      case DXGI_FORMAT_B4G4R4A4_UNORM:
      case DXGI_FORMAT_R8A8_UNORM:
         return 16;

      case DXGI_FORMAT_R8_UNORM:
      case DXGI_FORMAT_A8_UNORM:
         return 8;


      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC4_UNORM:
      case DXGI_FORMAT_BC5_UNORM:
      case DXGI_FORMAT_BC7_UNORM:
         // TODO: Fill this lot in.
		default:
			return 0;
	}
}


// Returned as float because compressed formats are fractional.
inline float GetBytesPerPixel(PixelFormat pf)
{
   return (float) (GetBitsPerPixel(pf)) / 8.0f;
}


inline bool Specifies_Alpha(PixelFormat pf)
{
   //assert(pf != DXGI_FORMAT_UNKNOWN);

   // Notes:
   // As DXGI_FORMAT_R8G8_UNORM is used to represent D3DFMT_A8L8 it implicitly contains an alpha channel. Explicitly define if/when two colour components are used. E.g. x/y normal map.

   switch (pf)
   {
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B5G6R5_UNORM:
      case DXGI_FORMAT_R8G8B8_UNORM:
      case DXGI_FORMAT_R8_UNORM:
      case DXGI_FORMAT_R8_SNORM:
      case DXGI_FORMAT_P8:
      case DXGI_FORMAT_UNKNOWN:
         return false;

      default:
         return true;
   }
}



static bool IsGammaCorrected(Infinity::PixelFormat pf)
{
   switch (pf)
   {
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC4_SNORM:  // TODO: Really ?
      case DXGI_FORMAT_BC5_SNORM:  // TODO: Really ?
      //case DXGI_FORMAT_BC6H_SF16:
      case DXGI_FORMAT_BC7_UNORM_SRGB:
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      case DXGI_FORMAT_R8_SNORM:
      //case DXGI_FORMAT_R16G16_SNORM:
      //case DXGI_FORMAT_R16_SNORM:
         return true;
   }

   return false;
}

}


#endif // INF_PIXEL_H
