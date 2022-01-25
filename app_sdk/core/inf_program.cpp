/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_program.cpp

Description : Program base class implementation.

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


#define __ENABLE_DEBUG_CATCH_MSG_BOX 0

// Stop Visual Studio whining about unsafe string operations for now.
#pragma warning (disable : 4996)

#include <windows.h>
#include <wchar.h>
#include <strsafe.h>
#include <string>
#include <iostream>
#include <fstream>
#include <map>

// Ensure no infinity kernel side dependencies.
// TODO: Clean up headers so not required.
#define INF_UTIL_H 

#include "inf_interprocess_msg.h"
#include "..\base\inf_pipe.h"

#include "app_info.h"

#include "inf_app.h"
#include "inf_app_diagnostics.h"

// For codecvt_utf8_utf16
#include <locale>
#include <codecvt>
#include <string>

//#include "..\base\inf_util.h"

#define INF_VERBOSE   0
#define INF_LOG_COMMS 0

extern const char* __program_id;


// nVidia optimus magic sauce to bypass win10 motherboard GPU default in windoze 10.
extern "C" {
   _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int __instance_id = -1;

static bool __closing_orphan = false;


static std::string _inf_app_log;
static bool __app_log_var_checked = false;
void _cdecl Infinity_Log(uint32_t log_type, uint32_t log_id, const char *format, ...)
{
   // Only check env var once per run
   if (!__app_log_var_checked)
   {
      const char* value = getenv("INF_APP_LOG");

      // Grab from env variable, if set.
      if (value)
         _inf_app_log.assign(value);

      __app_log_var_checked = true;

      std::remove(_inf_app_log.c_str());
   }

   va_list arg_list;
   va_start(arg_list, format);

#if defined _WIN32 || defined _WIN64
   int len = _vscprintf(format, arg_list);
#else
   char* tmp = NULL;
   // TODO[OPT]: Use the string returned by vasprintf directly rather than creating a second copy.
   int len = vasprintf(&tmp, format, arg_list);
   free(tmp);
#endif

   char* content_string = new char[len + 1];

   vsprintf(content_string, format, arg_list);

   if (log_type & INF_LOG_PARENT)
   {
      size_t additional = sizeof(log_id);

      const char *param = content_string;
      uint8_t* data;

      // Create data buffer sufficient for string & id.
      size_t slen = Strings_Serialize(data, &param, 1, additional);

      // Drop additional after the string
      uint32_t * ptr = (uint32_t*)(data + slen);
      *ptr = log_id;

      Msg_Parent(INFINITY_LOG, 0, data, slen + additional);

      // TODO: How does "data" get zapped here & elsewhere ?
   }


   if ( (log_type & INF_LOG_FILE) && !_inf_app_log.empty())
   {
#if 1
      std::ofstream outfile;

      outfile.open(_inf_app_log, std::ios_base::app); // append 

      // TODO: Add time.
      if (__instance_id >= 0)
      {
         char tmp[10];
         sprintf(tmp, "  [%d]", __instance_id);
         outfile << tmp;
      }
      
      outfile << content_string << "\n";
#else
      /*Win32*/::OutputDebugStringA(content_string);
#endif
   }

   delete [] content_string;
}



// ---------------------------- Cmd_Target- ----------------------------[BEGIN]

std::map<Cmd_Target, Infinity::App*> __lookup;
typedef std::pair<size_t, Infinity::App*> App_Instance;

extern "C" Infinity::App * __cdecl App_CreateInstance();

Infinity::App* __cdecl Infinity::Find(Cmd_Target t, Infinity::uint32 op)
{
   Infinity::App* app = nullptr;

   auto x = __lookup.find(t);

   if (op == FIND_CREATE)
   {
      bool new_entry = (x == __lookup.end());

      if (new_entry)
      {
         app = App_CreateInstance();
         app->m_instance_id = t;

         __lookup.insert(App_Instance(t, app));

         if (!app->Initialize())
         {
            delete app;
            app = nullptr;
         }
      }
   }

   if (!app)
   {
      if (x != __lookup.end())
      {
         auto entry = *x;
         app = entry.second;
      }
   }

   return app;
}


void Remove(Cmd_Target t)
{
   __lookup.erase(t);
}


// ---------------------------- Cmd_Target- ----------------------------[END]

// TODO[OPT]: Make this a singleton.
Program __program;


bool Parent_Ready()
{
   return __program.m_pipe && __program.m_ipc_ready;
}


bool Msg_Parent(Cmd_ID cmd_id, Cmd_Target t, void* data, size_t len, bool write_len)
{
   // TODO: Pipe writes complete asynchronously, so release of payloads needs to be when we know sent.
   // How do we do this ? ** IMPORTANT ** - will lead to instablilities if not done properly.

   if (Parent_Ready())
   {
#if INF_LOG_COMMS
      Infinity_Log(INF_LOG_FILE, 0, "cmd_id: %d  payload: %d", cmd_id, len);
#endif

      __program.m_pipe->Message_Send(cmd_id, t, data, len, write_len);
      return true;
   }

   Infinity_Log(INF_LOG_FILE, 0, "Client pipe trying to send when not ready !");

   return false;
}


using namespace Infinity;



static bool __active = true;
bool __log_active = false;
const size_t __magic = 0xf00d40b1;

extern const char* __program_id;

// App specific implementation.
void App_OnVisible(Cmd_Target t, unsigned char new_val);
void App_Open(Cmd_Target t, wchar_t* url);
void App_Pause(Cmd_Target t);
void App_Exclusive_Requested(Cmd_Target t, bool enable);
void App_Redraw(Cmd_Target t);
void App_Play(Cmd_Target t);

void App_OnDelete(Cmd_Target t);

// TODO: Why the Cmd_Target here - this shuts down the app, not an instance. Review.
void App_Terminate(Cmd_Target t, bool normal);

void App_SetVolume(Cmd_Target t, float volume);
void App_SetButtonState(Cmd_Target t, const Button_State& value);
void App_System_Message(Cmd_Target t, const System_Message& value);
void App_OnFocus(Cmd_Target t, bool value);
void App_OnCursor(Cmd_Target t, bool value);
void App_Dialog_Close(Cmd_Target t, ::Parameters& params, int exit_code);

void App_SetCursorCoordinates(Cmd_Target t, const Vector2& tc);
void App_OnVerticalScroll(Cmd_Target t, float value);

void App_Telemetry_Update(Cmd_Target t, const Msg_Telemetry& msg);

void App_Audio_SetStatus(Cmd_Target t, uint8_t value);
void App_Audio_Request(Cmd_Target t, uint8_t value);

bool App_CmdLine_Parameter_Consume(const char* token, const char* value);
bool App_Operation(Infinity::uint32 operation_id, std::string& result, char* param_1);


Pipe* Program_GetPipe()
{
   return __program.m_pipe;
}


static uint32 _GetProcessorCount()
{
   SYSTEM_INFO si;
   memset(&si, 0, sizeof(SYSTEM_INFO));
   /*Win32*/ ::GetSystemInfo(&si);
   return si.dwNumberOfProcessors;
}


void Notify_Format_Changed(Cmd_Target t)
{
   if (Parent_Ready())
   {
      Msg_Parent(INFINITY_FORMAT_CHANGED, t, nullptr, 0, false);
   }
}


void Present(uint32 surface_handle, float frame_time, Cmd_Target t)
{
   if (Parent_Ready())
   {
      Display_Frame f;
      f.surface_handle = surface_handle;
      f.timestamp = frame_time;

      Msg_Parent(INFINITY_DELIVER_FRAME, t, &f, sizeof(f));
   }
}


void Overlay(uint32 surface_handle, RECT& r, Cmd_Target t)
{
   Display_Overlay msg;
   msg.dest = r;
   msg.surface_handle = surface_handle;

   Msg_Parent(INFINITY_DISPLAY_OVERLAY, t, &msg, sizeof(msg));
}

/*
 
// TODO: Combine these into OnParentEvent as required.

{
   if (msg->id == INF_MESSAGE_SYSTEM)
   {
      uint8 *ptr = (uint8*) msg->data;

      uint32 msg_id = (uint32) *((uint32*)ptr);
      ptr += sizeof(uint32);
      size_t wparam = *((size_t*)ptr);
      ptr += sizeof(size_t);
      size_t lparam = *((size_t*)ptr);

      ProcessSystemMessage(msg_id, wparam, lparam);

#if INF_VERBOSE
      Infinity_Log(INF_LOG_FILE, INF_INFO + 1, "INF_MESSAGE_SYSTEM");
#endif
   }

   else if (msg->id == INF_MESSAGE_IDENTIFIER_CHANGED)
      OnIdentifierChanged();

   else if (msg->id == INF_MESSAGE_FILENAME_CHANGED)
      OnFilenameChanged();

   else if (msg->id == INF_MESSAGE_BUTTON_UP)
   {
      OnButtonUp(*INF_CAST_PTR(uint32, &msg->data));

#if INF_VERBOSE
      Infinity_Log(INF_LOG_FILE, INF_INFO + 1, "INF_MESSAGE_BUTTON_UP");
#endif
   }
   else if (msg->id == INF_MESSAGE_BUTTON_DOWN)
   {
      void **data = INF_CAST_PTR(void *, msg->data);
      OnButtonDown((uint32) (size_t) data[0], (uint32)  (size_t) data[1]);

#if INF_VERBOSE
      Infinity_Log(INF_LOG_FILE, INF_INFO+1, "INF_MESSAGE_BUTTON_DOWN");
#endif
   }

   else if (msg->id == INF_MESSAGE_SCROLL_VERTICAL)
   {
      OnVerticalScroll(*INF_CAST_PTR(float, &msg->data));
   }

   else if (msg->id == INF_MESSAGE_STOP)
      OnStop();

   else if (msg->id == INF_MESSAGE_SET_PLAYBACK_MODE)
      SetPlaybackMode(*INF_CAST_PTR(uint32, &msg->data));

   else if (msg->id == INF_MESSAGE_FOCUS)
   {
      OnFocus(); //msg->data);

#if INF_VERBOSE
      Infinity_Log(INF_LOG_FILE, INF_INFO + 1, "INF_MESSAGE_FOCUS");
#endif
   }
   else if (msg->id == INF_MESSAGE_FOCUS_LOST)
   {
      OnFocusLost(); //msg->data);

#if INF_VERBOSE
      Infinity_Log(INF_LOG_FILE, INF_INFO + 1, "INF_MESSAGE_FOCUS_LOST");
#endif
   }
   else if (msg->id == INF_MESSAGE_GOTO_PREV)
      GoBack();

   else if (msg->id == INF_MESSAGE_GOTO_NEXT)
      GoForwards();

   else if (msg->id == INF_MESSAGE_REFRESH)
      Reload();

   else if (msg->id == INF_MESSAGE_PRINT)
      OnPrint();

   else if (msg->id == INF_MESSAGE_SET_ASPECT)
   {
      SetAspectRatio(*(float*)&msg->data);
   }
   else if (msg->id == INF_MESSAGE_SET_DESIRED_WIDTH)
   {
      if (msg->data)
         SetDesiredWidth((uint32) (size_t) msg->data);
   }
   else if (msg->id == INF_MESSAGE_SET_DESIRED_HEIGHT)
   {
      if (msg->data)
         SetDesiredHeight((uint32) (size_t) msg->data);
   }
}
*/

static bool OnParentEvent(const Cmd_Header& cmd)
{
#if INF_VERBOSE

   // Filtered out coz loads of these, which get in the way of spotting what's going on.
   bool filter = (cmd.id == INFINITY_APP_TELEMETRY) || (cmd.id == INFINITY_APP_CURSOR_POSITION);

   if (!filter)
      Infinity_Log(INF_LOG_FILE, INF_INFO, "OnParentEvent: %s", Parent_Message_ID(cmd.id));

#endif

   if (cmd.id == INFINITY_APP_TELEMETRY)
   {
      Msg_Telemetry msg_data;
      __program.m_pipe->Read(&msg_data, sizeof(msg_data));
      App_Telemetry_Update(cmd.target, msg_data);
      return true;
   }
   else if (cmd.id == INFINITY_APP_CURSOR_POSITION)
   {
      Vector2 value;
      __program.m_pipe->Read(&value, sizeof(value));
      App_SetCursorCoordinates(cmd.target, value);

#if INF_VERBOSE
      Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "Cursor: %0.3f %0.3f", value.x, value.y);
#endif

      return true;
   }
   else if (cmd.id == INFINITY_APP_AUDIO_REQUEST)
   {
      uint8_t value;
      __program.m_pipe->Read(&value, sizeof(value));
      App_Audio_Request(cmd.target, value);
      return true;
   }
   else if (cmd.id == INFINITY_APP_AUDIO_STATUS)
   {
      uint8_t value;
      __program.m_pipe->Read(&value, sizeof(value));
      App_Audio_SetStatus(cmd.target, value);
      return true;
   }
   else if (cmd.id == INFINITY_APP_WHEEL_STATE)
   {
      float value;
      __program.m_pipe->Read(&value, sizeof(value));
      App_OnVerticalScroll(cmd.target, value);
      return true;
   }
   else if (cmd.id == INFINITY_APP_VISIBLE)
   {
      uint8 value;
      __program.m_pipe->Read(&value, sizeof(value));
      App_OnVisible(cmd.target, value);
      return true;
   }
   else if (cmd.id == INFINITY_APP_DIALOG_COMPLETE)
   {
      ::Parameters params;
      Parameters_Receive(__program.m_pipe, params);

      int exit_code = 1; // for now.

      App_Dialog_Close(cmd.target, params, exit_code);

      return true;
   }
   else if (cmd.id == INFINITY_APP_OPEN)
   {
      wchar_t* str = __program.m_pipe->ReadWString();

      Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "Opening[%d] : %S", cmd.target, str);

      __instance_id = cmd.target;

      App_Open(cmd.target, str);

      delete[] str;
      return true;
   }

   else if (cmd.id == INFINITY_APP_THEME_CHANGE)
   {
      auto str = __program.m_pipe->ReadString();

      std::string result; // dummy - nothing is returned.

      App_Operation(INFINITY_APP_THEME_CHANGE, result, str);

      // Clean up.
      if (str)
         delete[] str;

      return true;
   }

   else if (cmd.id == INFINITY_APP_COOKIE_ASSIGN)
   {
      auto cookie = __program.m_pipe->ReadString();
      auto _value = __program.m_pipe->ReadString();

      std::string value(_value);

      App_Operation(INFINITY_APP_COOKIE_ASSIGN, value, cookie);

      // Clean up.
      if (cookie)
         delete[] cookie;

      if (_value)
         delete[] _value;

      return true;
   }

   else if (cmd.id == INFINITY_APP_COOKIE_QUERY)
   {
      auto str = __program.m_pipe->ReadString();

      void* handle;
      __program.m_pipe->Read(&handle, sizeof(handle));

      std::string result;

      if (App_Operation(INFINITY_APP_COOKIE_QUERY, result, str))
      {
         size_t additional = sizeof(handle);

         const char* param = result.c_str();
         uint8* data;

         // Create data buffer containing strings with space for handle at the end.
         size_t slen = Strings_Serialize(data, &param, 1, additional);

         // Drop handle in after the strings
         void** ptr = (void**)(data + slen);
         *ptr = handle;

         Msg_Parent(INFINITY_APP_COOKIE_VALUE, 0, data, slen + additional);
      }

      // Clean up.

      if (str)
         delete[] str;

      return true;
   }
   else if (cmd.id == INFINITY_APP_EXCLUSIVE)
   {
      uint8 enable;
      __program.m_pipe->Read(&enable, sizeof(enable));
      App_Exclusive_Requested(cmd.target, enable);
   }
   else if (cmd.id == INFINITY_APP_PAUSE)
   {
      App_Pause(cmd.target);
      return true;
   }
   else if (cmd.id == INFINITY_APP_REDRAW)
   {
      App_Redraw(cmd.target);
      return true;
   }
   else if (cmd.id == INFINITY_APP_PLAY)
   {
      App_Play(cmd.target);
      return true;
   }
   else if (cmd.id == INFINITY_APP_VOLUME)
   {
      float value;
      __program.m_pipe->Read(&value, sizeof(value));

      App_SetVolume(cmd.target, value);
      return true;
   }
   else if (cmd.id == INFINITY_APP_FOCUS)
   {
      bool value;
      __program.m_pipe->Read(&value, sizeof(value));

      App_OnFocus(cmd.target, value);
      return true;
   }
   else if (cmd.id == INFINITY_APP_CURSOR)
   {
      bool value;
      __program.m_pipe->Read(&value, sizeof(value));

      App_OnCursor(cmd.target, value);
      return true;
   }
   else if (cmd.id == INFINITY_APP_BUTTON_STATE)
   {
      Button_State value;
      __program.m_pipe->Read(&value, sizeof(value));
      App_SetButtonState(cmd.target, value);
      return true;
   }

   else if (cmd.id == INFINITY_APP_SYSTEM_MSG)
   {
      System_Message value;
      __program.m_pipe->Read(&value, sizeof(value));

      App_System_Message(cmd.target, value);
      return true;
   }

   else if (cmd.id == INFINITY_APP_EXPIRE_INSTANCE)
   {
      App_OnDelete(cmd.target);
      return true;
   }

   else if (cmd.id == INFINITY_APP_TERMINATE)
   {
      Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "App_Terminate");
      App_Terminate(cmd.target, true);
      return true;
   }

   // Unknown parent event ...

   return false;
}


