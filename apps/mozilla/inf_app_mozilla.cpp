/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Mozilla App

File        : inf_app_mozilla.cpp

Description : Infinity / Mozilla/Gecko interop

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


/* --------------------------------------------------------------------------------------------


// TODO: MOZ_UPGRADE.

1. Code tidy & <><> infinity app API refine.

2. Test, ship.
   2.1 Size page as required (done - first pass, SizeToContent by chrome gets lost on focus change
      - use ours if further testing shows built in s2c to be insufficient).

3. Merge to mozilla-central if patch will be accepted.

4. Patch into WebRender - Compositor11 removed, next version.


5.  Add support for dynamic content (plugins) : https://bugzilla.mozilla.org/show_bug.cgi?id=651192


6. Get our NPAPI plugin working with new windowless async shared surface interface.

7. Check whether flash/qt/java plugins support NPAPI async shared surface windowless mode.


// MOZ_UPGRADE [end]


 -------------------------------------------------------------------------------------------- */

#define __UPDATE_DIAGNOSTICS         0
#define __MOZ_PLUGIN_LOG_CURSOR_INFO 0
#define __TEST_FAULT_HANDLER         0
#define INF_MOZ_DIAGNOSTICS 0

// TODO: Benefit either way ?
#define Windoze_Msg ::PostMessage
//#define Windoze_Msg ::SendMessage

#include "nsIPrintSettings.h"
#include "nsIWebBrowserPrint.h"

#include "nsICookieService.h"
#include "nsICookieManager.h"

#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsFocusManager.h"
#include "..\..\..\..\mozilla\toolkit\xre\CmdLineAndEnvUtils.h"
#include "nsIScrollableFrame.h"

// + <><> Infinity App SDK <><> +
#include "..\app_sdk\base\inf_version.h"
#include "..\app_sdk\base\inf_parameter.h"
#include "..\app_sdk\core\inf_app_diagnostics.h"

static uint8 _GetWindowTransparency()
{
   // High value used for testing only - low transparency, so native window content can be viewed.
   // Normally, we don't want to see the window content - it is ideally off screen.

#if __DIAGNOSTICS_SHOW_WEBPAGE_WINDOW
   return 128;
#elif defined _DEBUG || defined INF_DEBUG
   return 250;
#else
   return 5;
#endif

}

void __cdecl _SetWindowTransparency(void *wnd, uint_8 alpha)
{
   HWND hwnd = (HWND) wnd;

#ifndef WS_EX_LAYERED
   const DWORD WS_EX_LAYERED = 0x80000;
#endif

#ifndef LWA_ALPHA
   const DWORD LWA_ALPHA     = 0x02;
#endif

   if (!hwnd || !/*Win32*/::IsWindow(hwnd))
      return;

   /*Win32*/::SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE)|WS_EX_LAYERED);

	HINSTANCE hInst = /*Win32*/::LoadLibraryA("USER32.DLL");

	if(hInst)
	{
		typedef BOOL (WINAPI *MYFUNC)(HWND,COLORREF,BYTE,DWORD);

		// Find SetLayeredWindowAttributes function ...
		MYFUNC func = (MYFUNC) /*Win32*/::GetProcAddress(hInst, "SetLayeredWindowAttributes");

		if (func)
         func(hwnd, RGB(0xff,0xff,0), alpha, LWA_ALPHA);

		/*Win32*/::FreeLibrary(hInst);
	}
}


#include "inf_app_mozilla.h"


#include "mozilla/Logging.h"
mozilla::LazyLogModule sLog_Infinity("Infinity");


// Configuration parameters.

const float loading_anim_cycle_time = 1.0f;

// Defaults for fixed size Mozilla dialogs.
const uint32 __dialog_width  = 512;
const uint32 __dialog_height = 512;

#define LOAD_PROGESS_COUNTER_MAX 100

// TODO: Query for the scroll bar dimensions to remove the constant.
// Values found by experimentation.
// The increase required to compensate for the scroll bar to prevent
// the need for a scroll bar in the other dimension.
const uint32 scrollbar_width  = 34;
const uint32 scrollbar_height = 34;

static void ThemeChange_StyleSheet(mozilla::dom::Document* doc, const std::string &theme_id);

// Debug configuration macros ...

// When the render display and webpage windows are on different screens,
// windoze seems to ignore our DirectInput exclusive mouse capture
// state and sends win32 mouse messages. This results in duplicated
// messages, which results in incorrect moz. mouse button processing code.
// When we are debugging using this screen configuration, disable our
// mouse button message generation as a workaround.
#define __DISABLE_BUTTON_MESSAGE_GENERATION 0

static bool __desktop_cursor_zapped = false;
static std::string __pending_theme_change;

XPCOM_API(HWND) hwndForDOMWindow(mozIDOMWindowProxy* window);

XPCOM_API(void*) Compositor_Update_Consume(mozilla::layers::LayerManager* lm);


#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

bool File_Exists(const std::string& path)
{
   if (FILE* file = fopen(path.c_str(), "r")) 
   {
      fclose(file);
      return true;
   }
   else
      return false;
}


bool File_or_Dir_Exists(const std::string& path)
{
   fs::path p(path);
   return fs::exists(p);
}


void replace(std::string& s, const char from, const char to)
{
   std::replace(s.begin(), s.end(), from, to);
}


void NativePath(std::string& path)
{
   if (path.length() >= 2)
   {
      auto char_2 = path.at(2);

      if ((path[0] == '/') && ((char_2 == '/') || (char_2 == '\0')))
      {
         path[0] = path[1];
         path[1] = ':';
      }

      replace(path, '/', '\\');
   }
}


// Convert local filesystem specification to url format, when detected.
static void FormProperURL(std::string& dest_url, const std::string &addr)
{
   // Form mozilla local file syntax.
   std::string local_url(addr);
   NativePath(local_url);
   replace(local_url, '\\', '/');
   local_url = std::string("file:///") + local_url;

   if (File_or_Dir_Exists(addr))
      dest_url = local_url;

   else if (File_Exists(addr + "/index.htm"))
      dest_url = local_url + "/index.htm";

   else if (File_Exists(addr + "/index.html"))
      dest_url = local_url + "/index.html";

   else if (File_Exists(addr + ".htm"))
      dest_url = local_url + ".htm";

   else if (File_Exists(addr + ".html"))
      dest_url = local_url + ".html";

   else
      dest_url = addr;
}


static bool __window_creator_initialized = false;


#ifdef WIN32

static bool InsideRect(RECT &r, POINT &p)
{
   if (p.x < r.left)
      return false;

   if (p.x > r.right)
      return false;

   if (p.y < r.top)
      return false;

   if (p.y > r.bottom)
      return false;

   return true;
}


void ShowNativeWindow(HWND hwnd, bool other_thread, bool show)
{
   if (hwnd)
   {
      // We must call ShowWindowAsync, rather than ShowWindow, when changing the window state from
      // a different thread - e.g. application compact. ShowWindow can lock up when called from another thread.
      if (other_thread)
         /*Win32*/::ShowWindowAsync(hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
      else
         /*Win32*/::ShowWindow(hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
   }
}


// Module handle.
static HINSTANCE __module_handle = 0;

HINSTANCE GetCurrentModuleHandle()
{
   return __module_handle;
}


// TODO: [BEGIN] Move this code out of this plugin and into Infinity initialize code.
static WNDPROC __default_ComboLBox_callback = nullptr;
static WNDPROC __default_32769_callback = nullptr;


bool IgnoreDesktopCursor()
{
#if 1
   ::SetCursor(nullptr);
   return true; // Prevent Windows from setting cursor to window class cursor
#else
   return false; // Default behaviour.
#endif
}


static LRESULT CALLBACK __ComboLBox_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   if (message == WM_SETCURSOR)
      return IgnoreDesktopCursor();

   if (__default_ComboLBox_callback)
      return __default_ComboLBox_callback(hWnd, message, wParam, lParam);

   return 0;
}


static LRESULT CALLBACK __32769_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   if (message == WM_SETCURSOR)
      return IgnoreDesktopCursor();

   if (__default_32769_callback)
      return __default_32769_callback(hWnd, message, wParam, lParam);

   return 0;
}


