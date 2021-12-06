/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Mozilla App


File        : _update_info.cpp

Description : Content -> control application communication implementation.

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


#if defined _WIN32 || defined _WIN64
#include <windows.h>
#else
// Linux
#include <wchar.h>
#include <stdlib.h>
#endif

#if 1

#define ___MAX(x,y) ((x) > (y) ? (x) : (y))

#include "..\..\app_sdk\core\inf_app_diagnostics.h"
#include "..\..\app_sdk\base\inf_pixel.h"

#include "_update_info.h"
#include "adv_sw_patch.h"
#include "..\..\app_sdk\base\inf_dialog_info.h"

#include <set>

#include <D3D11.h>



using namespace Infinity;

void Inf_Yield()
{
#if defined _WIN32 || defined _WIN64

#if !WIN_RT
    // TODO: Re-enable if older equipment with fewer hardware threads requires idle processing time.
    ///*Win32*/::SwitchToThread();
#endif

#else
    sched_yield();
#endif
}



typedef std::set<void*> WndSet;


#define UPDATE_INFO_ID     "_u_"
#define POPUP_INFO_ID      "_p_"

#define _DYNAMIC_MARKER_ID "_d_"

#define GET_PRIVATE(c) (( c##_Private*)m_private)
#define GET_PRIVATE_PTR(a,b) (( a##_Private *) ( b ) ->m_private)

#define MUTEX_ENABLED 1

#define ENABLE_MUTEX_DIAGNOSTICS 0

#define LOG_INVALIDATES 0

#define NUM_POPUP_DATA_FIELDS 3

#define POPUP_DATA_NATIVE 0


// TODO: LOAD_BALANCE: Check this
const uint_32 __mutex_wait_period = 5;

static void *__master_mutex = nullptr;

static Infinity::PixelFormat __popup_pixel_format = DXGI_FORMAT_B8G8R8A8_UNORM;


// All popups are the same type so this is sufficient to define state.
static bool __backend_dx11 = true;


_ProtectedAccess::_ProtectedAccess()
{
   m_mutex = nullptr;

#if ENABLE_MUTEX_DIAGNOSTICS
   m_locked_by = -1;
#endif

   // Initialize master mutex .... should this be a critical section ?
   if (!__master_mutex)
   {
#if defined _WIN32 || defined _WIN64
      __master_mutex = /*Win32*/::CreateMutexA(nullptr, FALSE, "<!>");

#if ENABLE_MUTEX_DIAGNOSTICS
         if (!__master_mutex)
            Log("Could not create master mutex.");
#endif

#endif

   }
}


_ProtectedAccess::~_ProtectedAccess()
{
#if defined _WIN32 || defined _WIN64
   if (m_mutex)
      /*Win32*/::CloseHandle(m_mutex);
#endif
}


bool _ProtectedAccess::Lock(uint_32 lock_id, bool wait_for_access)
{
#if MUTEX_ENABLED
   bool created_n_locked = false;

   if (!m_mutex)
   {

#if defined _WIN32 || defined _WIN64
      // Wait for the master mutex ...
      while (!/*Win32*/::WaitForSingleObject(__master_mutex, __mutex_wait_period) == WAIT_OBJECT_0)
      {
         // If we can't get it & we don't need to wait, try later ...
         if (!wait_for_access)
            return false;

         Inf_Yield();
      }
#endif


      if (!m_mutex) // Check again ... another thread could have set this while we were waiting ...
      {
         char mutex_id[50];

         for (;;)
         {
            // Attempt to create a unique identifier for this mutex.
            // If the id clashes with an existing one, try again.
            sprintf(mutex_id, "_updateinfo_mx_%d", rand());

#if defined _WIN32 || defined _WIN64
            m_mutex = /*Win32*/::CreateMutexA(NULL, TRUE, mutex_id);
            created_n_locked = true;

            if (/*Win32*/::GetLastError() == ERROR_ALREADY_EXISTS)
               continue;
#endif

            break;
         }
      }

#if defined _WIN32 || defined _WIN64
      // Release ownership of the master mutex ...
      uint_32 rc = /*Win32*/::ReleaseMutex(__master_mutex);
      // TODO: Should check this error code ...
#endif

   }

#if ENABLE_MUTEX_DIAGNOSTICS
  uint_32 counter =0;
  const uint_32 max_lock_attempts = 50;
#endif

   if (!created_n_locked)
   {
#if defined _WIN32 || defined _WIN64
      // Request ownership of the mutex ... timeout if we can't get it ...
      while (!/*Win32*/::WaitForSingleObject(m_mutex, __mutex_wait_period) == WAIT_OBJECT_0)
      {
#if ENABLE_MUTEX_DIAGNOSTICS
         Log("Mutex wait ...  request : %d ", lock_id);

         if (m_locked_by > 1)
            Log("locked by %d  ", m_locked_by);

         if (++counter > max_lock_attempts)
            return false;
#endif

         // If we can't get it, go do something else ...
         if (!wait_for_access)
            return false;

         Inf_Yield();
      }
#endif

#if ENABLE_MUTEX_DIAGNOSTICS
   m_locked_by = lock_id;
#endif
   }

#endif

   return m_mutex != NULL;
}


