/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_dialog_info.h

Description : Dialog information definition.

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



#ifndef INF_DIALOG_INFO_H
#define INF_DIALOG_INFO_H

#include <list>

#if defined INF_SHARED_API_EXPORTS || defined KERNEL_EXPORTS
#include "inf_call.h"
#define INF_TMP_API INF_SHARED_API
#else
// External app
#define INF_TMP_API 
#endif


union Value
{
   float    f;
   wchar_t *s;
   void    *ptr;
};


typedef const char * Token;


class Parameters
{
public:
   typedef std::pair<Token, Value> TokenValue;

   INF_TMP_API Parameters() {}
   INF_TMP_API ~Parameters() { Reset(); }

   INF_TMP_API void Reset();
   INF_TMP_API void Insert(Token token, wchar_t *_value);
   INF_TMP_API void Insert(Token token, float value);
   INF_TMP_API bool Get(Token parameter, void *&dest);
   INF_TMP_API bool Get(Token parameter, float &dest);
   INF_TMP_API void Copy(Parameters &src);
   INF_TMP_API std::list<Parameters::TokenValue> &Entries_Get();

private:
   std::list<Parameters::TokenValue> m_entries;
};


class RemoteDialogInfo
{
public:
   RemoteDialogInfo(void *user_data) {
      m_exit_code = 0; m_user_data = user_data; m_open_pending = false;
   }
   void Close(unsigned int exit_code) { m_exit_code = exit_code + 1; }

   Parameters m_parameters;
   void *m_user_data;
   bool m_open_pending;

   // 0  : still processing. Above 0, exited with code n-1.
   // -1 : failed.
   // TODO: Pull out still processing into a seperate flag.
   // There is no need to compress two variables into one here, it's confusing.
   int m_exit_code;
};

#if defined _WIN32 || defined _WIN64
const Infinity::uint32 INF_DIALOG_OPEN = WM_USER + 2;
#endif // WIN32

//}

#endif // INF_DIALOG_INFO_H