static void InitializeSystemPopupOverride()
{
   // We need to override the callbacks for these two system window classes, so that
   // we can prevent system popups from displaying the standard windows cursor.

   HWND tmp = /*Win32*/::CreateWindow(TEXT("ComboLBox"), TEXT("tmp"), WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, GetCurrentModuleHandle(), 0);

   __default_ComboLBox_callback = (WNDPROC) /*Win32*/::GetClassLongPtr(tmp, GCLP_WNDPROC);
   /*Win32*/::SetClassLongPtr(tmp, GCLP_WNDPROC, (LONG_PTR)__ComboLBox_callback);

   /*Win32*/::DestroyWindow(tmp);


   tmp = /*Win32*/::CreateWindow(TEXT("#32769"), TEXT("tmp"), WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, GetCurrentModuleHandle(), 0);

   __default_32769_callback = (WNDPROC) /*Win32*/::GetClassLongPtr(tmp, GCLP_WNDPROC);
   /*Win32*/::SetClassLongPtr(tmp, GCLP_WNDPROC, (LONG_PTR)__32769_callback);

   /*Win32*/::DestroyWindow(tmp);
}



mozilla::PresShell* Infinity::App_Mozilla::GetPresShell()
{
   nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(m_dom_window_proxy));
   nsCOMPtr<nsIDocShell> doc_shell(do_QueryInterface(webNav));

   if (doc_shell)
      return doc_shell->GetPresShell();

   return nullptr;
}


nsIWidget* GetBaseWidget(mozIDOMWindowProxy *dwp)
{
   nsIWidget* widget = nullptr;

   if (dwp)
   {
	  nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(dwp));
	  nsCOMPtr<nsIDocShell> ds(do_QueryInterface(webNav));
	  //NS_ENSURE_STATE(ds);

      nsCOMPtr<nsIBaseWindow> base_wnd = do_QueryInterface(ds);
      base_wnd->GetParentWidget(&widget);
   }

   return widget;
}
  

//
//  FUNCTION: BrowserWndProc(HWND, UINT, WRAPAM, LPARAM)
//
//  PURPOSE:  Processes messages for the browser container window.
//
LRESULT CALLBACK BrowserWndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
{
   nsIWebBrowserChrome* chrome = (nsIWebBrowserChrome*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

   switch (message)
   {
   case WM_ERASEBKGND:
      // Reduce flicker by not painting the non-visible background
      return 1;
   }

   return DefWindowProc(hWnd, message, wparam, lparam);
}


#endif // WIN32


// TODO: Implement like this instead as it permits more flexible accept/reject.
uint32 App_GetVersion()
{
   return INF_PROGRAM_VERSION;
}


void __stdcall Inf_Cookie_Update()
{
   Msg_Parent(INFINITY_APP_COOKIE_UPDATE, 0, nullptr, 0);
}


void __stdcall Inf_Progress_Update()
{
   Msg_Parent(INFINITY_PROGRESS, 0, nullptr, 0);
}



// --- [END] Procedural texture plugin interface ---


// Constructor

Infinity::App_Mozilla::App_Mozilla() : Infinity::App()
{
   m_scroll_x = 0;
   m_scroll_y = 0;

   m_trigger_resize = false;

   m_diagnostics_update_counter = 0;

   m_cursor_texture_coords = Zero_Vector2;
   m_cursor_data_available = false;

#ifdef WIN32
   m_cursor_window = nullptr;
   m_popup_under_cursor = nullptr;
   m_page_plugin_under_cursor_native = nullptr;
   m_inspection_window = nullptr;
#endif // WIN32

   m_update_next_n_frames = 0;

   m_cursor_coords.x = 0;
   m_cursor_coords.y = 0;
   m_cursor_data_available = false;

   m_inspection_active = false;
   m_inspection_coords.x = 0;
   m_inspection_coords.y = 0;

   m_resize_disabled = false;

   m_is_dialog = false;

   m_desired_width = 0;
   m_desired_height = 0;

   m_full_redraw_next_frame = false;

   m_button_state = 0;

   m_load_progress_state = false;
   m_load_start_time = -1.0f;
   m_load_update = false;


   // Experimental
   m_trigger_full_screen = false;
}


Infinity::App_Mozilla::~App_Mozilla()
{
#if INFINITY_INTERNAL_BUILD
   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "~App_Mozilla");
#endif

}


const std::string &App_Mozilla::GetClassIdentifier()
{
   static const std::string id = "webpage";
   return id;
}


bool Infinity::App_Mozilla::GetParameter(const std::string& param, Parameter& p)
{
   if (param == "inspect")
   {
      auto value = p.Get_UInt();
      uint32 input = value ? *value : 0;
      p.Set((size_t)((input == 0) ? 2 : 1));  // 2D content but show in window when UI visible.

      return true;
   }

   return false;
}


bool Infinity::App_Mozilla::Terminate()
{
   return true;
}


#if 0

// TODO: Rework as IPC

uint32 Infinity::App_Mozilla::Write(Stream& s)
{
   if (m_desired_width != 0)
      s.WriteAttribute(XSG_Attribute_width, m_desired_width);

   if (m_desired_height != 0)
      s.WriteAttribute(XSG_Attribute_height, m_desired_height);

   return 0;
}


void Infinity::App_Mozilla::ReadAttributes(Stream& s)
{
   // Plugin specific attributes read.
   String* param = s.FindAttribute(XSG_Attribute_width);

   if (param)
   {
      m_desired_width = param->GetUInt();
   }

   param = s.FindAttribute(XSG_Attribute_height);

   if (param)
   {
      m_desired_height = param->GetUInt();
   }
}
#endif


void Infinity::App_Mozilla::OnFilenameChanged()
{
   std::string tmp;
   GetFilename(tmp);

   URL_Normalize(m_pending_address, tmp);
}


HWND Infinity::App_Mozilla::GetViewWindow()
{
   HWND hwnd = nullptr;

   if (m_inspection_active)
      hwnd = m_inspection_window;

   if (!hwnd)
      hwnd = (HWND)GetNativeRoot();

   return hwnd;
}


void Infinity::App_Mozilla::OnStateChange(uint32 flags)
{
}


void Infinity::App_Mozilla::SetAspectRatio(float aspect)
{
   if (m_aspect_ratio != aspect)
   {
      // dbg
      //String s;
      //s.Printf("[moz] aspect change from %f to %f", m_aspect, aspect);
      //Infinity_Log(INF_LOG_DEFAULT, INF_INFO, s);

      m_aspect_ratio = aspect;

      m_trigger_resize = true;
   }
}


void Infinity::App_Mozilla::LoadPendingContent()
{
   if (!m_pending_address.empty())
   {
      if (NavigateTo(m_pending_address))
         m_pending_address.clear();
   }
}