bool _ProtectedAccess::Unlock()
{
   uint_32 rc = 1;

#if MUTEX_ENABLED
   if (m_mutex)
   {
#if defined _WIN32 || defined _WIN64
      uint_32 rc = /*Win32*/::ReleaseMutex(m_mutex);
#endif

      if (rc == 0)
      {
#if ENABLE_MUTEX_DIAGNOSTICS
         uint_32 error = /*Win32*/::GetLastError();

         Log("UPDATEINFO: Win32 Error (ReleaseMutex,GetLastError=%d", error);
#endif
      }
   }
#endif

#if ENABLE_MUTEX_DIAGNOSTICS
   m_locked_by = -1;
#endif

   return !!rc;
}


class UpdateInfo_Private : public _ProtectedAccess
{
public:
    UpdateInfo_Private();
    ~UpdateInfo_Private();

    HANDLE           m_handle;

    WndSet           m_dynamic_widgets;

    WndSet           m_popups;

    int_32            m_root_offset_x;
    int_32            m_root_offset_y;

    bool m_full_redraw_required;

    bool m_expired;

    int m_cursor;
};


RemoteDialogInfo *UpdateInfo::GetRemoteDialogInfo(void *user_data)
{
   if (!m_dialog_info)
      m_dialog_info = new RemoteDialogInfo(user_data);
   else
      m_dialog_info->m_user_data = user_data;

   return m_dialog_info;
}


UpdateInfo::UpdateInfo(void *native_root, void *dest)
{
   m_dialog_info = nullptr;
   
   m_content_string = nullptr;
   m_current_java_popup = nullptr;

   UpdateInfo_Private *priv = new UpdateInfo_Private();
   m_private = (void*) priv;

   m_user_data = dest;

   m_popup_close_notify_required = 0;
   m_full_redraw_required = false;
   m_ignore_update = false;

   m_exclusive = false;
   m_exclusive_trigger = false;

   m_cursor_position_x = 0;
   m_cursor_position_y = 0;

   Initialize(native_root);
}


void *UpdateInfo::GetInterface(uint_32 width, uint_32 height)
{
   //Log("GetInterface (%d, %d)\n", width, height);
   
#if 0 // defined _WIN32 || defined _WIN64
   if (m_user_data)
   {
      BasicTexture *bt = INF_CAST_PTR(BasicTexture, m_user_data);

      DeviceTexture *dt = bt->GetDeviceTexture(true);
      // BUG: Workaround weird apparent window resize in paint. Not sure what's going on.
      //if (bt->GetSurfaceWidth() == 0)
         return (void*) dt->GetInterface(width, height, false);
      //else
        // return (void*) dt->GetInterface(false);
   }
#endif

   return NULL;
}


void UpdateInfo::ReleaseInterface(void *interface_graphics_2D)
{
#if 0 // defined _WIN32 || defined _WIN64
   if (m_user_data)
   {
      BasicTexture *bt = INF_CAST_PTR(BasicTexture, m_user_data);
      DeviceTexture *dt = bt ? bt->GetDeviceTexture(true) : NULL;
      
      if (dt)
         dt->ReleaseInterface((HDC)interface_graphics_2D);
   }
#endif
}


UpdateInfo_Private::UpdateInfo_Private()
{
   m_root_offset_x = 0;
   m_root_offset_y = 0;
   m_cursor        = 0;

   m_expired       = false;
}


