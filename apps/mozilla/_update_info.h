/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Mozilla App


File        : _update_info.h

Description : Gecko -> Control Application communications interface.

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

#ifndef _UPDATE_INFO_H
#define _UPDATE_INFO_H

#include <vector>
#include <mutex>

#pragma warning (disable : 4786)

const unsigned int INF_PATCH_GECKO_HWND_DISPLAY_HANDLE_ID = 0;
const unsigned int INF_PATCH_GECKO_HWND_PRESENT_INFO = 1;
const unsigned int INF_PATCH_GECKO_HWND_NUM_REQUIRED_DATA_SLOTS = 2;  // Ensure this is always last required index + 1

typedef unsigned long uint_32;
typedef signed long    int_32;
typedef unsigned char uint_8;

#define MOZ_DXGI_RENDER_TARGET_PIXEL_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM_SRGB

// Note: DirtyRect shared with [mozilla]layout\base\nsPresShell.cpp
// Perform a full moz rebuild if this changes.

class Rectangle
{
   public:
	Rectangle() : x(0), y(0), width(0), height(0) {}
   Rectangle(float aX, float aY, float aWidth, float aHeight)
   {
      x = aX; y = aY;
      width = aWidth; height = aHeight;
   }

   float x, y;
   float width, height;
};

typedef class Rectangle DirtyRect;



// Update Info behaviour flags. Top two are probably redundantant now.
#define UINFO_IGNORE_PAINT     1
#define UINFO_RTT_DC           2
#define UINFO_RENDER_METHOD_2  4
#define UINFO_NATIVE_POPUP_TRANSPARENCY 8

class RemoteDialogInfo;

class UpdateInfo
{
public:
     UpdateInfo(void *native_root, void *dest);
     ~UpdateInfo();

    void *  GetInterface(uint_32 width, uint_32 height);
    void    ReleaseInterface(void *surf);

    void  RegisterDynamicRegion(void *native_window);
    void  UnregisterDynamicRegion(void *native_window);

    void  RegisterFullUpdateRequired();
    bool  IsFullUpdateRequired();

    int   GetVisiblePopupCount();
    void  GetVisiblePopups(void **);

    void  RegisterDirtyRectangle(void *rect);
    void  RegisterUpdateWindow(void *native_window);

    void  Reset();

    void  PopupShow(void *widget);
    void  PopupHide(void *widget);

    void  SetCursor(int c);
    int   GetCursor();

    void  UpdateDynamicRegion(void *view_root);

    void  ClearDiagnostics();

   // Logging (performed on the content side)
    void Log(const char *format, ...);

    void  DialogParam_Set(char *parameter, void *value);
    void  DialogParam_Set(char *parameter, float value);

    bool  DialogParam_Get(char *parameter, void *&dest);
    bool  DialogParam_Get(char *parameter, float &dest);

    void  OpenModalDialog();
    bool  IsDialogDataEntryComplete();
    unsigned long int  GetDialogExitCode();
    void  ResetDialog();


   // TODO: Make generic - rename WebpagePopups to Overlays.
   void DrawWebpagePopups(void *&dest_dc, int_32 offset_x, int_32 offset_y, int_32 image_width, int_32 image_height);
   void DrawWebpagePopup(void *hwnd_popup, void *&dest_dc, int_32 offset_x, int_32 offset_y, int_32 image_width, int_32 image_height);
   bool RequiresPopupDraw();
   RemoteDialogInfo *GetRemoteDialogInfo(void *user_data);

   DirtyRect *Get(unsigned int index);

   std::vector<DirtyRect> m_update_region;

   bool m_popup_close_notify_required;
   void *m_current_java_popup;

   char *m_content_string;  // ... which is retrieved in the plugin and passed through to the diagnostics handler.
   void *m_private;

   void *m_user_data;

   bool m_full_redraw_required;
   bool m_ignore_update;
   uint_32 m_cursor_position_x;
   uint_32 m_cursor_position_y;

   bool m_exclusive;
   bool m_exclusive_trigger;

   RemoteDialogInfo *m_dialog_info;

private:

   void Initialize(void *native_root);
};


UpdateInfo *  _GetUpdateInfo(void *native_window, bool wait_for_access = true);

void  _ReleaseUpdateInfo(UpdateInfo *info);

UpdateInfo *  _CreateUpdateInfo(void *native_window, void *user_data);
void  _DestroyUpdateInfo(void *native_window);

void  _MarkDynamic(void *native_window);
bool  _IsDynamic(void *native_window);
bool  _IsParentDynamic(void *native_window);
void  _DebugBreak();

void  _RegisterUpdateRegion(void *native_wnd);

bool AdvSw_GetInterface(void **iface, uint_32 iface_id);

bool AdvSW_Plugin_Capability_DX9();

void  UpdateInfo_Configure(uint_32 popup_pixel_format, bool mode_dx9);

bool  _UpdateWindow(void *wnd);
bool  _InvalidateRect(void *wnd, void *rect);

void * _GetFirstVisiblePopup(void *parent);
void  _PopupHide(void *widget, void *native_owner);
void  _PopupShow(void *native_widget, void *native_owner, void *moz_widget, uint_32 flags);


void * _LockPopupInfo(void *native_widget);
void * _GetPopupData(void *pi, uint_32 id);
void * _SetPopupData(void *pi, void *data);
void   _UnlockPopupInfo(void *native_widget);

void * _WindowFromPoint(void *wnd, void *point);
uint_32  _GetMessagePos(void *wnd);

void  _DiscardUpdate(void *native_owner);

class _ProtectedAccess
{
public:
   _ProtectedAccess();
   virtual ~_ProtectedAccess();

   bool Lock(uint_32 lock_id, bool wait_for_access=true);
   bool Unlock();

   std::mutex m_mutex;

#if ENABLE_MUTEX_DIAGNOSTICS
   int_32 m_locked_by;
#endif
};


class PopupInfo : public _ProtectedAccess
{
public:
   PopupInfo() 
   { 
      _SetPopupData((void*) this, nullptr);

      m_expired = false;
      m_width=0;
      m_height=0;
      m_widget = nullptr;
   }

   ~PopupInfo();
   
   void   *m_data;
   void   *m_widget;
   uint_32 m_width;
   uint_32 m_height;
   bool    m_expired;
};


 PopupInfo *  _GetPopupInfo(void *native_window, bool wait_for_access);

#endif // _UPDATE_INFO_H
