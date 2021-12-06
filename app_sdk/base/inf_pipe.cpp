/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_pipe.cpp

Description : IPC communications pipe.

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



#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif


// More than you ever wanted to know about windoze pipes here :
// https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipe-server-using-overlapped-i-o


#include "inf_pipe.h"
#include <mutex>
#include <string>

using namespace Infinity;

const size_t pipe_buffer_size = 65536;

// IMPORTANT: Message mode is essential to ensure stability. Don't use byte mode as not clear how to predictably flush.
const uint32_t  INF_PIPE_READ_MODE = PIPE_READMODE_MESSAGE;
const uint32_t  INF_PIPE_TYPE = PIPE_TYPE_MESSAGE;

static std::mutex __send_msg_mutex;
#define pipe_assert(x,msg) assert(x)

#define PIPE_DIAGNOSTICS 0

#ifdef INFINITY_PLUGIN_EXPORTS

#if PIPE_DIAGNOSTICS
#include <inf_diagnostics.h>
#define PIPE_LOG(...) Infinity::Logf(__VA_ARGS__)
#else
#define PIPE_LOG(...) 
#endif // PIPE_DIAGNOSTICS

#else

#if PIPE_DIAGNOSTICS
#include "../core/inf_app_diagnostics.h"
#define PIPE_LOG(...) Infinity_Log(3, __VA_ARGS__)
#else
#define PIPE_LOG(...)
#endif // PIPE_DIAGNOSTICS

#endif // INFINITY_PLUGIN_EXPORTS


#include "..\core\inf_interprocess_msg.h"


Pipe::Pipe()
{
   m_handle = nullptr;
   m_id = -1;

   m_async = true;

   if (m_async)
   {
      // Windows async pipe requirement.
      memset(&m_overlapped, 0, sizeof(m_overlapped));

      // Create an event object for this instance. 

      m_overlapped.hEvent = ::CreateEvent(
         NULL,    // default security attribute 
         TRUE,    // manual-reset event 
         TRUE,    // initial state = signaled 
         NULL);   // unnamed event object 

      if (!m_overlapped.hEvent)
      {
         PIPE_LOG("CreateEvent failed.); // with %d.\n", GetLastError());
      }
   }

   m_live = true;
   m_server = false;
}


Pipe::~Pipe()
{
   Close();
}


void Pipe::Close()
{
   if (m_handle)
   {
      if (m_server)
         /*win_api*/::FlushFileBuffers(m_handle);
      else
         Ensure_Connection();

      /*win_api*/::DisconnectNamedPipe(m_handle);
      /*win_api*/::CloseHandle(m_handle);

      m_handle = nullptr;
   }
}


bool Pipe::Create(size_t uid)
{
   Close();

   m_id = uid;

   m_server = true;

   const UINT __create_timeout = 100000; // Oculus firing up can slow us right now, so be patient.

   // Overlapped named pipes deliver asynchronous non-stalling communications. Our implementation could go further, but this will do for now.
   std::string full_id = std::string(INF_PIPE_ID) + std::to_string(m_id);

   size_t flags = PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE; // Fail if this pipe already exists.
   
   if (m_async)
      flags |= FILE_FLAG_OVERLAPPED;

   m_handle = ::CreateNamedPipeA(full_id.c_str(),
        flags, INF_PIPE_TYPE | INF_PIPE_READ_MODE | PIPE_WAIT,

        1, // Only allow one instance of this pipe

        pipe_buffer_size, pipe_buffer_size, __create_timeout, nullptr);


   if (m_handle == INVALID_HANDLE_VALUE)
   {
      if (::GetLastError() == ERROR_ACCESS_DENIED)
      {
         PIPE_LOG("CreateNamedPipe access denied - probably already exists.");
      }
      else
         PIPE_LOG("CreateNamedPipe failed in Pipe::Create.");
      
      Close();

      return false;
   }

   PIPE_LOG("[ok] CreateNamedPipeA: %s", full_id.c_str());

   return true;
}