static void _AdvAspectAdjust(Infinity::uint32& width, Infinity::uint32& height, float required_aspect,
   const float max_width, const float max_height)
{
   // An aspect ratio of zero means size exactly to content (ie. do not perform an aspect adjust).

   if ((height > 0) && (required_aspect > 0.0f))
   {
      float content_aspect = float(width) / float(height);

      float required_adjust = required_aspect / content_aspect;

#if INF_MOZ_DIAGNOSTICS
      std::string s;
      s << "[aspect] content_aspect=" << content_aspect;
      Infinity_Log(INF_LOG_DEFAULT, INF_INFO, s);

      s.Printf("[aspect] req=%f", required_adjust);
      Infinity_Log(INF_LOG_DEFAULT, INF_INFO, s.ptr());
#endif

      if (required_adjust > 1.0f)
      {
         uint32 proposed_width = uint32(float(width) * required_adjust);

         // If there's not enough space, adjust by reducing height.
         if (/*vertical_scroll_bar || */(proposed_width > max_width))
            height = uint32(float(height) / required_adjust);
         else // or increasing width, if it does not.
         {
            width = proposed_width;
         }
      }
      else
      {
         // Instead of reducing the width, which will introduce a horizontal scroll bar,
         // increase the height to maintain the same aspect ratio.
         uint32 proposed_height = uint32(float(height) * (1.0f / required_adjust));

         // If content requires a horizontal scroll bar, or there's not enough space, adjust by reducing width.
         if (proposed_height > max_height)
            width = uint32(float(width) * required_adjust);
         else // or increasing width, if it does not.
         {
            height = proposed_height;
         }
      }
   }
}


bool Infinity::App_Mozilla::Resize(uint32 width, uint32 height)
{
#if 0

   // TODO: Reference Docshell patch on load complete if we need this again.

   uint32 prev_width = m_content_width;
   uint32 prev_height = m_content_height;

   const uint32 WEBPAGE_MIN_WIDTH = 50;
   const uint32 WEBPAGE_MIN_HEIGHT = 50;

   // Get constraints ...
   Infinity::uint32 max_width = 0, max_height = 0;
   GetPreferences_Page(max_width, max_height);

#if 0
   if (m_is_dialog)
   {
      m_desired_width = __dialog_width;
      m_desired_height = __dialog_height;
   }
#endif

   if (width > max_width)
      width = max_width;

   if (height > max_height)
      height = max_height;



   //m_aspect_ratio = 5.0f;

#if INF_MOZ_DIAGNOSTICS
   String s;
   s.Printf("[moz_aspect] m_aspect=%f", m_aspect);
   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, s.ptr());
#endif

   m_content_width = width;
   m_content_height = height;

   _AdvAspectAdjust(m_content_width, m_content_height, m_aspect_ratio, max_width, max_height);

   bool hscroll = false, vscroll = false;

   nsIPresShell* ps = GetPresShell();

   if (ps)
      ps->GetScrollbarVisibility(hscroll, vscroll);

   // Add space for scroll bars, if required.
   if (vscroll)
      m_content_width += scrollbar_width;

   if (hscroll)
      m_content_height += scrollbar_height;

   if ((prev_width != m_content_width) || (prev_height != m_content_height))
   {
      HWND hwnd = GetNativeRoot();
      /*Win32*/::SetWindowPos(hwnd, HWND_TOP, 0, 0, m_content_width, m_content_height, SWP_NOMOVE);// | SWP_NOACTIVATE);

#if INF_MOZ_DIAGNOSTICS
      std::string s;
      s << "[moz_size] " << m_content_width << " " << m_content_height;
      Infinity_Log(INF_LOG_DEFAULT, INF_INFO, s);
#endif
   }

#endif

   return true;
}


void Diagnostics_Processing(HWND hroot)
{
   UpdateInfo* info = _GetUpdateInfo(hroot, true);

   if (info)
   {
      if (info->m_content_string)
      {
         // Log string buffer to the onscreen console ...
         /*auto dev = Infinity::GetRenderingDevice();

         if (dev)
            dev->Print(1, info->m_content_string);
            */

            //... and to log file.
         Infinity_Log(INF_LOG_DEFAULT, INF_INFO, info->m_content_string);

         info->ClearDiagnostics();
      }

      _ReleaseUpdateInfo(info);
   }
}


mozilla::dom::Document* GetDocument(void* inf_moz)
{
   Infinity::App_Mozilla* mp = (Infinity::App_Mozilla*)inf_moz;
   mozilla::PresShell* ps = mp->GetPresShell();
   return ps->GetDocument();
}


void Infinity::App_Mozilla::SetCursorCoordinates(const Vector2& tc)
{
   m_cursor_texture_coords = tc;
   m_cursor_data_available = true;
}


Infinity::uint32 Infinity::App_Mozilla::Async_Update(uint32 flags)
{
   if (m_trigger_resize)
   {
      // TODO: Could be implicit from non-zero desired width/height
      Resize(m_desired_width, m_desired_height);
      m_desired_width = 0;
      m_desired_height = 0;
      m_trigger_resize = false;
   }

   if (!__pending_theme_change.empty())
   {
      mozilla::PresShell* ps = GetPresShell();
      mozilla::dom::Document* doc = ps ? ps->GetDocument() : nullptr;

      if (doc)
      {
         ThemeChange_StyleSheet(doc, __pending_theme_change);
         __pending_theme_change.clear();
      }
   }

   if (m_cursor_data_available)
   {
      if (!m_cursor_over)
         OnCursorEnter();

      OnCursorOver();

      m_cursor_data_available = false;
   }

   LoadPendingContent();

   // Only update render surfaces when we're informed we should update the display.
   if (!(flags & INF_UPDATE_DISPLAY))
      return 0;

   // Look for an open java popup ...
   auto hbrowser = (HWND) GetNativeRoot();
   auto info = _GetUpdateInfo(hbrowser, false);

   if (info)
   {
      info->m_current_java_popup = Popup_FindOpenJava();

      //Log("moz_pt: NO_exclusive_trigger");

      if (info->m_exclusive_trigger)
      {
         //Log("moz_pt: exclusive_trigger");

         uint32 enable = (uint32)info->m_exclusive;
         info->m_exclusive_trigger = false;

         // TODO: Rework this now we're out of process.
         Infinity::Message* msg = Infinity::CreateMessage(INF_MESSAGE_EXCLUSIVE_REQUEST, enable);
         MessageParent(msg);
      }

      _ReleaseUpdateInfo(info);
   }

   if (++m_diagnostics_update_counter > 500)
   {
      Diagnostics_Processing((HWND)GetNativeRoot());
      m_diagnostics_update_counter = 0;
   }


   // Experimental ...
   if (m_trigger_full_screen)
   {
//      if (!m_context->IsLoading())
      {
         bool done = SetFullScreen(true);

         if (done)
            m_trigger_full_screen = false;
      }
   }

   return 0;
}


void SetOwner(HWND native_window, void* owner)
{
   // TODO: Check if already present & delete.
   //_DestroyUpdateInfo(native_window);

   if (native_window)
   {
      UpdateInfo* info = _CreateUpdateInfo(native_window, owner);
   }
}


HWND Infinity::App_Mozilla::Popup_FindJava()
{
   // TODO: Enhance the identification of the popup, to ensure we do
   // not confuse it with others that may be present.

   DWORD pid = GetProcessId(nullptr); //dev ? dev->GetNativeProcessID() : GetProcessId(nullptr);

   HWND hwnd = nullptr;
   char parent_class_id[256];

#ifdef WIN32
   do
   {
      hwnd = /*Win32*/::FindWindowEx(nullptr, hwnd, TEXT("ComboLBox"), nullptr);

      if (hwnd)
      {
         DWORD wnd_pid;
         /*Win32*/::GetWindowThreadProcessId(hwnd, &wnd_pid);

         // The popup must belong to the same process as this application.
         if (pid == (DWORD)wnd_pid)
         {
            HWND hpar = /*Win32*/::GetParent(hwnd);

            if (hpar)
            {
               /*Win32*/::GetClassNameA(hpar, parent_class_id, 255);

               if (!strcmp(parent_class_id, "#32769"))
                  break;
            }
         }
      }

   } while (hwnd);

#endif // WIN32

   return hwnd;
}