void _cdecl Program_Terminate()
{
   // Finally zap pipe. 
   ::SwitchToThread();

   // Communications channels are now closed - parent will close the native pipe once it receives the ack above.
   // Everything we send thru the pipe must have completed by now or who knows what will happen.
   // TODO: Ensure flushed.
   if (__program.m_pipe)
   {
      delete __program.m_pipe;
      __program.m_pipe = nullptr;
   }
}


void __cdecl Program_Complete()
{
   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "App_Exit[1]");

   if (!__closing_orphan)     // Can't msg parent if it no longer exists.
   {
      Msg_Parent(INFINITY_APP_EXIT, 0, 0, 0);
   }

   // TODO: Ensure Program_Terminate only called once above pipe write has completed.
}




const char* INF_MAGIC = "-infinity";

void trim_leading(std::string& str)
{
   const auto pos = str.find_first_not_of(" \t");

   if (pos == std::string::npos)
      str.clear();
   else
      str.erase(0, pos);
}


static bool CmdLine_Parse(std::string cline)
{
   while (!cline.empty())
   {
      trim_leading(cline);

      if (cline.empty())
         break;

      if (cline.at(0) != '-')
      {
         Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "Invalid cmd_param");
         break;
      }


      // Process multi-character parameters ...


      if (!cline.compare(0, strlen(INF_MAGIC), INF_MAGIC))
      {
         cline.erase(0, strlen(INF_MAGIC));
         Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "<><> infinity app.");
         continue;
      }


      // Process single character parameters ...

    
      if ((cline.length() < 3 || cline.at(2) != '='))
      {
         Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "Invalid cmd_param");
         break;
      }

      // Remove the -
      cline.erase(0, 1);

      std::string token(1, cline.at(0));

      // Remove token=
      cline.erase(0, 2);

      // Consider first char of value
      char v = cline.at(0);
      char c;

      bool default_delim = false;

      // If quoted, that's our delimitor.
      if (v == '\"')
      {
         c = '\"';
         cline.erase(0, 1);
      }
      else if (v == '\'')
      {
         c = '\'';
         cline.erase(0, 1);
      }
      else
      {
         c = ' ';
         default_delim = true;
      }

      std::string value;

      // Cut parameter
      auto pos = cline.find(c);

      if (pos != std::string::npos)
      {
         value = cline.substr(0, pos);
         cline.erase(0, pos + (default_delim ? 0 : 1));
      }

      bool parsed = __program.CmdLine_Parameter_Consume(token.c_str(), value.c_str());

      if (!parsed)
         App_CmdLine_Parameter_Consume(token.c_str(), value.c_str());
   }

   return true;
}