bool Pipe::Ensure_Connection()
{
  if (::ConnectNamedPipe(m_handle, m_async  ? &m_overlapped : nullptr))
  {
     if (m_async)
     {
        PIPE_LOG("Unexpected, overlapped ConnectNamedPipe() always returns 0.");
        return false;
     }
  }

  if (m_async)
  {
     switch (::GetLastError())
     {
        case ERROR_IO_PENDING:
           PIPE_LOG("ConnectNamedPipe ... pending.");
           WaitForSingleObjectEx(m_overlapped.hEvent, INFINITE, TRUE);
           PIPE_LOG("ConnectNamedPipe ... done.");
           return true;

        case ERROR_PIPE_CONNECTED:
        {
           ::SetEvent(m_overlapped.hEvent);
           PIPE_LOG("ConnectNamedPipe ... connected.");
           return true;
        }

        default:
           // Error
           PIPE_LOG("ConnectNamedPipe failed: %d", GetLastError());
           return false;
        }
  }

   return true;
}


bool Pipe::Connect()
{
   PIPE_LOG("Pipe::Connect[1]");
   
   std::string full_id = std::string(INF_PIPE_ID) + std::to_string(m_id);
   bool timeout = false;
   uint32 counter = 0;

   // Open the named pipe

   for (;;)
   {
      m_handle = ::CreateFileA(full_id.c_str(), 
         GENERIC_READ | GENERIC_WRITE, 
         FILE_SHARE_READ|FILE_SHARE_WRITE, 
         nullptr, OPEN_EXISTING,
         m_async ? FILE_FLAG_OVERLAPPED : 0
         ,nullptr);
       
      if (m_handle != INVALID_HANDLE_VALUE)
      {
         PIPE_LOG("Pipe::Connect[handle]");
         break;
      }

      auto error = GetLastError();

      if (error == ERROR_FILE_NOT_FOUND)
      {
         // Try again, server might not have a connection at this point.
         PIPE_LOG("// Pipe not found. Searching ... '%s'", full_id.c_str());
         counter++;
         Sleep(100);
      }

      else if (error == ERROR_PIPE_BUSY)
      {
          PIPE_LOG("// Pipe busy. Waiting ... '%s'", full_id.c_str());
          
          // Wait for pipe to become available ...
          auto rc = ::WaitNamedPipeA(full_id.c_str(), 20000);  // Wait max 20 seconds for a connect.

          if (!rc)
          {
             PIPE_LOG("Could not connect for 20 seconds. Pipe timeout.");
             return false;
          }
      }
      else
      {
         PIPE_LOG("Client pipe open error ['%s'] : %d ", full_id.c_str(), error);
         Close();
         return false;
      }

      if (counter > 100)
      {
         timeout = true;
         break;
      }
   }

   if (timeout)
   {
      PIPE_LOG("Pipe::Connect timeout : '%s'", full_id.c_str());
      Close();
      return false;
   }

   
#if 0
   DWORD pipe_mode = INF_PIPE_READ_MODE; 

   PIPE_LOG("Pipe::SetNamedPipeHandleState[A]");
   
   if (!::SetNamedPipeHandleState(m_handle, &pipe_mode, nullptr, nullptr))
   {
      PIPE_LOG("SetNamedPipeHandleState error %d", GetLastError());
      Close();
      return false;
   }

   PIPE_LOG("Pipe::SetNamedPipeHandleState[B]");
   PIPE_LOG("Pipe::Connect[WaitForSingleObjectEx] A");
   WaitForSingleObjectEx(m_overlapped.hEvent, INFINITE, TRUE);
   PIPE_LOG("Pipe::Connect[WaitForSingleObjectEx] B");
#endif


   // Tell server we're ready ...
   bool ok = Message_Send(INFINITY_APP_CONNECTED, 0, nullptr, 0, false);

   if (ok)
      PIPE_LOG("<><> Client connected !");
   else
      PIPE_LOG("*** Client connect failed !");

   return ok;
}