void UpdateInfo::Initialize(void *native_root)
{
#if defined _WIN32 || defined _WIN64
   /*Win32*/::SetPropA((HWND)native_root, UPDATE_INFO_ID, (HANDLE) this);

   RECT rect;
   /*Win32*/::GetWindowRect((HWND)native_root, &rect);

   GET_PRIVATE(UpdateInfo)->m_root_offset_x = rect.left;
   GET_PRIVATE(UpdateInfo)->m_root_offset_y = rect.top;
#endif
}


UpdateInfo_Private::~UpdateInfo_Private()
{

}


void UpdateInfo::SetCursor(int c)
{
   GET_PRIVATE(UpdateInfo)->m_cursor = c;
}


int UpdateInfo::GetCursor()
{
   return GET_PRIVATE(UpdateInfo)->m_cursor;
}


// TODO: Switch this to uint_32 return type. No need for sign.
int UpdateInfo::GetVisiblePopupCount()
{
   return int(GET_PRIVATE(UpdateInfo)->m_popups.size());
}


void UpdateInfo::GetVisiblePopups(void **dest_native_popup_wnds)
{
   WndSet &ws = GET_PRIVATE(UpdateInfo)->m_popups;

   for (WndSet::iterator i = ws.begin(); i != ws.end(); ++i)
   {
      *(dest_native_popup_wnds++) = *i;
   }
}


void UpdateInfo::RegisterDynamicRegion(void *native_window)
{
   if (native_window)
   {
      _MarkDynamic(native_window);
      GET_PRIVATE(UpdateInfo)->m_dynamic_widgets.insert(native_window);
   }
}


void UpdateInfo::UnregisterDynamicRegion(void *native_window)
{
   if (native_window)
   {
      GET_PRIVATE(UpdateInfo)->m_dynamic_widgets.erase(native_window);
   }
}


void UpdateInfo::RegisterFullUpdateRequired()
{
   m_full_redraw_required = true;
}


bool UpdateInfo::IsFullUpdateRequired()
{
   return m_full_redraw_required;
}


void UpdateInfo::Reset()
{
#if defined _WIN32 || defined _WIN64
    m_update_region.clear();
#endif
     
    m_full_redraw_required = false;
}


void UpdateInfo::RegisterDirtyRectangle(void *rect)
{
   // No point registering update region if we have to redraw the whole surface.
   if (m_full_redraw_required)
       return;

   if (!rect)
      return;

#if defined _WIN32 || defined _WIN64

   auto r = (RECT *) rect;
   
   DirtyRect ur;

   ur.x = (short) r->left;
   ur.y = (short) r->top;

   ur.width  = (short) (r->right - r->left);
   ur.height = (short) (r->bottom - r->top);


   // OPT: Skip duplicates.
   for (std::vector<DirtyRect>::iterator i = m_update_region.begin();
                                        i != m_update_region.end(); ++i)
   {
       const DirtyRect &r = *i;

       if (!memcmp(&r, &ur, sizeof(DirtyRect)))
         return;
   }
   
   m_update_region.push_back(ur);
       
#if LOG_INVALIDATES
   Logf("_InvalidateRect (%d, %d, %d, %d)\n", ur.x, ur.y, ur.width, ur.height);
#endif

#endif

}


void UpdateInfo::RegisterUpdateWindow(void *native_window)
{
   // No point registering update regions if we have to draw entire surface anyway.
   if (!m_full_redraw_required)
   {
#if defined _WIN32 || defined _WIN64

     // Add window rect to update region.
     //Log("RegisterUpdateWindow");
#endif
   }
}


static bool _IsChildOf(HWND parent, void *native_window)
{
   for (;;)
   {
      if (native_window == parent)
         return true;

#if defined _WIN32 || defined _WIN64
      native_window = /*Win32*/::GetParent((HWND)native_window);
#endif

      if (!native_window)
         return false;
   }
}


