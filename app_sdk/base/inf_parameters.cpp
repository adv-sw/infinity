/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_parameter.cpp

Description : Generic parameter passing.

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



#include "inf_memory.h"
#include "inf_dialog_info.h"

#undef INF_SHARED_API
#define INF_SHARED_API

INF_SHARED_API void Parameters::Reset()
{
   while (!m_entries.empty())
   {
      TokenValue &p = *m_entries.begin();

      if (p.first[0] != '#')
         Global_Free(p.second.s);

      m_entries.pop_front();
   }
}

 
INF_SHARED_API std::list<Parameters::TokenValue> &Parameters::Entries_Get() 
{ 
   return m_entries; 
}


INF_SHARED_API void Parameters::Insert(Token token, wchar_t *_value)
{
   // Duplicate wide string value pointers on the global heap so they can be reallocated as necessary.
   auto len = wcslen(_value) + 1;   // TODO: Do we need the +1 ?
   wchar_t *value = (wchar_t *)Global_Alloc(len * sizeof(wchar_t));
   wcscpy_s(value, len, _value);

   auto len_t = strlen(token) + 1;
   char *param = (char *)Global_Alloc(len_t * sizeof(char));   // TODO: Do we need the +1 ?
   strcpy_s(param, len_t, token);

   TokenValue p;
   p.first = param;
   p.second.s = value;
   m_entries.push_back(p);
}


 INF_SHARED_API void Parameters::Insert(Token token, float value)
{
   auto len_t = strlen(token) + 2;
   char *param = (char *)Global_Alloc(len_t * sizeof(char));   // TODO: Is +1 sufficient ?
   *param = '#';  // Add a '#' prefix to the parameter id to indicate this is a float.
   strcpy_s(param + 1, len_t, token);

   TokenValue p;
   p.first = param;
   p.second.f = value;
   m_entries.push_back(p);
}


INF_SHARED_API bool Parameters::Get(Token parameter, void *&dest)
{
   for (auto i = m_entries.begin(); i != m_entries.end(); ++i)
   {
      Token t = (*i).first;

      if (!strcmp(t, parameter))
      {
         dest = (void*)(*i).second.s;
         return true;
      }
   }

   return false;
}


INF_SHARED_API  bool Parameters::Get(Token parameter, float &dest)
{
   for (auto i = m_entries.begin(); i != m_entries.end(); ++i)
   {
      Token t = (*i).first;

      if (*t != '#')
         continue;

      if (!strcmp(t + 1, parameter))
      {
         dest = (*i).second.f;
         return true;
      }
   }

   return false;
}


INF_SHARED_API void Parameters::Copy(Parameters &src)
{
   for (auto i = src.m_entries.begin(); i != src.m_entries.end(); ++i)
   {
      Token t = (*i).first;

      if (*t == '#')
         Insert(t, (*i).second.f);
      else
         Insert(t, (*i).second.s);
   }
}