/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_base_types.h

Description : Base types definition.

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


#ifndef INF_BASIC_TYPES_H
#define INF_BASIC_TYPES_H

#if defined _WIN32 || defined _WIN64 || defined WIN64
namespace Infinity
{
   typedef unsigned long    uint32;
   typedef unsigned short   uint16;
   typedef unsigned char    uint8;
   typedef signed long      int32;
   typedef signed short     int16;
   typedef signed char      int8;
};
#else
#include <stdint.h>
namespace Infinity
{
   typedef uint32_t   uint32;
   typedef uint16_t   uint16;
   typedef uint8_t    uint8;
   typedef int32_t    int32;
   typedef int16_t    int16;
   typedef int8_t     int8;
};
#endif


// The following not specifically basic types - TODO - move to more appropriate header.


// TODO: Extend this to perform a dynamic_cast with return code with failure diagnostics for internal builds.
#define INF_CAST(t, v)   *( ( t *) ( v ) )

#define INF_CAST_PTR(t, v) ( ( t *) ( v ) )
#define INF_CAST_CONST_PTR(t, v) ( const  ( t ) ) * ) ( v )


#if defined _WIN32 || defined _WIN64
typedef  void * Desktop_Window;
#else
typedef uint32_t Desktop_Window;   // xcb_window_t is a uint32
#endif

// Button flags
const Infinity::uint32 BFLAGS_FOCUS = 0x0001;
const Infinity::uint32 BFLAGS_DOWN  = 0x8000;

namespace Infinity
{
   typedef Infinity::uint32 Play_Mode;

   // TODO: Put number of times to loop (when enabled) in upper bits. 0=infinite.

   // Behaviour flags ...
   const Play_Mode  Play_Cycles   = 0x0fff;
   const Play_Mode  Play_Once     = 0x0001;  // Lowest 3 nibbles are # cycles
   const Play_Mode  Play_Loop     = 0x1000;  // Loop constantly.
   const Play_Mode  Play_Relative = 0x2000;
   const Play_Mode  Play_Paused   = 0x4000;

   // Default is loop, dropping to single cycle when looping disabled.
   const Play_Mode  Play_Default  = Play_Loop|0x001;

   enum Cursor { Cursor_None = 0, Cursor_Default = 1, Cursor_Busy = 2, Cursor_Caret=3, Cursor_Link = 4 };
}

#endif // INF_BASIC_TYPES_H