void UpdateInfo::UpdateDynamicRegion(void *view_root)
{
#if defined _WIN32 || defined _WIN64
   WndSet &ws = GET_PRIVATE(UpdateInfo)->m_dynamic_widgets;

   if (ws.empty())
      return;

   RECT vrect;
   /*Win32*/::GetWindowRect((HWND) view_root, &vrect);

   for (WndSet::iterator i = ws.begin(); i != ws.end(); i++)
   {
      HWND hwnd = (HWND) *i;

      // Skip any dynamic widgets that are not present within the view root's hierarchy.
      if (!_IsChildOf((HWND)view_root, hwnd))
         continue;

      // TODO: [OPT] We might be able to use the update region which could reduce unnecessary copies
      //             when parts of the plugin's display area are not modified.

      RECT rect;

      HDC hdc = /*Win32*/::GetDC(hwnd);

      int cb = /*Win32*/::GetClipBox(hdc, &rect);

      RECT wrect;
      /*Win32*/::GetWindowRect(hwnd, &wrect);

      if (cb == NULLREGION)
         rect = wrect;
      else
         /*Win32*/::OffsetRect(&rect, wrect.left, wrect.top);

      /*Win32*/::ReleaseDC(hwnd, hdc);

      /*Win32*/::OffsetRect(&rect, -vrect.left, -vrect.top);

      RegisterDirtyRectangle((void*)&rect);
   }
#endif
}





// Content side debug logging - picked up & processed in control side.
// TODO: Add support for format strings, multiple entries, if required.
void UpdateInfo::Log(const char *format, ...)
{
   va_list arg_list;
   va_start(arg_list, format);

#if defined _WIN32 || defined _WIN64
   int len = _vscprintf(format, arg_list);
#else
    char *tmp = NULL;
    // TODO[OPT]: Use the string returned by vasprintf directly rather than creating a second copy.
    int len = vasprintf(&tmp, format, arg_list);
    free(tmp);
#endif

   m_content_string = new char[len+1];

   vsprintf(m_content_string, format, arg_list);

#ifdef _DEBUG

#if defined _WIN32 || defined _WIN64
   /*Win32*/::OutputDebugString(m_content_string);
#else
    printf("%s\n", m_content_string);
#endif

#endif
}


void UpdateInfo::ClearDiagnostics()
{
   if (m_content_string)
   {
      delete [] m_content_string;
      m_content_string = nullptr;
   }
}


UpdateInfo::~UpdateInfo()
{
   if (m_dialog_info)
      delete m_dialog_info; 
   
   ClearDiagnostics();
   delete GET_PRIVATE(UpdateInfo);
}


void UpdateInfo::DialogParam_Set(char *parameter, void *_value_)
{
   wchar_t *_value = (wchar_t*) _value_;

   // Assume the value is a unicode string, unless the parameter begins with a #, indicating a float.
   if (parameter)
   {
      RemoteDialogInfo *di = GetRemoteDialogInfo(m_user_data);
      di->m_parameters.Insert(parameter, _value);
   }
}


void UpdateInfo::DialogParam_Set(char *parameter, float value)
{
   if (parameter)
      GetRemoteDialogInfo(m_user_data)->m_parameters.Insert(parameter, value);
}


bool UpdateInfo::DialogParam_Get(char *parameter, float &dest)
{
   if (!parameter)
      return false;

   Parameters &p = GetRemoteDialogInfo(m_user_data)->m_parameters;

   return p.Get(parameter, dest);
}


bool UpdateInfo::DialogParam_Get(char *parameter, void *&dest)
{
   if (!parameter)
      return false;

   Parameters &p = GetRemoteDialogInfo(m_user_data)->m_parameters;

   return p.Get(parameter, dest);
}


void UpdateInfo::ResetDialog()
{
   Parameters &params = GetRemoteDialogInfo(m_user_data)->m_parameters;

   params.Reset();
}


void UpdateInfo::OpenModalDialog()
{
   RemoteDialogInfo *di = GetRemoteDialogInfo(m_user_data);

   di->m_exit_code = 0; // 0 = Running, has not yet exited.

#if 0 //defined _WIN32 || defined _WIN64
   HWND hwnd = /*Win32*/::FindWindowEx(nullptr, nullptr, TEXT("InfinityClass"), nullptr);
   assert(hwnd);

   /*Win32*/::PostMessage(hwnd, INF_DIALOG_OPEN, 0, (LPARAM) di);
#else
   di->m_open_pending = true;
#endif
}


bool UpdateInfo::IsDialogDataEntryComplete()
{
   return GetRemoteDialogInfo(m_user_data)->m_exit_code != 0;
}


uint_32 UpdateInfo::GetDialogExitCode()
{
   RemoteDialogInfo *di = GetRemoteDialogInfo(m_user_data);
   return di->m_exit_code - 1;
}


 void  _MarkDynamic(void *native_window)
{
#if defined _WIN32 || defined _WIN64
   /*Win32*/::SetPropA((HWND)native_window, _DYNAMIC_MARKER_ID, (HANDLE) 1);
#endif
}


