#if defined _WIN32 || defined _WIN64

#include <windows.h>
#include <assert.h>

#include "_update_info.h"
#include "adv_sw_patch.h"

class _Find
{
public:
   _Find(const POINT &_p)
   {
      result = nullptr;
      depth = -1;
      p = _p;
   }

   POINT p;
   HWND  result;
   int   depth;
};


static bool _IsChildOf(HWND parent, HWND hwnd)
{
   if (!parent || !/*Win32*/::IsWindow(parent))
   {
      assert(0);
      return false;
   }

   if (!hwnd || !/*Win32*/::IsWindow(hwnd))
   {
      assert(0);
      return false;
   }

   for (;;)
   {
      if (hwnd == parent)
         return true;

      hwnd = /*Win32*/::GetParent(hwnd);

      if (!hwnd)
         return false;
   }
}


static int _GetDepth(HWND hwnd)
{
   if (!hwnd || !/*Win32*/::IsWindow(hwnd))
   {
      assert(0);
      return 0;
   }

   int depth = 0;

   for (;;)
   {
      hwnd = /*Win32*/::GetParent(hwnd);

      if (!hwnd)
         return depth;

      depth++;
   }
}


static bool _IsPointInside(HWND hwnd, const POINT &p)
{
   if (!hwnd || !/*Win32*/::IsWindow(hwnd))
   {
      assert(0);
      return false;
   }

   RECT rect;

#if 0  
   // Get client rectangle ...
   RECT wrect;
   /*Win32*/::GetWindowRect(hwnd, &wrect);
   /*Win32*/::GetClientRect(hwnd, &rect);

   // Offset client rect into screen coordinates.
   rect.left += wrect.left;
   rect.top += wrect.top;
#else
    // Get window rectangle ...
   /*Win32*/::GetWindowRect(hwnd, &rect);
#endif

   // Is point inside the rectangle ?
   return (p.x>=rect.left) && (p.x<=rect.right) && (p.y>=rect.top) && (p.y<=rect.bottom);
}


static BOOL CALLBACK EnumFindChildProc(HWND hwnd, LPARAM lParam)
{
   if (!hwnd || !/*Win32*/::IsWindow(hwnd))
   {
      assert(0);
      return FALSE;
   }

   _Find *f = (_Find*) lParam;

   // Our offscreen webpage rendering solution hides all webpage hierarchy native windows.
   // TODO: Implement a way of flagging the native window should be ignored via Set/GetProp on the HWND instread if selective hiding is required.
   bool candidate = true;// /*/*Win32*/::IsWindowVisible(hwnd);

   if (candidate && _IsPointInside(hwnd, f->p))
   {
      int depth = _GetDepth(hwnd);

      if (depth > f->depth)
      {
         f->result = hwnd;
         f->depth = depth;
      }
   }

   return TRUE;
}


// Replacement for ::WindowFromPoint that searches recursively rather than
// only direct children.
void * _WindowFromPoint(void *wnd, void *_p)
{
   HWND hwnd = (HWND) wnd;
   const POINT &p = *((POINT*)_p);

   if (!hwnd)
      return nullptr;

   if (!/*Win32*/::IsWindow(hwnd))
   {
      assert(0);
      return nullptr;
   }

   if (!_Gecko_OffscreenSharedSurfaceMode())
      return /*Win32*/::WindowFromPoint(p);

   // Find hierarchy root ...
   HWND root = hwnd;

   while (hwnd)
   {
      hwnd = /*Win32*/::GetParent(hwnd);

      if (hwnd)
         root = hwnd;
   }

   // Find window ...
   _Find find(p);

   /*Win32*/::EnumChildWindows(root, EnumFindChildProc, (LPARAM) &find);

   if (!find.result)
   {
      if (_IsPointInside(root, p))
         find.result = root;
   }

   return (void*) find.result;
}

#endif