bool Program_Init(wchar_t* cmdline)
{

#if __ENABLE_DIAGNOSTICS
   // Clear any previous logfile ...

   if (__log_active)
      ::DeleteFile(GetLogFile());
#endif

   setlocale(LC_ALL, "English");
   /*Win32*/::CoInitialize(nullptr);


#if __ENABLE_DEBUG_CATCH_MSG_BOX
   // Debug: Enable this to catch the process in the debugger.
   ::MessageBox(nullptr, TEXT("start"), TEXT("<><> inf_app"), MB_OK);
   //DebugBreak();
#endif

   // LOAD_BALANCE : On multi-processor systems, secondary processes (such as  video processor) run on anything except
   //                the first processor, which is used for the primary application main render loop.
   HANDLE hprocess = ::GetCurrentProcess();

   if (_GetProcessorCount() > 1)
      /*Win32*/::SetProcessAffinityMask(hprocess, 0xfffe);

   //HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);



   // Core functionality is sent via env var so it can be accessed by multiple processes, as may be required by some apps.
   // Each instance gets its own env block so this should be asynchronously thread & process safe.
   char inf_app_config[256];
   GetEnvironmentVariableA("INF_APP_CONFIG", inf_app_config, 255);

   //Infinity_Log(INF_LOG_DEFAULT, 2,"INF_APP_CONFIG=%s", inf_app_config);

   if (!CmdLine_Parse(inf_app_config))
   {
      Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "Config fail.\n");
      return false;
   }

   // Switch string format
   std::wstring cmdline_ws;

   if (cmdline)
      cmdline_ws.assign(cmdline);

   std::string _cmdline(cmdline_ws.begin(), cmdline_ws.end());

   if (cmdline && !CmdLine_Parse(_cmdline))
   {
      Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "[%s] Cmd_line fail.\n", __program_id);
      return false;
   }

   uint32_t process_id = GetProcessId(GetCurrentProcess());