bool Pipe::Available(size_t required)
{
   HANDLE handle = m_handle;

   if (!handle)
   {
      // This happens after closing a pipe. Logging disabled as not needed right now.
      //PIPE_LOG("Pipe::Available : no handle !");
      return false;
   }

   DWORD available = 0;
   m_live = ::PeekNamedPipe(handle, nullptr, 0, nullptr, &available, nullptr);

   return m_live && (available >= required);
}


static void System_Processing()
{
   MSG msg;
   size_t counter = 0;

   while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
   {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);

      if (++counter > 100) // Max messages to process per cycle.
         break;
   }
}


bool Pipe::Read(void *dest, size_t required)
{
   HANDLE handle = m_handle;
   
   if (!handle)
   {
      PIPE_LOG("Pipe invalid in Pipe::ReadMessage");
      return false;
   }

   while (!Available(required))
   {
#if 0
      // This makes system unresponsive. If it's found to be required, ensure minimum occurs per loop.
      System_Processing();
 
      ::SwitchToThread();
#endif

      if (!m_live)
         return false;
   }

   DWORD bytes_read = 0;
  

    // Read from pipe...
    BOOL ok = ::ReadFile(handle, dest, (DWORD) required, &bytes_read, m_async ? &m_overlapped : nullptr);

    if (!ok)
    {
       if (m_async)
       {
          DWORD err = ::GetLastError();

          if (err == ERROR_IO_PENDING)
          {
             // TODO[OPT]: Busy wait in overlapped mode, could come back to it later, recording what we've got so far.
                 // [OPT] Revisit for asynchronous completion if it ever seems worth the effort.

             ::WaitForSingleObject(m_overlapped.hEvent, INFINITE);

             ok = ::GetOverlappedResult(handle, &m_overlapped, &bytes_read, TRUE);
             return ok;
          }

          if (err != ERROR_BROKEN_PIPE)
          {
             PIPE_LOG("Broken pipe.");
             return false;
          }
       }
   }

   return ok;
}


bool Pipe::Message_Send(Cmd_ID id, Cmd_Target t, void *data, size_t len, bool write_len)
{
   // Mutex to ensure multiple threads can't overwrite each other.
   __send_msg_mutex.lock();

   Cmd_Header h;
   h.id = id;
   h.target = t;
   
   bool fail = !Write(&h, sizeof(h));

   if (write_len)
   {
      Infinity::uint32 _len_32 = (uint32_t)len;
      fail |= !Write(&_len_32, sizeof(uint32_t));
   }

   if (data)
      fail |= !Write(data, len);

   __send_msg_mutex.unlock();

   return !fail;
}


bool Pipe::Write(const void *buffer, size_t len)
{
   DWORD bytes_written = 0;
   HANDLE handle = m_handle;
   bool ok = !!::WriteFile(handle, buffer, (DWORD) len, &bytes_written, m_async ? &m_overlapped : nullptr);

   if (!ok)
   {
      auto err = GetLastError();

      if (err == ERROR_IO_PENDING)
      {
         if (m_async)
         {
            // For now, we busy wait for the operation to complete.
            ::WaitForSingleObject(m_overlapped.hEvent, INFINITE);

            // [OPT] Revisit for asynchronous completion if it ever seems worth the effort.
            ::GetOverlappedResult(handle, &m_overlapped, &bytes_written, TRUE);
         }
         else
         {
             PIPE_LOG("Unexpected pending on sync pipe.");
             return false;
         } 
      } 
      else if (err == ERROR_NO_DATA)
      {
          PIPE_LOG("Pipe WriteFile: ERROR_NO_DATA");
          return false;
      }
      else if (err == ERROR_INVALID_HANDLE)
      {
         DWORD pid = GetCurrentProcessId();
         PIPE_LOG("Process_ID: %d   Invalid handle: %x", pid, handle);
         return false;
      }
      else
      {
         PIPE_LOG("Pipe WriteFile error : %d", err);
         return false;
      }
   }

   if (bytes_written < len)
   {
      size_t remainder = len - bytes_written;
      PIPE_LOG("Writing remainder ...");
      return Write((uint8*)buffer+remainder, remainder);
   }

   return ok;
}