static bool Popup_Java_Initialized(HWND hwnd_jpopup)
{
#ifdef WIN32
   bool initialized = (bool)!!/*Win32*/::GetProp(hwnd_jpopup, TEXT("_jpopinit"));

   if (!initialized)
      /*Win32*/::SetProp(hwnd_jpopup, TEXT("_jpopinit"), (void*)1);
#else
   bool initialized = true;
#endif

   return initialized;
}


HWND Infinity::App_Mozilla::Popup_FindOpenJava()
{
   HWND hwnd = Popup_FindJava();

   // Force transparency (first call) on this popup to ensure it remains
   // hidden (offscreen), but draws so we can blit from its surface.
   if (!Popup_Java_Initialized(hwnd))
   {
      // Webpage popups should not be topmost elements.
      // We move them from topmost to top to prevent "draw through".

#ifdef WIN32
      /*Win32*/::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

      // Make transparent, so the popup still draws when repositioned behind the render window.
      _SetWindowTransparency(hwnd, _GetWindowTransparency());
#endif // WIN32

      // TODO:     Prevent visual artifacts.
      //
      // The popup activates the standard desktop cursor.
      // a) Intermittantly, when the cursor crosses the edge of the popup into the plugin window.
      // b) After an item from the popup has been selected.
      //      -  this also forces an undesired SetForegroundWindow 'flicker'.
      //
      // Possible solutions :-
      //
      // 1. Desktop cursor artifacts.
      //
      // a) Modify popup window class, so it has no visible cursor.
      // b) Modify java source.
      // c) Zap desktop cursor, while <><> is active, restore on minimize, exit.
      //      - Side effect - loss of desktop cursor on app crash.
      //                    - if we implement parent process (below), it could restore the cursor in this event.
      //
      // 2. SetForegroundWindow flicker.
      //
      // a) Modify java source to prevent the call.
      // b) Create a parent process which locks the foreground. [TODO - implement/test].
   }

#ifdef WIN32
   if (/*Win32*/::IsWindowVisible(hwnd))
   {
      if (!__desktop_cursor_zapped)
      {
         Infinity::Message* msg = Infinity::CreateMessage(INF_MESSAGE_ZAP_DESKTOP_CURSOR);
         MessageParent(msg);
         __desktop_cursor_zapped = true;
      }

      return hwnd;
   }
#endif // WIN32

   return nullptr;
}


HWND Infinity::App_Mozilla::Popup_GetUnderCursor()
{
   HWND hbrowser = (HWND)GetNativeRoot();
   UpdateInfo* info = _GetUpdateInfo(hbrowser, true);

   HWND hpop = nullptr;

   if (info)
   {
      // Java plugin popups ...
#ifdef WIN32
      if (info->m_current_java_popup)
      {
         RECT rect;

         /*Win32*/::GetWindowRect((HWND)info->m_current_java_popup, &rect);

         if (InsideRect(rect, m_cursor_coords))
            hpop = (HWND)info->m_current_java_popup;
      }
#endif

      // Regular webpage popups ...

      if (!hpop)
      {
         int count = info->GetVisiblePopupCount();

         if (count > 0)
         {
            void** popups = new void* [count];

            info->GetVisiblePopups(popups);

            RECT rect;

            for (int i = 0; i < count; i++)
            {
#ifdef WIN32
               HWND hwnd = (HWND)popups[i];

               /*Win32*/::GetWindowRect(hwnd, &rect);

               if (InsideRect(rect, m_cursor_coords))
               {
                  hpop = hwnd;
                  break;
               }
#endif // WIN32
            }

            delete[] popups;
         }
      }
   }

   _ReleaseUpdateInfo(info);

   return hpop;
}


#ifdef WIN32

void Infinity::App_Mozilla::Msg_Coords_Widget(HWND hwnd, UINT msg_id, WPARAM wparam)
{
   POINT pt = m_cursor_coords;

   // Map from surface to widget local coordinate space.
   POINT offset;
   offset.x = 0;
   offset.y = 0;
   /*Win32*/::ClientToScreen(hwnd, &offset);

   pt.x -= offset.x;
   pt.y -= offset.y;

   // If these fail, the moz native window wasn't placed at origin zero as we currently require.
   //INF_ASSERT(pt.x >= 0);
   //INF_ASSERT(pt.y >= 0);

   Windoze_Msg(hwnd, msg_id, wparam, MAKELPARAM(pt.x, pt.y));
}


void Infinity::App_Mozilla::Msg_Hierarchy(HWND hwnd, UINT msg_id, WPARAM w, LPARAM l)
{
   while (hwnd)
   {
      if (hwnd)
         Windoze_Msg(hwnd, msg_id, w, l);

      hwnd = GetParent(hwnd);
   }
}


void Infinity::App_Mozilla::Msg_Hierarchy_Coords_Widget(HWND hwnd, UINT msg_id, WPARAM w)
{
   while (hwnd)
   {
      if (hwnd)
         Msg_Coords_Widget(hwnd, msg_id, w);

      hwnd = GetParent(hwnd);
   }
}


static void DisplayNativeWindow(HWND target_window, Infinity::uint32 pos)
{
   char id[255];
   ::GetClassNameA(target_window, id, 254);
   std::stringstream str;
   str << "cursor:" << id << target_window;
}

#endif // WIN32


void Infinity::App_Mozilla::OnFocusLost()
{
   if (m_button_state == 0)
   {
      OnClipboardCommand(Select_None);

      nsFocusManager* fm = nsFocusManager::GetFocusManager();

      if (fm && m_dom_window_proxy)
      {
         fm->WindowLowered(m_dom_window_proxy, nsFocusManager::GenerateFocusActionId());

         // DEBUG_FOCUS_1: Used to verify correct focus events received.
         //Beep(1000,100);
      }
   }
}


void Infinity::App_Mozilla::OnFocus()
{
   if (m_button_state == 0)
   {
      nsFocusManager* fm = nsFocusManager::GetFocusManager();

      if (fm && m_dom_window_proxy)
      {
         fm->WindowRaised(m_dom_window_proxy, nsFocusManager::GenerateFocusActionId());

         // DEBUG_FOCUS_2: Used to verify correct focus events received.
         //Beep(2000,100);
      }
   }
}


void Infinity::App_Mozilla::Invalidate()
{
   HWND hbase = (HWND) GetNativeRoot();
   ::PostMessage(hbase, WM_USER, 1, 0xc001f00d);
}


bool Infinity::App_Mozilla::SetFullScreen(bool fs)
{
   auto ps = GetPresShell();
   auto doc = ps ? ps->GetDocument() : nullptr;

   if (doc->GetFullscreenRoot())
      return true;

   auto elem = ps->GetCanvas();

   static mozilla::ErrorResult rc;

   if (elem)
      elem->RequestFullscreen(mozilla::dom::CallerType::System, rc);

   return false;
}