#if 0
   // TODO: Debug this - its getting wrong process parent for firefox.
   uint32_t parent_process_id = 0;
   char parent_filename[1024];
   bool found = false;

   if (GetParentProcessID(process_id, parent_process_id))
   {
      if (GetProcessInfo(parent_process_id, parent_filename, 1024))
      {
         found = true;
      }
   }

   if (!found)
      strcpy(parent_filename, "???");

#endif

   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "Infinity_App[ %s ] [ PID: %x ] ", __program_id, process_id);

   // Pipe initialized during cmdline parse of config variable above.
   if (__program.m_pipe)
   {
      if (!__program.m_pipe->Connect())
         return false;
   }

   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "[%s] Pipe_Connect[post]", __program_id);

   __program.m_ipc_ready = true;

   return true;
}


bool Program_Initialized()
{
   return !!__program.m_parent_hwnd;
}


static bool Parent_Terminated()
{
   HANDLE parent_binding = 0;

   if (__program.m_parent_hwnd)
   {
      parent_binding = (HANDLE) ::GetPropA((HWND)__program.m_parent_hwnd, __program_id);

      if (!parent_binding)
         return true;
   }

   if ((parent_binding != (HANDLE)__magic))
   {
      Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "Binding error.");
      return true;
   }

   return false;
}