bool _IsDynamic(void *native_window)
{
#if defined _WIN32 || defined _WIN64
   return !!/*Win32*/::GetPropA((HWND)native_window, _DYNAMIC_MARKER_ID);
#else
    return true;
#endif
}


bool _IsParentDynamic(void *native_window)
{
   while (native_window)
   {
      if (_IsDynamic(native_window))
         return true;
#if defined _WIN32 || defined _WIN64
      native_window = /*Win32*/::GetParent((HWND) native_window);
#endif
   }

   return false;
}


static HWND _GetHierarchyRoot(void *native_window)
{
#if defined _WIN32 || defined _WIN64
   if (!native_window)
      return nullptr;

   if (!/*Win32*/::IsWindow((HWND) native_window))
   {
      assert(false);
      return nullptr;
   }

   // Find root of the window hierarchy ...
   for (;;)
   {
       HWND parent = /*Win32*/::GetParent((HWND) native_window);

       if (parent)
           native_window = parent;
       else
           break;
   }

#endif

   return (HWND) native_window;
}


// Pass in the window we want the update_info for.
// The update_info is stored in the hierarchy root.
 UpdateInfo *  _GetUpdateInfo(void *native_window, bool wait_for_access)
{
#if defined _WIN32 || defined _WIN64
   native_window = _GetHierarchyRoot(native_window);

   UpdateInfo *info = INF_CAST_PTR(UpdateInfo, (native_window ? /*Win32*/::GetPropA((HWND) native_window, UPDATE_INFO_ID) : nullptr));

   bool locked = false;

   if (info)
   {
      if (!GET_PRIVATE_PTR(UpdateInfo, info)->m_expired)
          locked = GET_PRIVATE_PTR(UpdateInfo, info)->Lock(1, wait_for_access);

      //info->Display("_GetUpdateInfo: %p\n", native_window);
   }

   return locked ? info : nullptr;
#else
    return NULL;
#endif
}


 void  _ReleaseUpdateInfo(UpdateInfo *info)
{
   if (info)
   {
      GET_PRIVATE_PTR(UpdateInfo, info)->Unlock();
   }
}


 UpdateInfo *  _CreateUpdateInfo(void *native_window, void *dest)
{
   HWND root = _GetHierarchyRoot(native_window);

   return new UpdateInfo(root, dest);
}


 void  _DestroyUpdateInfo(void *native_window)
{
   HWND root = _GetHierarchyRoot(native_window);

   // Lock
   UpdateInfo *info = _GetUpdateInfo(root, true);

   if (info)
   {
      // Mark as expired ...
      GET_PRIVATE_PTR(UpdateInfo, info)->m_expired = true;

#if defined _WIN32 || defined _WIN64
      // Clear the native window property pointer.
      /*Win32*/::SetPropA(root, UPDATE_INFO_ID, nullptr);
#endif

      // Unlock ...
      _ReleaseUpdateInfo(info);

      // Zap ...
      delete info;
   }
}



#define USE_MSG_MARKER 0

// LOAD_BALANCE - maximum number of messages to be processed per cycle.

// Return code : Flags : 1: Search message is found.
//                       2: Exception (error) generated. Not currently implemented. See comment below.