void Infinity::App_Mozilla::OnButtonDown(uint32 button, uint32 xxflags)
{
   // Ensure focus is gained before button msg sent to ensure correct operational behaviour.
   OnFocus();

   nsIWidget* widget = GetBaseWidget(m_dom_window_proxy);

   if ((button == 0) || (button == 1))
   {
      if (widget)
      {
         const auto native_msg = nsIWidget::NativeMouseMessage::ButtonDown;
         
         mozilla::LayoutDeviceIntPoint p(m_cursor_coords.x, m_cursor_coords.y);
         widget->SynthesizeNativeMouseEvent(p, native_msg, button == 0 ? MouseButton::ePrimary : MouseButton::eSecondary, nsIWidget::Modifiers::NO_MODIFIERS, nullptr);
      }

      m_button_state |= (button == 0) ? 1 : 0;
      m_button_state |= (button == 1) ? 2 : 0;
   }
   else if (button == 3)
   {
      SetExclusiveDisplay(m_inspection_active ^ 1);
   }
}

  
void Infinity::App_Mozilla::OnButtonUp(uint32 button)
{
   HWND target_window = GetCursorWindow();

   // We need to ignore any right button messages that come through while we are over a plugin
   // window as we don't currently process plugin window popups such as flash preferences.
   // This prevents lock-ups until further support is implemented.
   bool ignore = (button > 1); 

   if (ignore)
      return;

   nsIWidget *widget = GetBaseWidget(m_dom_window_proxy);

   if ((button == 0) || (button == 1))
   {
      if (widget)
      {
         const auto native_msg = nsIWidget::NativeMouseMessage::ButtonUp;

         mozilla::LayoutDeviceIntPoint p(m_cursor_coords.x, m_cursor_coords.y);
         widget->SynthesizeNativeMouseEvent(p, native_msg, 
                  button == 0 ? MouseButton::ePrimary : MouseButton::eSecondary, nsIWidget::Modifiers::NO_MODIFIERS, nullptr);

         m_button_state &= (button == 0) ? ~1 : 0xff;
         m_button_state &= (button == 1) ? ~2 : 0xff;
      }
   }
}


void Infinity::App_Mozilla::SetExclusiveDisplay(bool enable)
{
   if (m_inspection_active != enable)
   {
      m_inspection_active = enable;

      if (m_inspection_active)
      {
         m_inspection_coords = m_cursor_coords;

         // Look for a plugin under the cursor to inspect, if we don't find one,
         // we'll inspect the entire visible page area.

         // TODO: Support zooming of frames.

#ifdef WIN32
         m_inspection_window = (HWND)_WindowFromPoint(GetNativeRoot(), (void*)&m_inspection_coords);

         while (m_inspection_window && !_IsDynamic(m_inspection_window))
         {
            m_inspection_window = /*Win32*/::GetParent(m_inspection_window);
         }
#endif // WIN32

         Exclusive_Request(true);
      }
      else
      {
         m_inspection_window = nullptr;
         Exclusive_Request(false);
      }

      NotifyOwnerOfNativeWindowChange();
   }
}


uint32_t Infinity::App_Mozilla::Update_Consume(nsIWidget* wid)
{
   if (wid)
      return (uint32_t) wid->GetNativeData(NS_NATIVE_GRAPHIC);

#if 1

   // TODO ff95 upgrade requires WebRender and/or different means of accessing LayerManager as this interface is end of line.
   
   else
   {
      auto ps = GetPresShell();

	   // TODO: Will require rework for ff94+ hence move this version to web render to minimize porting difficulties.
      mozilla::layers::LayerManager *lm = ps ? ps->GetLayerManager() : nullptr;
      return (uint32) Compositor_Update_Consume(lm);
   }
#endif

   return 0;
}


void* Infinity::App_Mozilla::GetNativeRoot()
{
   return (void*) hwndForDOMWindow(m_dom_window_proxy);
}


HWND Infinity::App_Mozilla::GetCursorWindow()
{
   // Grab the root widget to enable us to interact with the page & scroll bars.
   m_cursor_window = (HWND) GetNativeRoot();

#if 0
   // dbg
   DisplayNativeWindow(m_cursor_window, 1);

   nsIWidget* widget = GetBaseWidget(m_dom_window_proxy);

   void* hwnd = (void*)(widget ? widget->GetNativeData(NS_NATIVE_WINDOW) : nullptr);

   // dbg
   DisplayNativeWindow((HWND)hwnd, 2);

   //HWND hroot = GetNativeRoot();
   //m_cursor_window = hroot ? _WindowFromPoint(hroot, m_cursor_coords) : nullptr;
   //DisplayNativeWindow(m_cursor_window, 2);
#endif

   m_popup_under_cursor = Popup_GetUnderCursor();

   return m_popup_under_cursor ? m_popup_under_cursor : m_cursor_window;
}


void Infinity::App_Mozilla::ConvertToContentCoordinates(POINT& dest, const Infinity::Vector2& tcoords)
{
   nsIWidget* widget = GetBaseWidget(m_dom_window_proxy);

   int32_t content_width = 0;
   int32_t content_height = 0;

   if (widget) {
      mozilla::LayoutDeviceIntRect r = widget->GetClientBounds();
      content_width = r.width;
      content_height = r.height;
      widget->Release();  // GetParentWidget AddRefs so clean up.
   }

   // For now, whilst we fix stuff up.
   //INF_ASSERT(content_width > 0);
   //INF_ASSERT(content_height > 0);

   dest.x = LONG(tcoords.x * content_width);
   dest.y = LONG(tcoords.y * content_height);
}


void Infinity::App_Mozilla::OnCursorOver()
{
   ConvertToContentCoordinates(m_cursor_coords, m_cursor_texture_coords);

#if 0 // def WIN32
   // Determine if the cursor is over a plugin window. If it is, we'll route synthesized messages to it.
   HWND candidate = (HWND)_WindowFromPoint(GetNativeRoot(), (void*)&m_cursor_coords);

   while (candidate && !_IsDynamic(candidate))
      candidate = /*Win32*/::GetParent(candidate);

   m_page_plugin_under_cursor_native = candidate;
#endif // WIN32


   nsIWidget* widget = GetBaseWidget(m_dom_window_proxy);

   if (widget)
   {
      mozilla::LayoutDeviceIntPoint pt(m_cursor_coords.x, m_cursor_coords.y);

      widget->SynthesizeNativeMouseEvent(pt, nsIWidget::NativeMouseMessage::Move, m_button_state & 1 ? MouseButton::ePrimary : MouseButton::eNotPressed,
         nsIWidget::Modifiers::NO_MODIFIERS, nullptr);

      //Infinity_Log(INF_LOG_DEFAULT, INF_INFO + 2, "cursor: %d  %d", pt.x, pt.y);
   }
}


void Infinity::App_Mozilla::OnCursorEnter()
{
   App::OnCursorEnter();

   OnFocus();
}


void Infinity::App_Mozilla::OnCursorLeave()
{
   App::OnCursorLeave();

   OnFocusLost();
}


#ifdef WIN32
void Infinity::App_Mozilla::Msg_Coords_Screen(HWND hwnd, UINT msg_id, WPARAM wparam)
{
   POINT pt = m_cursor_coords;
   Windoze_Msg(hwnd, msg_id, wparam, MAKELPARAM(pt.x, pt.y));
}
#endif // WIN32


static size_t __page_uid = 0;