// Caller must delete [] returned string.
char *Pipe::ReadString()
{
   uint32 len = 0;
   Read(&len, sizeof(len));

   const size_t num_chars = len / sizeof(char);
   char *str = new char[num_chars + 1];

   Read(str, len);
   str[num_chars] = '\0';

   return str;
}


// Caller must delete [] returned string.
wchar_t *Pipe::ReadWString()
{
   uint32 len = 0;

   bool ok = Read(&len, sizeof(len));

   auto num_chars = len / sizeof(wchar_t);
   wchar_t *str = new wchar_t[num_chars + 1];

   ok = Read(str, len);

   if (ok)
      str[num_chars] = '\0';
   else
   {
      delete [] str;
      str = nullptr;
   }

   return str;
}


void Parameters_Receive(Pipe *pipe, ::Parameters &parms)
{
   uint32 nparams;
   pipe->Read(&nparams, sizeof(nparams));

   for (uint32 i = 0; i < nparams; i++)
   {
      char *token = pipe->ReadString();
      wchar_t *value = pipe->ReadWString();
      parms.Insert(token, value);

      delete [] token;
      delete [] value;
   }
}


// Duplicate kernel side used to send back final dialog state.
// TODO: Keep in sync & factor both to common code.
void Parameters_Send(Pipe *pipe, Cmd_Target t, Cmd_ID cmd_id, ::Parameters *params)
{
   size_t len = sizeof(uint32);  // Number of parameters.

   if (params)
   {
      const std::list<Parameters::TokenValue> &entries = params->Entries_Get();

      // Calculate storage requirements for dialog parameters
      for (auto i = entries.begin(); i != entries.end(); ++i)
      {
         auto p = *i;
         len += sizeof(uint32); // strlen
         len += strlen(p.first) * sizeof(char);

         len += sizeof(uint32);  // strlen
         len += wcslen(p.second.s) * sizeof(wchar_t);
      }
   }

   // Allocate ...
   void *data = (unsigned char*) malloc(len);
   auto ptr = (char *) data;

   // Compile ...

   const std::list<Parameters::TokenValue> &entries = params->Entries_Get();

   *((Infinity::uint32*)ptr) = Infinity::uint32(params ? entries.size() : 0);
   ptr += sizeof(Infinity::uint32);

   if (params)
   {
      for (auto i = entries.begin(); i != entries.end(); ++i)
      {
         auto p = *i;

         auto slen = strlen(p.first);
         *((Infinity::uint32*)ptr) = Infinity::uint32(slen);
         ptr += sizeof(Infinity::uint32);
         strcpy(ptr, p.first);
         ptr += slen;

         slen = wcslen(p.second.s);
         *((Infinity::uint32*)ptr) = Infinity::uint32(slen * sizeof(wchar_t));
         ptr += sizeof(Infinity::uint32);
         wcscpy((wchar_t*)ptr, p.second.s);
         ptr += slen * sizeof(wchar_t);
      }
   }

   // Send
   Cmd_Header h;
   h.id = cmd_id;
   h.target = t;
   pipe->Write(&h, sizeof(h));
   pipe->Write(data, len);

   // Cleanup ...
   free(data);
}


size_t Strings_Serialize(uint8 *&dest, const char **str, uint32 num_strings, size_t additional_required)
{
   size_t str_len = 0;
   std::list<size_t> str_lengths;

   for (uint32 c = 0; c < num_strings; c++)
   {
      auto slen = strlen(str[c]);
      str_len += /*strlen*/ sizeof(uint32) + /*string*/ slen;
      str_lengths.push_back(slen);
   }

   size_t cmd_len = str_len + additional_required;
   dest = (uint8 *) malloc(cmd_len);

   uint8* ptr = dest;

   for (uint32 c = 0; c < num_strings; c++)
   {
      size_t slen = str_lengths.front();
      str_lengths.pop_front();
      *((uint32*)ptr) = Infinity::uint32(slen); // length of string.
      ptr += sizeof(uint32);

      memcpy(ptr, str[c], slen);  // string itself.
      ptr += slen;
   }

   return str_len;
}
