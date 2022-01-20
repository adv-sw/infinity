/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_app.cpp

Description : App base class implementation.

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

//"..\base\inf_system_message.h"

#include "inf_app.h"
#include "inf_interprocess_msg.h"

using namespace Infinity;


void App::OnCursorEnter()
{
   m_cursor_over = true;
}


void App::OnCursorLeave()
{
   //m_cursor_over = false;
}


App::App()
{
   m_percentage_loaded = 0.0f;

   m_native_target = nullptr;
   m_native_target_changed = false;

   m_is_compact = false;
   m_terminated = false;
   m_loading = false;
   m_expired = false;
   m_ignore_input = false;

   m_aspect_ratio = 1.0f;

	// The frame this texture was last drawn (0 = never)
	m_last_update_frame_stamp = 0;

   m_popup_blocked = false;
   m_request_update = false;

   m_can_go_forwards  = false;
   m_can_go_backwards = false;
   m_cursor = Cursor_Default;

   m_cursor_over = false;

   m_device_update_in_process = false;
}


App::~App()
{
}


void App::Exclusive_Request(bool enable)
{
   uint8 value = (uint8) enable;
   Msg_Parent(INFINITY_REQUEST_EXCLUSIVE, m_instance_id, &value, 1, false);
}


void App::SetNativeTarget(void *target)
{
   m_native_target = target;
   m_native_target_changed = true;
}


bool App::ProcessSystemMessage(uint32 msg_id, size_t wparam, size_t lparam)
{
/*

   if (msg_id == WM_KEYDOWN)
   {
      auto key = wparam;

      // TODO: Why is the Windows 8 SDK gating here ??
//#if  WINVER <= _WIN32_WINNT_WIN8

      // Copy ...
      if ( (key == 'C') && KeyState(VK_CONTROL))
      {
         OnClipboardCommand(Clipboard_Command::Copy);
         return true;
      }

      // Cut ...
      else if ( (key == 'X') && KeyState(VK_CONTROL))
      {
         OnClipboardCommand(Clipboard_Command::Cut);
         return true;
      }

      // Paste ...
      else if (key == 'V')
      {
         if (KeyState(VK_CONTROL))
         {
            OnClipboardCommand(Clipboard_Command::Paste);
            return true;
         }
      }

      else if (key == 'A')
      {
         if (KeyState(VK_CONTROL))
         {
             OnClipboardCommand(Clipboard_Command::Select_All);
             return true;
         }
      }
//#endif

   }
*/


   return false; // Not processed.
}


// TODO: app_util.cpp
bool Syntax_LocalFileSystemSyntax(const std::string &filename)
{
   auto first = filename.at(0);

   // *nix local filesystem root directory format ... also infinity local path format.
   if (first == '/')
      return true;

   // *nix home directory format. TODO: Expand.
   if (first == '~')
     return true;

   return false;
}


void URL_Normalize(std::string &dest, const std::string &url_in)
{
   if (Syntax_LocalFileSystemSyntax(url_in))
   {
      dest.assign("file://");
      dest += url_in;

#if defined _WIN32 || defined _WIN64
      dest.insert(9, 1, ':');
#endif
   }
   else
      dest.assign(url_in);
}