bool Infinity::App_Mozilla::NavigateTo(const std::string &addr)
{
   MOZ_LOG(sLog_Infinity, mozilla::LogLevel::Warning, ("NavigateTo : %s)", addr.c_str()));

   SetExclusiveDisplay(false);

   std::string _url;
   FormProperURL(_url, addr);
   nsCString url(_url.c_str());
   
   nsresult rv = NS_OK;
   nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
   nsCString features("chrome,remote"); 
 
   // TODO: Browsing history/navigate : A new chrome each navigate will lose history, hence rework for persistance.
   
   // Assign unique page name 
   std::stringstream id;
   id << "inf_" << __page_uid++;
   nsCString name(id.str().c_str());
   
   rv = wwatch->OpenWindow(nullptr, // mozIDOMWindowProxy* aParent, 
                            url,
                            name,
                            features,
                            nullptr, // nsISupports* aArguments,
							getter_AddRefs(m_dom_window_proxy));
									 
    
   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "NavigateTo : %s", addr.c_str());
   
   if (rv != NS_OK)
   {
      Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, rv == NS_ERROR_MALFORMED_URI ? "Malformed URI" : "FAIL");
   }
   
   // Bind the chrome to this procedural texture ...
   auto native_window = (HWND) GetNativeRoot();

   SetOwner(native_window, this);

   return rv == NS_OK;
}


void Infinity::App_Mozilla::UpdateNavigationState()
{
   if (m_dom_window_proxy)
   {
      nsCOMPtr<nsIWebNavigation> web_nav(do_GetInterface(m_dom_window_proxy));

      if (web_nav)
      {
         bool canGoForward = false;
         web_nav->GetCanGoForward(&canGoForward);
         m_can_go_forwards = !!canGoForward;

         bool canGoBack = false;
         web_nav->GetCanGoBack(&canGoBack);
         m_can_go_backwards = !!canGoBack;
      }
   }
}


void Infinity::App_Mozilla::GoBack()
{
   if (m_dom_window_proxy)
   {
      nsCOMPtr<nsIWebNavigation> web_nav(do_GetInterface(m_dom_window_proxy));

      if (web_nav)
         nsresult rv = web_nav->GoBack(false, true);
   }

   SetExclusiveDisplay(false);
}


void Infinity::App_Mozilla::OnStop()
{
   if (m_dom_window_proxy)
   {
      nsCOMPtr<nsIWebNavigation> web_nav(do_GetInterface(m_dom_window_proxy));

      nsresult rv = web_nav->Stop(nsIWebNavigation::STOP_NETWORK);
   }
}


void Infinity::App_Mozilla::GoForwards()
{
   if (m_dom_window_proxy)
   {
      nsCOMPtr<nsIWebNavigation> web_nav(do_GetInterface(m_dom_window_proxy));

      if (web_nav)
         nsresult rv = web_nav->GoForward(false, true);
   }

   SetExclusiveDisplay(false);
}


void Infinity::App_Mozilla::OnPrint()
{
   if (m_dom_window_proxy)
   {
      nsCOMPtr<nsIWebBrowserPrint> webBrowserPrint(do_QueryInterface(m_dom_window_proxy));

      // NOTE: Embedding code shouldn't need to get the docshell or
      //       contentviewer AT ALL. This code below will break one
      //       day but will have to do until the embedding API has
      //       a cleaner way to do the same thing.

      // IMPORTANT : You need to disable print progress in all.js to prevent stalling.

      if (webBrowserPrint)
      {
         nsCOMPtr<nsIPrintSettings> printSettings;
         webBrowserPrint->GetCurrentPrintSettings(getter_AddRefs(printSettings));
         NS_ASSERTION(printSettings, "You can't PrintPreview without a PrintSettings!");
         if (printSettings)
         {
            printSettings->SetPrintSilent(PR_TRUE);
            webBrowserPrint->Print(printSettings, (nsIWebProgressListener*)nullptr);
         }
      }
   }
}


void Infinity::App_Mozilla::Reload()
{
   if (m_dom_window_proxy)
   {
      nsCOMPtr<nsIWebNavigation> web_nav(do_GetInterface(m_dom_window_proxy));

      if (web_nav)
         nsresult rv = web_nav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
   }

   SetExclusiveDisplay(false);
}


// TODO: Now pages are always hidden, this can be simplified.
void Infinity::App_Mozilla::OnCompact(uint32 flags)
{
   HWND hbrowser = (HWND)GetNativeRoot();

#ifdef WIN32
   if (hbrowser)
      ShowNativeWindow(hbrowser, flags & 1, false);
#endif // WIN32

   // We need to explicitly hide java popup windows when we compact, or they
   // will remain floating on the desktop when we've compacted.

   UpdateInfo* info = _GetUpdateInfo(hbrowser, true);

   if (info)
   {
#ifdef WIN32
      if (info->m_current_java_popup)
         ShowNativeWindow((HWND)info->m_current_java_popup, flags & 1, false);
#endif // WIN32

      _ReleaseUpdateInfo(info);
   }

   m_cursor_window = nullptr;
   m_popup_under_cursor = nullptr;

   // Desktop cursor may have been restored, following an application iconization.
   // Note that compacts also occur when a page is no longer visible, but
   // additional calls to the associated code will have low overhead,
   // and cause no additional side effects. Its all a big nasty hack, anyway ...
   __desktop_cursor_zapped = false;
}


void Infinity::App_Mozilla::NotifyOwnerOfNativeWindowChange()
{
   SetNativeTarget((void*)(m_inspection_window ? m_inspection_window : GetNativeRoot()));
}


void Infinity::App_Mozilla::OnLoadBegin()
{
}


void Infinity::App_Mozilla::OnLoadEnd()
{
   UpdateNavigationState();
}


void Infinity::App_Mozilla::ScanForDuplicates(nsIURI* uri)
{
   if (!uri)
      return;

   // Some content reloads itself following a resize.
   // Ensure we don't get stuck in an endless loop by not resizing if this is the same content we saw last time

   nsAutoCString aURI;
   uri->GetSpec(aURI);

   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, aURI.get());

   if (m_last_uri == aURI.get())
   {
      m_resize_disabled = true;
   }
   else
      m_last_uri = aURI.get();
}


/*
void Infinity::App_Mozilla::ScanForDialogs(nsIURI* uri)
{
   if (!uri)
      return;

   nsAutoCString aURI;
   uri->GetSpec(aURI);

   std::string id(aURI.get());

   // Mozilla dialogs do not currently size-to-content correctly.
   // so detect them so them and size to optimal fixed dimensions (found by trial and error).

   // TODO: Not everything in a local jar file is necessarily a dialog, but this will do for now.
   m_is_dialog = !id.Compare("jar:file", 8, false);
   m_is_dialog |= !id.Compare("chrome://", 9, false);

   //if (m_is_dialog)
     // m_trigger_resize = true;
}
*/

void Infinity::App_Mozilla::OnClipboardCommand(Clipboard_Command cmd)
{
#if 0

   // TODO: Fixup - moz changed.

   if (m_context)
   {
      auto ds = m_context->GetDocShell();
      mozIDOMWindowProxy* dwp = nullptr;
      ds->GetDomWindow(&dwp);

      nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(ds);

      if (!clipCmds)
         return;

      switch (cmd)
      {
      case Infinity::Cut:
         clipCmds->CutSelection();
         break;

      case Infinity::Copy:
         clipCmds->CopySelection();
         break;

      case Infinity::Paste:
         clipCmds->Paste();
         break;

      case Infinity::Select_All:
         clipCmds->SelectAll();
         break;

      case Infinity::Select_None:
         clipCmds->SelectNone();
         break;
      }
   }
#endif
}


float Infinity::App_Mozilla::Percentage_Ready()
{
   return 1.0f;
}


// TODO: Fix this up

#if 1
static void ThemeChange_StyleSheet(mozilla::dom::Document* doc, const std::string &theme_id) {}
#else

#include "nsContentList.h"

#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLLinkElement.h"


