/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Mozilla App

File        : adv_sw_patch.h

Description : Dynamic patching determination implementation.

License : Copyright (c) 2021, Advance Software Limited.

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


#ifndef ADV_SW_PATCH_H
#define ADV_SW_PATCH_H

// For getenv
#include <stdlib.h>     

static bool __patch_initialized = false;
static bool __is_patch_active = false;


// Note: Can't use Windows API in this file as including windows headers
// currently results in link errors due to macros renaming functions.
// Could #undef offending macros if we must have windows.h here.

inline char *AdvGetValue(const char *id)
{
   return getenv(id);
}


inline bool _Gecko_OffscreenSharedSurfaceMode()
{
   if (!__patch_initialized)
   {
      auto value = AdvGetValue("MOZ_GECKO_SERVER");
      
	  __is_patch_active = value && (value[0] != '\0');
	  
      __patch_initialized = true;
   }

   return __is_patch_active;
}

#endif // ADV_SW_PATCH_H