bool ProcessNativeWindowMessages(void *monitor_hwnd, bool *early_out, unsigned int exit_at_message)
{
   // Note: Exception handlers around message loops don't seem to work on Windoze.
   // An exception handler must be placed in each message processing callback.
   // TODO: Research possible generic solutions to enable an exception handler to be placed here.

   bool found_message = false;
   bool found_exit_msg = false;

   bool display_change_in_progress = false;

   bool marker_inserted = false;

   uint32 max_messages_to_process;

   // LOAD_BALANCE - message processing takes significant time, so keep the number of messages per cycle as low as possible

#ifdef INF_DEBUG
   uint32 __message_quota_per_normal_cycle = 20;
#else
   uint32 __message_quota_per_normal_cycle = 20;
#endif

#if USE_MSG_MARKER
   // Drop a marker in the pipe so we know when we've processed all outstanding messages.
   if (!PostMessage((Desktop_Window) monitor_hwnd, __message_marker, 0, GetCurrentThreadId()))
      max_messages_to_process = 0; // submit failed because queue full so clear it.
   else
   {
      max_messages_to_process = __message_quota_per_normal_cycle;
      marker_inserted = true;
   }
#else
   max_messages_to_process = __message_quota_per_normal_cycle;
#endif

   bool found_marker = false;

   MSG msg;

   while (/*Win32*/::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
   {
      if (early_out && *early_out)
         break;

      // Run a second message processing cycle to ensure display mode changes function properly.
	   // Required for correct DX11 alt-enter behaviour.
	   if (msg.message == WM_DISPLAYCHANGE)
		   max_messages_to_process += 5000;

       /*Win32*/::TranslateMessage(&msg);
       /*Win32*/::DispatchMessage(&msg);

       if (max_messages_to_process > 0)
       {
          max_messages_to_process--;

          if (max_messages_to_process == 0)
          {
             if (marker_inserted && !found_marker)
                  max_messages_to_process = __message_quota_per_normal_cycle;
             else
               break;
          }
       }

#if USE_MSG_MARKER
      if (monitor_hwnd && (msg.hwnd == (Desktop_Window) monitor_hwnd) && (msg.message == __message_marker) &&
          (msg.lParam == GetCurrentThreadId()) )
         found_marker = true;
#endif

      if (monitor_hwnd && (msg.hwnd == (Desktop_Window) monitor_hwnd) && (msg.message == exit_at_message))
      {
         found_exit_msg = true;
         break;
      }
   }

   return found_exit_msg;
}



 void  _DebugBreak()
{
#if defined _WIN32 || defined _WIN64
   // Find application window ...
   HWND hwnd = /*Win32*/::FindWindowEx(NULL, NULL, TEXT("InfinityClass"), NULL);

   if (hwnd)
   {
      /*Win32*/::ShowWindow(hwnd, SW_HIDE);

      // Wait for render window to hide by processing windows messages ...
      while (/*Win32*/::IsWindowVisible(hwnd))
	   {
         uint_32 flags = ProcessNativeWindowMessages(hwnd, nullptr, 0);

         // Stop processing if an exception is generated
         if (flags & 2)
            break;

         // LOAD_BALANCE
         Inf_Yield();
	   }

      /*Win32*/::DebugBreak();
   }
   else
   {
     // /*Win32*/::MessageBeep(1);
   }
#endif
}


bool UpdateInfo::RequiresPopupDraw()
{
   return m_current_java_popup || GetVisiblePopupCount() > 0;
}



DirtyRect* UpdateInfo::Get(unsigned int index)
{
   if (index >= m_update_region.size())
      return nullptr;

   return &(m_update_region[index]);
}


 void * _GetFirstVisiblePopup(void *_parent)
{
   HWND parent = (HWND)_parent;

   if (!parent || !/*Win32*/::IsWindow(parent))
   {
      assert(false);
      return nullptr;
   }

   UpdateInfo *info = _GetUpdateInfo(parent, true);

   void **h = nullptr;

   if (info)
    {
       uint_32 c = info->GetVisiblePopupCount();

       if (c > 0)
       {
          h = new void*[c];
          info->GetVisiblePopups(h);
       }

       _ReleaseUpdateInfo(info);
    }

    return (h ? h[0] : NULL);
}



 bool  _InvalidateRect(void *wnd, void *rect)
{
   HWND hwnd = (HWND)wnd;
   RECT *region = (RECT *)rect;
   
   if (!hwnd || !/*Win32*/::IsWindow(hwnd))
   {
      assert(false);
      return FALSE;
   }

   UpdateInfo *info = _GetUpdateInfo(hwnd, true);

   if (info)
   {
      if (region)
      {
         RECT urect, wrect;
         memcpy(&urect, region, sizeof(RECT));
 
         /*Win32*/::GetWindowRect(hwnd, &wrect);
         /*Win32*/::OffsetRect(&urect, wrect.left, wrect.top);
  
         info->RegisterDirtyRectangle((void*)&urect);
      }
      else
         info->m_full_redraw_required = true;

      _ReleaseUpdateInfo(info);
   }
   else
      return false;

   return true;
}


 bool  _UpdateWindow(void *wnd)
{
   HWND hwnd = (HWND)wnd;
   
   if (!hwnd || !/*Win32*/::IsWindow(hwnd))
   {
      assert(false);
      return false;
   }

   // TODO: Check this doesn't break window based functionality if gated again.
   // currently popups (menus/combos) & window based plugins.
   //if (!_IsAdvSwPatchActive())
      return !!/*Win32*/::UpdateWindow(hwnd);

   // Ignore request - we log dirty region notifications in _InvalidateRect, then
   // render in the texture processing loop.
   return true;
}



 void  _RegisterUpdateRegion(void *native_wnd)
{
   if (_Gecko_OffscreenSharedSurfaceMode())
   {
      UpdateInfo *info = _GetUpdateInfo(native_wnd, true);

      if (info)
      {
         info->RegisterUpdateWindow(native_wnd);
         _ReleaseUpdateInfo(info);
      }
  }
}



// TODO[OPT]: windoze/DX11/9 - Rework as a DX texture/texture copy.



 PopupInfo *  _GetPopupInfo(void *native_window, bool wait_for_access)
{
#if defined _WIN32 || defined _WIN64

   PopupInfo *info = INF_CAST_PTR(PopupInfo, (native_window ? /*Win32*/::GetPropA((HWND) native_window, POPUP_INFO_ID) : nullptr));

   bool locked = false;

   if (info)
   {
      if (!info->m_expired)
          locked = info->Lock(1, wait_for_access);
   }

   return locked ? info : NULL;
#else
    return NULL;
#endif
}


 void  _UnlockPopupInfo(void *pinfo)
{
   if (pinfo)
      ((PopupInfo *)pinfo)->Unlock();
}


 void  _DestroyPopupInfo(void *native_window)
{
   // Lock
   PopupInfo *info = _GetPopupInfo(native_window, true);

   if (info)
   {
      // Mark as expired ...
       info->m_expired = true;

#if defined _WIN32 || defined _WIN64
      // Clear the native window property pointer.
      /*Win32*/::SetPropA((HWND) native_window, POPUP_INFO_ID, nullptr);
#endif

      // Unlock ...
      _UnlockPopupInfo(info);

      // Zap ...
      delete info;
   }
}


static HDC __GetDC(void *gdi_accessible_surface)
{
   HDC hdc=0;

   if (__backend_dx11)
   {
      if (gdi_accessible_surface)
      {
         HRESULT hr = ((IDXGISurface1*)gdi_accessible_surface)->GetDC(FALSE, &hdc);

         if (hr != S_OK)
         {
            Infinity_Log(INF_LOG_FILE, 0, "Could not retrieve HDC from IDXGISurface1.");
         }
      }
   }
   else
   {
      hdc = (HDC) gdi_accessible_surface;
   }
  
   return hdc;
}


static void __ReleaseDC(void *gdi_accessible_surface)
{
   if (__backend_dx11 && gdi_accessible_surface)
   {
      RECT empty;
      SetRect(&empty, 0,0,0,0);
      ((IDXGISurface1*)gdi_accessible_surface)->ReleaseDC(&empty);
   }
}


 void * _LockPopupInfo(void *native_widget)
{
   return (void*) _GetPopupInfo(native_widget, true);
}


 void * _SetPopupData(void *_pi, void *data)
{
   if (_pi)
   {
      ((PopupInfo *) _pi)->m_data = data;
   }

   return nullptr;
}


 void * _GetPopupData(void *_pi)
{
   if (_pi)
      return ((PopupInfo *) _pi)->m_data;

   return nullptr;
}


 bool AdvSw_GetInterface(void **iface, uint_32 interface_id)
{
   /* RenderingDevice *dev = GetRenderingDevice();
   
   if (dev)
   {
      *iface = dev->GetNative(interface_id);
      return true;
   }
   else*/
      return false;
}


void UpdateInfo::PopupShow(void *native_widget)
{
    GET_PRIVATE(UpdateInfo)->m_popups.insert(native_widget);
}


void UpdateInfo::PopupHide(void *widget)
{
   _DestroyPopupInfo(widget);

	UpdateInfo_Private *up = GET_PRIVATE(UpdateInfo);

   // 
   m_popup_close_notify_required = true;

    up->m_popups.erase(widget);

    // For now, when a popup is hidden we completely redraw the page.
    // [OPT] - extract the popup rectangle & pass that dirty rectangle.
    // TODO: DX11 implementation processes this inside a moz patch.
    // this can probably go soon as unnecessary overkill.
    m_full_redraw_required = true;
}




 void  UpdateInfo_Configure(uint_32 popup_pixel_format, bool mode_dx9)
{
   __backend_dx11 = !mode_dx9;
   __popup_pixel_format = (Infinity::PixelFormat) popup_pixel_format;
}


static PopupInfo * _CreatePopupInfo(void *native_window)
{
   auto pi = new PopupInfo;

#if defined _WIN32 || defined _WIN64
   /*Win32*/::SetPropA((HWND)native_window, POPUP_INFO_ID, (void *) pi);
#endif

   return pi;
}


 void  _PopupShow(void *native_widget, void *_native_owner, void *widget, uint_32 flags)
{
   HWND native_owner = (HWND)_native_owner;
   
    if (!native_owner || !/*Win32*/::IsWindow(native_owner))
    {
       assert(false);
       return;
    }

    UpdateInfo *info = _GetUpdateInfo(native_owner, true);

    if (info)
    {
       auto pi = _CreatePopupInfo(native_widget); 
       pi->m_widget = widget;
       
       info->PopupShow(native_widget);
       _ReleaseUpdateInfo(info);
    }
}


 void  _PopupHide(void *widget, void *_owner)
{
    HWND owner = (HWND)_owner;
   
    if (!owner || !/*Win32*/::IsWindow(owner))
    {
       assert(false);
       return;
    }

    UpdateInfo *info = _GetUpdateInfo(owner, true);

    if (info)
    {
       info->PopupHide(widget);
       _ReleaseUpdateInfo(info);
    }
}


// Replacement for ::GetMessagePos. 
// Uses infinity virtual cursor position rather than system desktop cursor location.
 DWORD  _GetMessagePos(void *wnd)
{
   HWND hwnd = (HWND)wnd;
   UpdateInfo *info = _GetUpdateInfo(hwnd, true);
   
   DWORD result = 0;
   if (info)
   {
      // Required format: x coordinate in low order short, y in high order short.
      result = DWORD(info->m_cursor_position_x) | (DWORD(info->m_cursor_position_y) << 16);
      _ReleaseUpdateInfo(info);
   }

   return result;
}



 void  _DiscardUpdate(void *native_owner)
{
    if (!native_owner || !/*Win32*/::IsWindow((HWND) native_owner))
    {
       assert(false);
       return;
    }

    UpdateInfo *info = _GetUpdateInfo(native_owner, true);

    if (info)
    {
       info->m_ignore_update = true;
       _ReleaseUpdateInfo(info);
    }
}



// We render all pages into this intermediate scratch surface.
// Stub to call from external appliction defined in adv_WindowGfx.cpp


// We render all pages into this intermediate scratch surface.
static ID3D11Texture2D * __render_target = nullptr;
static void * __render_target_device = nullptr;



 void  AdvSw_SetRenderTargetDevice(void *device)
{
   __render_target_device = device;
}


 void *  AdvSw_ConfigureRenderTarget(DWORD required_width, DWORD required_height)
{
    if (__render_target)
    {
       D3D11_TEXTURE2D_DESC current_desc;    
       __render_target->GetDesc(&current_desc);

       // If the current render target is smaller than we require, expand.
       if ( (required_width > current_desc.Width) || (required_height > current_desc.Height) )
       {
          required_width  = ___MAX(required_width, current_desc.Width);
          required_height = ___MAX(required_height, current_desc.Height);
          __render_target->Release();
          __render_target = nullptr;
       }
    }

    if ((required_width == 0) || (required_height == 0))
    { 
       if (__render_target)
       {
          __render_target->Release();
          __render_target = nullptr;
       }
    }
    else if (!__render_target && __render_target_device)
    {
			// Creates a texture surface for rendering.
		   CD3D11_TEXTURE2D_DESC desc(MOZ_DXGI_RENDER_TARGET_PIXEL_FORMAT, required_width, required_height, 1, 1);
		   desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		   desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

		   HRESULT hr = ((ID3D11Device*) __render_target_device)->CreateTexture2D(&desc, nullptr, &__render_target);

         if (hr != S_OK)
         {
            Infinity_Log(INF_LOG_DEFAULT, INF_INFO, "Couldn't create webpage render target (out of memory ?)");
            return nullptr;
         }
	}

   if (__render_target)
       return __render_target;
   else
       return nullptr;
}


 PopupInfo::~PopupInfo()
 {
#if defined _WIN32 || defined _WIN64
      if (m_data) 
      {
         if (__backend_dx11)
         {
            ((IDXGISurface1*)m_data)->Release();
            _SetPopupData((void*) this, nullptr);
         }

         // TODO: Zap Bitmap or RenderTarget.
      }
#endif
 }

#endif