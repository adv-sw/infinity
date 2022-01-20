/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : diagnostics.cpp

Description : Diagnostics implementation.

License : Copyright (c) 2022, Advance Software Limited.

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


#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "inf_app_diagnostics.h"

#if __ENABLE_DIAGNOSTICS

// Diagnostics

/*
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
*/

wchar_t *log_filename = nullptr;

wchar_t *GetLogFile()
{
   if (!log_filename)
   {
      DWORD process_id = GetProcessId(GetCurrentProcess());

      // Calculate length of the string.
      size_t filename_len = _snwprintf((wchar_t*) nullptr, 0, L"%s_%x.txt", INF_VIDEO_PROCESSOR_LOG_FILE_BASE, process_id);

      log_filename = new wchar_t[filename_len];
      wsprintf(log_filename, L"%s_%x.txt", INF_VIDEO_PROCESSOR_LOG_FILE_BASE, process_id);
   }

   return log_filename;
}


void Log(const wchar_t *message)
{
   if (__log_active)
   {
      FILE *fp = _wfopen(GetLogFile(), L"a");
      fputws(message, fp);
      fputws(L"\n", fp);
      fclose(fp);
   }
}


void Logf(const wchar_t *format, ...)
{
   va_list arg_list;
   va_start(arg_list, format);

   // Do the format once to get the length.
   size_t len = _vscwprintf(format, arg_list);
   wchar_t *s = new wchar_t[len + 1];
   wvsprintf(s, format, arg_list);
   va_end(arg_list);
   Log(s);

   delete s;
}

#endif



void Log_LastError()
{
   DWORD dw = GetLastError();
   wchar_t buffer[1024];

   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      buffer, 1024, NULL);

   Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "%S", buffer);
}


 void NotifyError(PCWSTR msg, HRESULT error)
{
   const size_t MESSAGE_LEN = 256;
   wchar_t message[MESSAGE_LEN];

   if (SUCCEEDED(StringCchPrintf(message, MESSAGE_LEN, L"%s (HRESULT = 0x%X)", msg, error)))
   {
      Infinity_Log(INF_LOG_DEFAULT, 1, "%S", message);
   }
}
