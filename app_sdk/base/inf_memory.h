/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_memory.h

Description : Memory allocation.

License : Copyright (c) 2002 - 2022, Advance Software Limited.

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


#ifndef INF_MEMORY_H
#define INF_MEMORY_H

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#else
// TODO: Is this still required here ?
#include <malloc.h>
#endif

#include "inf_basic_types.h"

#if defined INF_SHARED_API_EXPORTS || defined KERNEL_EXPORTS
#include "inf_call.h"
#else
#ifndef INF_SHARED_API
#define INF_SHARED_API
#endif
#endif

inline void * Global_Alloc(size_t nbytes)
{
#if defined _WIN32 || defined _WIN64

#if  WIN_RT
   return (void*) ::HeapAlloc(::GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, nbytes);
#else
   return (void*) /*Win32*/::GlobalAlloc(GPTR, nbytes);
#endif

#else
    return malloc(nbytes);
#endif
}

inline void Global_Free(void *ptr)
{
#if defined _WIN32 || defined _WIN64

#if  WIN_RT
   ::HeapFree(::GetProcessHeap(), 0, ptr);
#else
   /*Win32*/::GlobalFree((HGLOBAL)ptr);
#endif

#else
  free(ptr);
#endif
}

INF_SHARED_API void *Global_Duplicate(const char *string);


// Memory diagnostics. Currently disabled.

#if 0

#define USE_GLOBAL_ALLOC 1
#define USE_STDLIB 0

#include <new>

#include "windows.h"

#if USE_STDLIB
#include "stdlib.h"
#endif

void * operator new(size_t size)
{
   if (size==0)
      /*Win32*/::MessageBox(nullptr, TEXT("Invalid memory allocation"), TEXT("Infinity"), MB_OK);

#if USE_STDLIB
   void *mem = malloc(size);
#elif USE_GLOBAL_ALLOC
   void *mem = Global_Alloc(size);
#endif

   if (!mem)
   {
      /*Win32*/::MessageBox(nullptr, TEXT("Out of memory"), TEXT("Infinity"), MB_OK);
      /*Win32*/::MessageBeep(1);
   }

   return mem;
}

void operator delete(void *p)
{
   if (p)
   {
#if USE_STDLIB
      free(p);
#elif USE_GLOBAL_ALLOC
      Global_Free(p);
#endif
   }
   else
   {
      /*Win32*/::MessageBox(nullptr, TEXT("Null delete"), TEXT("Infinity"), MB_OK);
      /*Win32*/::MessageBeep(1);
   }
}

#endif

#endif // INF_MEMORY_H
