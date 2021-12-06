#ifndef ADV_BASE_WINDOW_CPP

#define ADV_BASE_WINDOW_CPP

#ifdef _WIN32

#include <windows.h>

static void *GetNativeWindowRenderTarget(void *_hwnd)
{
   // The paint listener we want is currently located one level down from the mozilla root..
   void *hwnd = (void*) ::GetWindow((HWND) _hwnd, GW_CHILD);
   return hwnd ? hwnd : _hwnd;
}

#else

static void *GetNativeWindowRenderTarget(void *_hwnd)
{
   return _hwnd;
}

#endif  // _WIN32

#endif // ADV_BASE_WINDOW_CPP