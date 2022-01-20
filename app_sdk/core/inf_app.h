/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_app.h

Description : App base class implementation.

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


#ifndef INF_APP_H
#define INF_APP_H

#include "inf_app_diagnostics.h"
#include "..\base\inf_pipe.h"
#include "..\base\inf_math.h"

#include <string>

namespace Infinity
{

   class Message;

#define INF_UPDATE_DISPLAY 1


#ifndef INF_APP_API
#define INF_APP_API __cdecl 
#endif // INF_APP_API


class App
{
public:
   App();
   virtual ~App();

   virtual bool INF_APP_API Initialize() { return true; }
   virtual void INF_APP_API Copy(App* src) {}

   virtual const std::string& INF_APP_API GetClassIdentifier() = 0;
   virtual bool INF_APP_API Busy() { return false; }

   virtual void INF_APP_API SetCursorCoordinates(const Vector2& tc) {}

   virtual bool INF_APP_API ProcessSystemMessage(uint32 msg_id, size_t wparam, size_t lparam);

   virtual void INF_APP_API SetPlaybackMode(Play_Mode pm) {}

   virtual void INF_APP_API SetAspectRatio(float aspect) {}
   virtual void INF_APP_API SetDesiredWidth(uint32 w) {}
   virtual void INF_APP_API SetDesiredHeight(uint32 h) {}

   // Exclusive display toggling.
   void INF_APP_API Exclusive_Request(bool enable);
   virtual void Exclusive_Requested(bool enable) {}

   virtual uint32 Update(uint32 flags) { return 0; } 
   
   virtual void OnCompact(uint32 flags) {}
   virtual void Alternative_Cue() {}

   virtual bool Terminate() { return true; }

   // This is a window handle used for targeting the desktop cursor.
   void INF_APP_API SetNativeTarget(void* target);

   virtual void OnIdentifierChanged() {}
   virtual void OnFilenameChanged() {}

   virtual void OnStateChange(uint32 flags) {}
   virtual void OnStop() {}

   virtual void INF_APP_API OnCursorEnter();
   virtual void INF_APP_API OnCursorLeave();

   virtual void OnVerticalScroll(float magnitude) {}
   virtual void OnButtonDown(uint32 button, uint32 flags) {}
   virtual void OnButtonUp(uint32 button) {}
   virtual void OnFocus() {}
   virtual void OnFocusLost() {}
   virtual void GoBack() {}
   virtual void GoForwards() {}
   virtual void Reload() {}
   virtual void OnPrint() {}
   virtual void OnPlay(bool play) {}

   void GetFilename(std::string& f) { f = m_filename; }
   void SetFilename(const std::string& f) { m_filename = f; }
   std::string m_filename;

   void GetIdentifier(std::string& id) { id = m_id; }
   void SetIdentifier(const std::string& id) { m_id = id; }
   std::string m_id;

   // State
   bool m_expired;
   bool m_is_compact;
   bool m_terminated;
   bool m_cursor_over;
   bool m_ignore_input;

   // Display
   float m_aspect_ratio;

   // Feedback.
   bool m_popup_blocked;
   bool m_request_update;

   // Can these two be combined ?
   float m_percentage_loaded;
   bool  m_loading;

   // Do we need these in the base class ?
   bool  m_can_go_forwards;
   bool  m_can_go_backwards;

   void* m_native_target;
   bool  m_native_target_changed;

   // Drawing
   Cursor m_cursor;

   bool m_device_update_in_process;

   // Used to decide when to update dynamic surfaces.
   size_t m_last_update_frame_stamp;

   Cmd_Target m_instance_id;
};


#define FIND_ONLY   0
#define FIND_CREATE 1
#define FIND_REMOVE 2


Infinity::App* __cdecl Find(Cmd_Target t, Infinity::uint32 op = FIND_ONLY);

}

// Utility functions ...
void URL_Normalize(std::string& url_out, const std::string& url_in);

#endif // INF_APP_H
