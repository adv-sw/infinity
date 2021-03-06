/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Test App

File        : app_test.cpp

Description : Bare bones skeleton reference app.

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



#ifndef UPDATE_INFO_EXPORTS

// Disable secure warnings
#pragma warning(disable:4996)

#include "../../app_sdk/base/inf_pipe.h"
#include "../../app_sdk/core/inf_app.h"
#include "../../app_sdk/core/inf_app_diagnostics.h"

#define FIND_ONLY   0
#define FIND_CREATE 1
#define FIND_REMOVE 2



bool Program_Init(wchar_t *cmd_line);
bool Program_Initialized();
void Program_Complete();
void Program_Terminate();
void Remove(Cmd_Target t);

#include <map>
#include <string>

extern std::map<size_t, Infinity::App *> __lookup;;
#include "../../app_sdk/base/inf_dialog_info.h"

const char *__program_id = "test";

using namespace Infinity;

static bool __running = true;


bool __cdecl  Infinity_App_Init()
{
   return Program_Init(nullptr);
}


void App_Terminate(Cmd_Target t, bool value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_Terminate[%d] : %d", t, value);
   
#if 0
   __running = false;
#else
   // Simulate behaviour of an app that doesn't exit cleanly - e.g. current firefox.

  HANDLE process = ::GetCurrentProcess();
  if (::TerminateProcess(process, 0)) {
    ::WaitForSingleObject(process, INFINITE);
  }

  Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "TerminateProcess failed.");

#endif 
}


void App_Shutdown()
{
   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "App_Shutdown");
   Program_Complete();
   Program_Terminate();
}


bool App_On_Event(const Cmd_Header& cmd)
{
   return true;
}


bool __cdecl Infinity_App_Processing()
{
   if (!Program_Initialized()) // If not initialized programming error or a child process we don't operate through.
   {
      Infinity_Log(INF_LOG_FILE, INF_ERROR, "Not initialized");
      return false;
   }

   static bool init = false;

   if (!init)
   {
      Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "Initialized.");
      init = true;
   }

   bool did_work = Program_Processing(App_On_Event);

   if (!did_work)
      ::SwitchToThread();  // load_balance.

   return __running;
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


INT WINAPI wWinMain(HINSTANCE module_handle, HINSTANCE hprev, wchar_t* cmd_line, int)
{
   // Simulate long startup for complex app such as firefox.
   //Sleep(2000);

   if (!Infinity_App_Init())
      return -2;

   if (!Program_Initialized()) // If not initialized programming error or a child process we don't operate through.
      return -1;

   while (Infinity_App_Processing())
   {
      System_Processing();
   }

   App_Shutdown();

   return 0;
}


extern "C" Infinity::App *App_CreateInstance()
{
   return nullptr;
}


void App_Open(Cmd_Target t, wchar_t *url)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_Open[%d] : %S", t, url);

//   App *pt = Find(t, FIND_CREATE);
}


void App_OnVerticalScroll(Cmd_Target t, float value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_OnVerticalScroll[%d] : %f", t, value);
}


void App_OnFocus(Cmd_Target t, bool value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_OnFocus[%d] : %d", t, value);
}


void App_OnCursor(Cmd_Target t, bool value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_OnCursor[%d] : %d", t, value);
}


void App_Audio_SetStatus(Cmd_Target t, uint8_t value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_Audio_SetStatus[%d] : %d ", t, value);
}


void App_Audio_Request(Cmd_Target t, uint8_t value)
{
}


void App_SetCursorCoordinates(Cmd_Target t, const Vector2 &value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_SetCursorCoordinates[%d] : %f %f", t, value.x, value.y);
}


void App_OnDelete(Cmd_Target t)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_OnDelete[%d]", t);
}


void App_OnVisible(Cmd_Target t, unsigned char value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_OnVisible[%d] : %d", t, value);
}


void App_Play(Cmd_Target t)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_Play[%d]", t);
}


void App_SetVolume(Cmd_Target t, float value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_SetVolume[%d] : %f", t, value);
}


bool App_CmdLine_Parameter_Consume(const char *token, const char *value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_CmdLine_Parameter_Consume : %s   =    %s", token, value);
   return true;
}


void App_SetButtonState(Cmd_Target t, const Button_State &value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_SetButtonState[%d] : %d %d", t, value.button, value.flags);
}


void App_System_Message(Cmd_Target t, const System_Message &value)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_System_Message[%d] : %d %d %d", t, value.msg_id, value.wparam, value.lparam);
}


void App_Pause(Cmd_Target t)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_Pause[%d]", t);
}


void App_Redraw(Cmd_Target t)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_Redraw[%d]", t);
}


void App_Dialog_Close(Cmd_Target t, ::Parameters &params, int exit_code)
{
   Infinity_Log(INF_LOG_DEFAULT, 2,"App_Dialog_Close[%d] : %d", t, params.Entries_Get().size());
}


bool App_Operation(Infinity::uint32 operation_id, std::string &result, char *param_1)
{
   return false;
}


void App_Telemetry_Update(Cmd_Target t, const Msg_Telemetry &msg)
{
}

void App_Exclusive_Requested(Cmd_Target t, bool enable)
{
}

#endif