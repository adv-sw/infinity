/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Mozilla App


File        : adv_sw_moz_sizetocontent.cpp

Description : Replacement for Gecko SizeToContent.

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


#ifndef ADV_SW_PATCH_SIZE_TO_CONTENT
#define ADV_SW_PATCH_SIZE_TO_CONTENT

#include "adv_sw_patch.h"
#include "_update_info.h"
#include <assert.h>

#include "nsIScrollableFrame.h"
#include "nsViewManager.h"
#include "nsDeviceContext.h"

#define _ADV_DEBUG_RESIZE 0


static nsIScrollableFrame *_GetScrollInfo(nsIPresShell *presShell, nsPresContext *presContext,
   float &scrollbar_width, float &scrollbar_height)
{
   nsIScrollableFrame *scrollable = presShell->GetRootScrollFrameAsScrollable();

   if (scrollable)
   {
      RefPtr<gfxContext> rcx(presShell->CreateReferenceRenderingContext());
      nsMargin sb = scrollable->GetDesiredScrollbarSizes(presContext, rcx);

      scrollbar_width = (float)presContext->AppUnitsToDevPixels(sb.LeftRight());
      scrollbar_height = (float)presContext->AppUnitsToDevPixels(sb.TopBottom());

      // TODO: Figure out why this is required.
      const float calc_inaccuracy_compensation = 25.0f;
      scrollbar_width  += calc_inaccuracy_compensation;
      scrollbar_height += calc_inaccuracy_compensation;

      return scrollable;
   }
   else
      return nullptr;
}


#if _ADV_DEBUG_RESIZE
void _AdvLogResizeInfo(nsIWidget *widget, bool hscroll, bool vscroll,
   float aspect_ratio, PRUint32 width, PRUint32 height)
{
   UpdateInfo *info = _GetUpdateInfo(widget, true);

   if (info)
   {
      char debug_str[1024];
      debug_str[0] = '\0';

      sprintf(debug_str, "(%d,%d) ", width, height);

      if (hscroll)
         strcat(debug_str, "hs ");

      if (vscroll)
         strcat(debug_str, "vs ");

      char aspect_str[255];
      sprintf(aspect_str, "ar=%f ", aspect_ratio);
      strcat(debug_str, aspect_str);

      info->Log(debug_str);

      _ReleaseUpdateInfo(info);
   }
}
#endif


void _AdvAspectAdjust(bool hscroll, bool vscroll, float aspect_ratio, float &width, float &height)
{
   float content_aspect = width / height;

   float correction = aspect_ratio / content_aspect;

   if (correction > 1)
   {
      // Adjust by reducing height, if content requires a scroll bar.
      if (vscroll)
         height = height / correction;
      else // or increasing width, if it does not.
         width = width * correction;
   }
   else
   {
      // ... and vice versa.
      if (hscroll)
         width  = width * correction;
      else
         height = height / correction;
   }
}


// Perform binary search to figure out ideal width by brute force.
// Required because moz built in SizeToContent is currently flaky.
static nscoord _GetPreferedWidth(nsPIDOMWindowOuter *rootWindow, nsIDocShellTreeOwner *treeOwner,
   nsIDocShellTreeItem *docShellAsItem,
   nsIScrollableFrame *scrollable, nsIPresShell *presShell,
   const float resizing_height, const nscoord max_width_app_units, const nscoord min_width_app_units, const nscoord default_width_app_units)
{
   // resizing_height must be at least enough height for a scroll bar, otherwise it won't be added.

   nscoord width = max_width_app_units;
   nscoord delta = max_width_app_units * 0.5f;

   bool invalid = false;

   while (delta > 10) // TODO: Tradeoff - lower value, more accurate, but takes longer to calculate.
   {
      nsresult rc = presShell->ResizeReflow(width, NS_UNCONSTRAINEDSIZE);

      if (rc != NS_OK)
      {
         invalid = true;
         break;
      }

      if (presShell)
         presShell->FlushPendingNotifications(FlushType::Layout);

      uint32_t scrollbar_flags = scrollable->GetScrollbarVisibility();

      bool horizontal_scroll_bar = scrollbar_flags & nsIScrollableFrame::HORIZONTAL;

      if (horizontal_scroll_bar)
      {
         // If we need a horizontal scroll bar when at the maximum setting, there is nothing we can do about it,
         // so just return, acknowledging this to be the case.
         if (width == max_width_app_units)
            return width;

         width += delta;
      }
      else
         width -= delta;

      delta *= 0.5f;
   }

   if (width < min_width_app_units)
      invalid = true;


   if (invalid)
   {
      // Couldn't get a width that looks valid from the page, so select default.
      width = default_width_app_units;

      nsresult rc = presShell->ResizeReflow(width, NS_UNCONSTRAINEDSIZE);

      if (rc != NS_OK)
      {
         // Log error
      }

      if (presShell)
         presShell->FlushPendingNotifications(FlushType::Layout);

   }

   return width;
}


