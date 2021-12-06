/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_process_info.cpp

Description : Windows platform process query implementation.

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


// TODO: Move to process_windoze.cpp

#include <tlhelp32.h>
#include <psapi.h>

bool GetParentProcessID(uint32_t process_id, uint32_t &parent_process_id)
{
   parent_process_id = 0;
   bool found = false;

   HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

   if (!h)
      return false;

   PROCESSENTRY32 pe = { 0 };
   pe.dwSize = sizeof(PROCESSENTRY32);

   if (Process32First(h, &pe)) 
   {
        do {
            if (pe.th32ProcessID == process_id) 
            {
                parent_process_id = pe.th32ParentProcessID;
                found = true;
                break;
            }
        } while( Process32Next(h, &pe));
   }

   CloseHandle(h);

   return found;
}


bool GetProcessInfo(DWORD process_id, CHAR *dest, size_t len)
{
   uint32_t parent_process_id = 0;
   bool found = false;

   HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

   if (!h)
      return false;

   PROCESSENTRY32 pe = { 0 };
   pe.dwSize = sizeof(PROCESSENTRY32);

   if (Process32First(h, &pe)) 
   {
        do {
            if (pe.th32ProcessID == process_id) 
            {
                parent_process_id = pe.th32ParentProcessID;
                strncpy(dest, pe.szExeFile, len);
                found = true;
                break;
            }
        } while( Process32Next(h, &pe));
   }

   CloseHandle(h);

   return found;
}
