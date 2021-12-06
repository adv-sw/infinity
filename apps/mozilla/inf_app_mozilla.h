/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Mozilla App


File        : inf_app_mozilla.h

Description : Mozilla app base definition.

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


#ifndef INF_MOZILLA_APP_H
#define INF_MOZILLA_APP_H

#ifndef WIN32
// For now.
typedef void* HWND;
typedef bool BOOL;
#define FALSE false
#define TRUE true
#endif


// TODO: Configure this via command line switch for internal builds.
#define __DIAGNOSTICS_SHOW_WEBPAGE_WINDOW 0

#pragma warning (disable : 4786)

// Prevent compile issues.
#define mozilla_ServoStyleConsts_h 1
#define mozilla_ServoStyleConstsInlines_h 1

class nsWindow;
class nsIWebBrowserChrome;
class nsIPresShell;
class WebBrowserChrome;
class nsIURI;
class nsIWidget;
class nsIScrollable;
struct IDXGIKeyedMutex;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;

class App_Mozilla_Private;

#include <mozilla/PresShell.h>

#include "mozilla/dom/BrowsingContext.h"

namespace Infinity
{

   class App_Mozilla : public Infinity::App
   {
   public:

      App_Mozilla();
      ~App_Mozilla();


      virtual const std::string& GetClassIdentifier();

      virtual uint32 Async_Update(uint32 flags);

      virtual void OnCompact(uint32 flags);
      virtual void OnStateChange(uint32 flags);

      virtual bool GetParameter(const std::string& param_id, Parameter& p);

      virtual void OnVerticalScroll(float magnitude);
      virtual void OnButtonDown(uint32 button, uint32 flags);
      virtual void OnButtonUp(uint32 button);
      virtual void OnFocus();
      virtual void OnFocusLost();

      virtual void OnCursorEnter();
      virtual void OnCursorLeave();
      virtual void SetCursorCoordinates(const Vector2 &tcoords);

      //virtual void ReadAttributes(Stream &s);
      //virtual uint32 Write(Stream &s);

      virtual void OnStop();
      virtual void OnPrint();
      virtual void GoBack();
      virtual void GoForwards();
      virtual void Reload();

      virtual bool ProcessSystemMessage(uint32 msg_id, size_t wparam, size_t lparam);

      virtual bool Terminate();
      virtual void SetExclusiveDisplay(bool enable);
      virtual void SetAspectRatio(float aspect);

      virtual float Percentage_Ready();

      virtual void OnFilenameChanged();
      void UpdateNavigationState();

      void OnLoadBegin();
      void OnLoadEnd();

      void ScanForDialogs(nsIURI* uri);
      void ScanForDuplicates(nsIURI* uri);
      void NotifyOwnerOfNativeWindowChange();

      virtual void OnClipboardCommand(Clipboard_Command cmd);

      mozilla::PresShell* GetPresShell();
      void SetChrome(mozIDOMWindowProxy* c);
      mozIDOMWindowProxy* GetChrome();

      void* GetNativeRoot();
      uint32_t Update_Consume(nsIWidget* wid = nullptr);

      void Invalidate();
      bool SetFullScreen(bool fs);
      bool Resize(uint32 width, uint32 height);

      bool m_full_redraw_next_frame;
      bool m_trigger_size_to_content;
      bool m_is_dialog;
      uint8 m_button_state;

      bool m_trigger_resize;

      uint32 m_desired_width;
      uint32 m_desired_height;

      Vector2 m_cursor_texture_coords;
      bool m_cursor_data_available;

      std::string m_pending_address;

      float m_percentage_loaded;
      bool  m_load_update;
      bool  m_load_progress_state;
      float m_load_start_time;

      bool m_trigger_full_screen;

      RefPtr<mozIDOMWindowProxy> m_dom_window_proxy;

   private:

#ifdef WIN32
      void Msg_Coords_Screen(HWND hwnd, UINT msg_id, WPARAM wparam);
      void Msg_Coords_Widget(HWND hwnd, UINT msg_id, WPARAM wparam);
      void Msg_Hierarchy_Coords_Widget(HWND hwnd, UINT msg_id, WPARAM w);
      void Msg_Hierarchy(HWND hwnd, UINT msg_id, WPARAM w, LPARAM l);
#endif

      void ConvertToContentCoordinates(POINT& dest, const Infinity::Vector2& tcoords);

      bool NavigateTo(const std::string& url);

      HWND Popup_FindOpenJava();
      HWND Popup_FindJava();
      HWND Popup_GetUnderCursor();

      // TODO: Merge these. Current solution is overly complex.
      HWND GetViewWindow();
      HWND GetCursorWindow();

      void LoadPendingContent();

      bool  m_resize_disabled;
      bool  m_inspection_active;
      POINT m_inspection_coords;

      uint32 m_diagnostics_update_counter;

#ifdef WIN32

      HWND  m_inspection_window;
      HWND  m_page_plugin_under_cursor_native;
      HWND  m_cursor_window;
      HWND  m_popup_under_cursor;
#endif

      void OnCursorOver();
      POINT m_cursor_coords;

      uint32 m_update_next_n_frames;

      std::string m_last_uri;

      float m_scroll_x;
      float m_scroll_y;

   };

}

#endif // INF_MOZILLA_APP_H