// Returns if it did work.

bool Program_Processing(Event_CB cb)
{
   if (!__program.m_pipe)
      return false;


#if 0
   // DBG: Heatbeat - check we're alive & responsive.
   static size_t __counter = 0;
   Infinity_Log(INF_LOG_PARENT, 0, "Program_Processing: %d", ++__counter);
#endif


   // Check for unexpected termination of parent & exit when detected.
   if (Parent_Terminated())
   {
      if (!__closing_orphan)
      {
         Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "Closing orphan process ...");
         App_Terminate(0, false);
         __closing_orphan = true;
      }
      else
      {
         Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "Orphan process terminated.");
         ::TerminateProcess(GetCurrentProcess(), 5150);
      }

      return true;
   }


   // Consume pending events.
   uint32 counter = 0;
   const uint32 max_cmds_per_cycle = 2;

   bool did_work = false;

   for (;;)
   {
      counter++;

      if (counter >= max_cmds_per_cycle)
         break; // Stay responsive if flooded.

      if (!__program.m_pipe->Available(sizeof(Cmd_Header)))
         break;

      Cmd_Header h;
      __program.m_pipe->Read(&h, sizeof(Cmd_Header));

      // Handle generic events ..
      if (OnParentEvent(h))
      {
         did_work = true;
      }
      else
      {
         bool handled = false;

         // Handle app specific events.
         if (cb)
            handled = cb(h);

         if (handled)
            did_work = true;
         else
            Infinity_Log(INF_LOG_FILE, INF_ERROR, "Pipe - unhandled cmd: %d", h.id);
      }
   }

   return did_work;
}