// Use a binary partition search to figure out ideal height by brute force.
static nscoord _GetPreferedHeight(nsPIDOMWindowOuter* rootWindow, nsIDocShellTreeOwner *treeOwner,
   nsIDocShellTreeItem *docShellAsItem,
   nsIScrollableFrame *scrollable, nsIPresShell *presShell,
   const nscoord max_height, const nscoord prefered_width)
{
   nscoord height = max_height;
   nscoord delta = max_height * 0.5f;

   while (delta > 10) // TODO: Tradeoff - lower value, more accurate, but takes longer to calculate.
   {
      nsresult rc = presShell->ResizeReflow(prefered_width, height);
      
      if (rc != NS_OK)
         return 0.0f;

      if (presShell)
         presShell->FlushPendingNotifications(FlushType::Layout);

      uint32_t scrollbar_flags = scrollable->GetScrollbarVisibility();

      bool vertical_scroll_bar = scrollbar_flags & nsIScrollableFrame::VERTICAL;

      if (vertical_scroll_bar)
      {
         // If we need a vertical scroll bar when at the maximum setting, 
         // there is nothing we can do about it, so just max out.
         if (height == max_height)
            return height;

         height += delta;
      }
      else
         height -= delta;

      delta *= 0.5f;
   }

   return height;
}


// IN: max_pixels width/height
// OUT: content pixels width/height/
nsresult nsGlobalWindow::GetContainerSize(nscoord &width_pixels, nscoord &height_pixels,
                                 const uint32_t max_width_pixels, const uint32_t max_height_pixels,
                                 const uint32_t min_width_pixels, const uint32_t default_width_pixels)

{
   if (!mDocShell)
      return NS_ERROR_FAILURE;

   // The content viewer does a check to make sure that it's a content
   // viewer for a toplevel docshell.
   nsCOMPtr<nsIContentViewer> cv;
   mDocShell->GetContentViewer(getter_AddRefs(cv));
   if (!cv)
      return NS_ERROR_FAILURE;

#if 0
   // Sometimes fails. Infinity online features page, france24.com
   return cv->GetContentSizeConstrained(max_width_pixels, max_height_pixels,
      &width_pixels, &height_pixels);
#else
   nsCOMPtr<nsIDocShellTreeOwner> treeOwner = GetTreeOwner();

   if (!treeOwner)
      return NS_ERROR_FAILURE;

   nsCOMPtr<nsIPresShell> presShell = GetDocShell()->GetPresShell();

   if (!presShell)
      return NS_ERROR_FAILURE;

   nsPIDOMWindowOuter* rootWindow =
      static_cast<nsPIDOMWindowOuter *>(GetPrivateRoot());

   if (!rootWindow)
      return NS_ERROR_FAILURE;

   nsIFrame *root = presShell->GetRootFrame();

   if (!root)
      return NS_ERROR_NOT_INITIALIZED;

   nsViewManager   *vm = presShell->GetViewManager();
   nsDeviceContext *dc = vm->GetDeviceContext();

   RefPtr<nsPresContext> presContext;
   mDocShell->GetPresContext(getter_AddRefs(presContext));

   if (!presContext)
      return NS_ERROR_FAILURE;

   nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(mDoc->GetDocShell());

   float scrollbar_width = 0.0f, scrollbar_height = 0.0f;

   nsIScrollableFrame *scrollable = _GetScrollInfo(presShell, presContext, scrollbar_width, scrollbar_height);

   if (!scrollable)
      return NS_ERROR_NOT_INITIALIZED;

   nsCOMPtr<nsIBaseWindow> treeOwnerAsWin = GetTreeOwnerWindow();

   //RefPtr<gfxContext> rcx(presShell->CreateReferenceRenderingContext());

   nscoord min_width_app_units = presContext->DevPixelsToAppUnits(min_width_pixels);

   nscoord default_width_app_units = presContext->DevPixelsToAppUnits(default_width_pixels);
   
   nscoord max_width_app_units  = presContext->DevPixelsToAppUnits(max_width_pixels);
   nscoord max_height_app_units = presContext->DevPixelsToAppUnits(max_height_pixels);

   nscoord width_app_units  = _GetPreferedWidth(rootWindow, treeOwner, docShellAsItem, scrollable, presShell, scrollbar_height, max_width_app_units, min_width_app_units, default_width_app_units);
   nscoord height_app_units = _GetPreferedHeight(rootWindow, treeOwner, docShellAsItem, scrollable, presShell, max_height_app_units, width_app_units);

   // Return dimensions of the content, subject to constraints passed into the function.
   width_pixels  = presContext->AppUnitsToDevPixels(width_app_units);
   height_pixels = presContext->AppUnitsToDevPixels(height_app_units);
#endif

   return NS_OK;
}


nsresult nsGlobalWindow::SetContainerSize(nscoord width_pixels, nscoord height_pixels)
{
   if (!mDocShell)
      return NS_ERROR_FAILURE;

   // The content viewer does a check to make sure that it's a content
   // viewer for a toplevel docshell.
   nsCOMPtr<nsIContentViewer> cv;
   mDocShell->GetContentViewer(getter_AddRefs(cv));
   if (!cv)
      return NS_ERROR_FAILURE;

   nsCOMPtr<nsIDocShellTreeOwner> treeOwner = GetTreeOwner();

   if (!treeOwner)
      return NS_ERROR_FAILURE;

   nsCOMPtr<nsIPresShell> presShell = GetDocShell()->GetPresShell();

   RefPtr<nsPresContext> presContext;
   mDocShell->GetPresContext(getter_AddRefs(presContext));

   nscoord width_app_units  = presContext->DevPixelsToAppUnits(width_pixels);
   nscoord height_app_units = presContext->DevPixelsToAppUnits(height_pixels);

   nsresult rc = presShell->ResizeReflow(width_app_units, height_app_units);

   if (rc != NS_OK)
      return rc;
   
   if (presShell)
      presShell->FlushPendingNotifications(FlushType::Layout);

   return NS_OK;
}


#endif ADV_SW_PATCH_SIZE_TO_CONTENT