static void ThemeChange_StyleSheet(nsIDocument* doc, const std::string &theme_id)
{
   nsAutoString name;
   name.AssignLiteral("link");

   RefPtr<nsContentList> list = doc->GetElementsByTagName(name);

   uint32_t len = 0;
   list->GetLength(&len);

   for (uint32_t index = 0; index < len; index++)
   {
      nsIDOMNode* node = nullptr;
      list->Item(index, &node);

      nsCOMPtr<nsINode> _node = do_QueryInterface(node);
      mozilla::dom::Element* de = _node->AsElement();

      nsAutoString attributeName, attributeValue;
      attributeName.AssignLiteral("title");
      de->GetAttribute(attributeName, attributeValue);

      std::string title(attributeValue.get());

      nsCOMPtr<mozilla::dom::HTMLLinkElement> nodeAsLink = do_QueryInterface(node);

      if (nodeAsLink)
      {
         if (theme_id && !title.compare(*theme_id))
            nodeAsLink->SetDisabled(false);
         else if (!title.IsEmpty())
            nodeAsLink->SetDisabled(true);
      }
   }
}
#endif



bool Infinity::App_Mozilla::ProcessSystemMessage(uint32 msg_id, size_t wparam, size_t lparam)
{
   if (App::ProcessSystemMessage(msg_id, wparam, lparam))
      return true; // Already Processed.

   if (msg_id != WM_CHAR)  // We need to ignore these to avoid character input duplication for some reason ...
   {
      HWND target_window = GetCursorWindow();
      Windoze_Msg(target_window, msg_id, wparam, lparam);
      return true;
   }

   return false;
}


void App_UpdateURI(void* user_data, const char* uri)
{
   //auto *app = INF_CAST_PTR(Infinity::App_Mozilla, user_data);
   auto app = (Infinity::App_Mozilla*) user_data;

   if (app && uri)
   {
      bool skip = !strncmp(uri, "chrome:", 7);
      skip |= !strncmp(uri, "about:", 6);

      if (!skip)
      {
         if (app)
         {
            std::string id;
            app->GetIdentifier(id);

            // TODO: This is not quite enough.
            // Initial requests can come through without file:// or http://
            // prefix, which results in this function passing back an
            // indication that the URL has changed, when it hasn't.
            if (id.compare(uri))
            {
               app->SetIdentifier(uri);
               app->m_request_update = true; // OPT: Do this with a message, rather than adding a bool to every plugin_texture.
            }
         }
      }
   }
}


void Infinity::App_Mozilla::OnVerticalScroll(float magnitude)
{
   const float webpage_scroll_wheel_sensitivity = 100.0f;

   auto sf = GetPresShell()->GetScrollableFrameToScroll(layers::EitherScrollDirection);

   if (sf)
   {
      nsIntPoint delta;
      delta.x = 0;
      delta.y = -magnitude * webpage_scroll_wheel_sensitivity;
      sf->ScrollBy(delta, mozilla::ScrollUnit::DEVICE_PIXELS, mozilla::ScrollMode::Normal);
   }
}



bool App_Operation(Infinity::uint32 operation_id, std::string& result, char *param_1)
{
   if (operation_id == INFINITY_APP_THEME_CHANGE)
   {
      // Flag theme change before looping over instances. Remove it at the other end so only done once.
      __pending_theme_change.assign(param_1);
   }

   else if (operation_id == INFINITY_APP_COOKIE_QUERY)
   {
      // Current Mozilla implementation requires the following to be called from moz main thread.
      // I.e. if you call from any other thread it will barf.
      // Hence why this implementation is so complicated.
      nsCOMPtr<nsICookieService> __cookieService = do_GetService(NS_COOKIESERVICE_CONTRACTID);

      if (!__cookieService)
         return false;

      nsresult rc = NS_ERROR_FAILURE;
      nsCOMPtr<nsIURI> uri;
      nsresult rv = NS_NewURI(getter_AddRefs(uri), param_1);

      nsAutoCString _cookie;

      /*if (flags & 0x0001)
      rc = __cookieService->GetCookieStringFromHttp(uri.get(), nullptr, nullptr, &cookie);
      else*/
      rc = __cookieService->GetCookieStringFromHttp(uri.get(), nullptr, _cookie);

      const char* cookie = nullptr;
      _cookie.GetData(&cookie);

      if (cookie)
         result.assign(cookie);
      else
         result.clear();

      return rc == NS_OK;
   }
   else if (operation_id == INFINITY_APP_COOKIE_ASSIGN)
   {
      // Current Mozilla implementation requires the following to be called from moz main thread.
      // If you call from any other thread it'll barf.
      // Hence have implemented infrastructure to enable that.
      nsCOMPtr<nsICookieService> __cookieService = do_GetService(NS_COOKIESERVICE_CONTRACTID);

      if (!__cookieService)
         return false;

      nsresult rc = NS_ERROR_FAILURE;
      nsCOMPtr<nsIURI> uri;
      nsresult rv = NS_NewURI(getter_AddRefs(uri), param_1);

      // Value is passed in the result parameter - a bit hacktastic but will do for now.
      std::string  cmd(result.c_str());

      cmd.append(";expires=01 Jan 3000;path=/"); // TODO: timeout as required.

      auto _cookie_str = cmd.c_str();

      /*if (flags & 0x0001)
      rc = __cookieService->SetCookieStringFromHttp(uri.get(), nullptr, nullptr, cookie_str, nullptr, nullptr);
      else*/

      nsAutoCString cookie_str(_cookie_str);

      rc = __cookieService->SetCookieStringFromHttp(uri.get(), cookie_str, nullptr);

      Inf_Cookie_Update();

      return rc == NS_OK;
   }

   return false;
}


bool __inf_modal_dialog = false;

extern mozilla::LazyLogModule sLog_Infinity;

// TODO: Move to header.
#define FIND_ONLY   0
#define FIND_CREATE 1
#define FIND_REMOVE 2


void Parameters_Send(Pipe* p, Cmd_Target t, Cmd_ID cmd_id, ::Parameters* params);
bool Program_Initialized();
void Remove(Cmd_Target t);
extern std::map<size_t, Infinity::App*> __lookup;


const char* __program_id = "webpage";

// App specific implementation.

// Glue app to infinity procedural texture implementation so we can port easily.

bool App_InitializeClass(uint32 version);
void App_TerminateClass();

bool Program_Init(wchar_t* cmdline);
Pipe* Program_GetPipe();


bool __cdecl Infinity_App_Init()
{
   // TODO: Reject incompatible apps - grab this from INF_APP_CONFIG enviironment variable or another after adding said support :)
   //if (Program_GetVersion() != INF_PROGRAM_VERSION)
     // return false; 

   if (!Program_Init(nullptr))
   {
      Infinity_Log(INF_LOG_DEFAULT, INF_ERROR, "[%s] Program_Init: fail.", __program_id);
      return false;
   }

   Infinity_Log(INF_LOG_DEFAULT, INF_INFO,"[%s] Infinity_App_Init", __program_id);

#ifdef INF_PATCH
   // If we're running as an <><> infinity app (redirected output into other application), signal we want gecko to run in content server mode, all processes.
   if (PR_GetEnv("INF_APP_CONFIG"))
   {
      // Run Gecko in content serving mode ...   
      mozilla::SaveToEnv("MOZ_GECKO_SERVER=1");
   }
#endif // INF_PATCH


   // Disabled Java DirectDraw/Direct3D rendering to allow grabbing of its buffer from a Window device context.
   // No idea if we still need something like this - test status of current Java.
   //_putenv("_JAVA_OPTIONS=-Dsun.java2d.noddraw=true");

   InitializeSystemPopupOverride();

#if 0

   // Not required now we're running as a modified firefox.

   // Figure out where we are ...
   char moz_path[MAX_PATH];
   /*Win32*/::GetModuleFileName(GetCurrentModuleHandle(), moz_path, MAX_PATH);

   char* ptr = strrchr(moz_path, '\\');

   if (ptr)
      *ptr = '\0';

   // Remember where we were so we can restore known state.
   char current_dir[_MAX_PATH];
   /*Win32*/::GetCurrentDirectory(_MAX_PATH, current_dir);

   // Establish state required to load this mozilla version (might be others in the system)
   char env[_MAX_PATH];
   sprintf(env, "GRE_HOME=%s", moz_path);
   _putenv(env);

   /*Win32*/::SetCurrentDirectory(moz_path);

#endif

   Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "[%s] Infinity_App_Init : ok.", __program_id);

   return true;
}