wchar_t* GetQuotedParameter(wchar_t*& dest, wchar_t* buffer, wchar_t* cmdline, uint32 cmd_len)
{
   wchar_t* end = wcschr(buffer, L'\'');

   dest = nullptr;

   // Terminate.
   if (end)
   {
      dest = buffer;
      *(end++) = '\0';

      size_t pos = (end - cmdline);

      // Resume tokenize immediately after this quoted parameter, if we've got any command line left to process.
      if (pos < cmd_len)
         return wcstok(end + 1, L" ");
   }

   return nullptr;
}


bool Program::CmdLine_Parameter_Consume(const char* token, const char* value)
{
   // IPC pipe config [BEGIN]

   if (*token == 'p')
   {
      m_pipe = new Pipe(); // We're the client. Server created pipe (the other end).

      unsigned __int64 pipe_id = 0;
      sscanf(value, "%llx", &pipe_id);
      m_pipe->m_id = (size_t)pipe_id;

      return true;
   }

   // IPC pipe config [END]

   else if (*token == 'w')
   {
      m_parent_hwnd = nullptr;
      sscanf(value, "%p", &m_parent_hwnd);

      // Mark the parent window as associated with this process class so we can be sure it's "our" window.
      // TODO: Different ID per process class
      ::SetPropA((HWND)m_parent_hwnd, __program_id, (HANDLE)__magic);
      return true;
   }

   else if (*token == 't')
   {
      unsigned int param_value;
	  // TODO: Use stl equivalents for this.
      sscanf(value, "%x", &param_value);
      m_parent_texture = param_value; // unique id for associated texture known to parent.
      return true;
   }

   else if (*token == 'x')
   {
#if INF_APP_DIAGNOSTICS_ENABLE
      _inf_app_dbg.assign(value);

      if (_inf_app_dbg.size() == 0)
      {
         Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "Invalid log path. Was expecting string.");
         return false;
      }

      Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "Log path : %s", _inf_app_dbg.c_str());
#endif

      return true;
   }

   else if (*token == 'd')
   {
      //sscanf(param.c_str(), TEXT("%d"), &__app.m_device_id);
      ::SetEnvironmentVariableA("INF_ADAPTER_ID", value);
      return true;
   }

   return false;
}