bool __running = true;

void Program_Complete();


void __cdecl Infinity_App_Shutdown()
{
   Program_Complete();
}


bool Dialog_Processing(Infinity::App_Mozilla *app, size_t t)
{
   bool done = false;

   auto hbrowser = (HWND) app->GetNativeRoot();
   auto info = _GetUpdateInfo(hbrowser, false);

   if (info)
   {
      if (__inf_modal_dialog)
      {
         if (info->IsDialogDataEntryComplete())
            done = true;
      }

      auto di = info->m_dialog_info;

      if (di && di->m_open_pending)
      {
         di->m_open_pending = false;
         Parameters_Send(Program_GetPipe(), t, INFINITY_DIALOG_OPEN, &(di->m_parameters));
      }

      // Register popups ... so they can be presented.
      // Currently hardwired to a maximum of one visible overlay.

      int count = info->GetVisiblePopupCount();

      if (count > 0)
      {
         void** popups = new void* [count];

         info->GetVisiblePopups(popups);

         RECT rect;

         // For now we only present the first popup.
         for (int i = 0; i < 1; i++)
         {
            auto hpopup = (HWND)popups[i];

            PopupInfo* pi = _GetPopupInfo(hpopup, true);  // TODO: Maybe don't busy wait.
            auto wid = (nsIWidget*)(pi ? pi->m_widget : nullptr);

            auto shandle = app->Update_Consume(wid);

            if (shandle)
            {
               /*Win32*/::GetWindowRect(hpopup, &rect);
               Overlay((uint32)shandle, rect, t);
               info->m_popup_close_notify_required = true;
            }
         }

         delete[] popups;
      }
      else if (info->m_popup_close_notify_required)
      {
         RECT rect;
         SetRect(&rect, 0, 0, 0, 0);
         Overlay(0, rect, t);
         info->m_popup_close_notify_required = false;
      }

      _ReleaseUpdateInfo(info);
   }

   return done;
}


bool __cdecl Infinity_App_Processing(bool &did_work)
{
   if (!Program_Initialized()) // If not initialized programming error or a child process we don't operate through.
      return true;

   bool abort = false;

   //did_work |= 
   Program_Processing(nullptr);

   bool done = false;

   size_t t = 0;
   for (auto i = __lookup.begin(); i != __lookup.end(); ++i, ++t)
   {
      auto app = (Infinity::App_Mozilla*) (*i).second;

      if (!app)
         continue;

      app->Async_Update(INF_UPDATE_DISPLAY);

      Dialog_Processing(app, t);

      mozilla::PresShell *ps = app->GetPresShell();

      if (ps)
      {
          auto handle = app->Update_Consume(nullptr);

          if (handle)
             Present(handle, 0.0f, t);
         
      }
   }

   return __running;
}


void App_Terminate(Cmd_Target t, bool normal)
{
   __running = false;
}


// This function is called every time a plugin of this class is created.
extern "C" Infinity::App * __cdecl App_CreateInstance()
{
   return new Infinity::App_Mozilla();
}


void __cdecl App_Open(Cmd_Target t, wchar_t* _url)
{
   if (!_url)
      return;

   Infinity_Log(INF_LOG_DEFAULT, 2,"App_Open : %S", _url);

   std::wstring url_w(_url);
   std::string url(url_w.begin(), url_w.end());

   auto app = Infinity::Find(t, FIND_CREATE);

   if (!url.empty() && app)
   {
      app->SetFilename(url);
      app->OnFilenameChanged();
   }
}


void __cdecl App_OnVerticalScroll(Cmd_Target t, float value)
{
   auto app = Infinity::Find(t);
   app->OnVerticalScroll(value);
}


void __cdecl App_OnFocus(Cmd_Target t, bool value)
{
   auto app = Infinity::Find(t);

   if (app)
   {
      if (value)
         app->OnFocus();
      else
         app->OnFocusLost();
   }
}


void __cdecl App_OnCursor(Cmd_Target t, bool value)
{
   auto app = Infinity::Find(t);

   if (app)
   {
      if (value)
         app->OnCursorEnter();
      else
         app->OnCursorLeave();
   }
}


void __cdecl App_Telemetry_Update(Cmd_Target t, const Msg_Telemetry &msg)
{
   auto app = Infinity::Find(t);
}


void __cdecl App_SetCursorCoordinates(Cmd_Target t, const Vector2& value)
{
   auto app = Infinity::Find(t);

   if (app)
      app->SetCursorCoordinates(value);
}


void __cdecl App_Audio_SetStatus(Cmd_Target t, uint8_t value)
{
}


void __cdecl App_Audio_Request(Cmd_Target t, uint8_t value)
{
}


void __cdecl App_OnDelete(Cmd_Target t)
{
   auto app = Infinity::Find(t);

   if (app)
   {
      Remove(t);
      delete app;
   }
}


void __cdecl App_Pause(Cmd_Target t)
{
   auto app = Infinity::Find(t);
   //assert(app);
}


void __cdecl App_Redraw(Cmd_Target t)
{
   auto app = (Infinity::App_Mozilla*)Infinity::Find(t);
   //assert(app);

   if (app)
      app->Invalidate();
}


void __cdecl App_Dialog_Close(Cmd_Target t, ::Parameters& params, int exit_code)
{
   auto app = (Infinity::App_Mozilla*)Infinity::Find(t);
   assert(app);

   auto hbrowser = (HWND) app->GetNativeRoot();
   auto info = _GetUpdateInfo(hbrowser, true);

   if (info)
   {
      RemoteDialogInfo* di = info->m_dialog_info;

      di->m_exit_code = exit_code;

      di->m_parameters.Copy(params);

      _ReleaseUpdateInfo(info);
   }
}


void __cdecl App_OnVisible(Cmd_Target t, unsigned char new_val)
{
   auto app = Infinity::Find(t);
   //assert(app);
}


void __cdecl App_Play(Cmd_Target t)
{
   auto app = Infinity::Find(t);
   //assert(app);
}


void __cdecl App_SetVolume(Cmd_Target t, float volume)
{
   auto app = Infinity::Find(t);
   //assert(app);
}


bool __cdecl App_CmdLine_Parameter_Consume(const char* token, const char* value)
{
   return false;
}


void __cdecl App_Shutdown()
{
}


void __cdecl App_SetButtonState(Cmd_Target t, const Button_State& value)
{
   auto app = Infinity::Find(t);
   
   if (app)
   {
      if (value.flags & BFLAGS_DOWN)
         app->OnButtonDown(value.button, value.flags);
      else
         app->OnButtonUp(value.button);
   }
}


void __cdecl App_System_Message(Cmd_Target t, const System_Message& value)
{
   auto app = Infinity::Find(t);

   if (app)
      app->ProcessSystemMessage(value.msg_id, value.wparam, value.lparam);
}