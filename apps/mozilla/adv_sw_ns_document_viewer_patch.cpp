/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Mozilla App

File        : adv_sw_ns_document_viewer_patch.cpp

Description : Replacement for Gecko SizeToContent.

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


/*

#ifndef ADV_SW_NS_DOCUMENT_VIEWER_PATCH_CPP

#define ADV_SW_NS_DOCUMENT_VIEWER_PATCH_CPP

#include "adv_sw_patch.h"
#include "_update_info.h"
#include <assert.h>

#include "nsIScrollableFrame.h"


#define _ADV_DEBUG_RESIZE 0


nsIScrollableFrame *_GetScrollInfo(mozilla::PresShell *presShell, mozilla::PresContext *presContext, mozilla::RenderingContext *rcx,
                             float &scrollbar_width, float &scrollbar_height)
{
   nsIScrollableFrame *scrollable = presShell->GetRootScrollFrameAsScrollable();

   if (scrollable)
   {
      nsMargin sb = scrollable->GetDesiredScrollbarSizes(presContext, rcx);
      
      scrollbar_width  = (float) presContext->AppUnitsToDevPixels(sb.LeftRight());
      scrollbar_height = (float) presContext->AppUnitsToDevPixels(sb.TopBottom());
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
         width = width * correction;
      else
         height = height / correction;
   }
}


// Use a binary partition search to figure out ideal width by brute force.
float _GetPreferedWidth(nsIDocument *doc, nsIPresShell *presShell, nsIDocShellTreeItem *docShellAsItem, nsIDocShellTreeOwner *treeOwner, nsIScrollableFrame *scrollable, const float resizing_height, const float pixels_max_width)
{
   // resizing_height must be at least enough height for a scroll bar, otherwise it won't be added.

   float width = pixels_max_width;
   float delta = pixels_max_width * 0.5f;

   while (delta > 10) // TODO: Tradeoff - lower value, more accurate, but takes longer to calculate.
   {
      nsresult rv = presShell->ResizeReflow((nscoord)width, (nscoord)resizing_height);

      if (rv != NS_OK)
         return -1.0f;

      doc->FlushPendingNotifications(Flush_Layout);

      nsMargin scrollbars = scrollable->GetActualScrollbarSizes();

      bool horizontal_scroll_bar = scrollbars.top != 0 || scrollbars.bottom != 0;

      if (horizontal_scroll_bar)
      {
         // If we need a horizontal scroll bar when at the maximum setting, there is nothing we can do about it,
         // so just return, acknowledging this to be the case.
         if (width == pixels_max_width)
            return width;

         width += delta;
      }
      else
         width -= delta;

      delta *= 0.5f;
   }

   return width;
}


// Use a binary partition search to figure out ideal height by brute force.
float _GetPreferedHeight(nsIDocument *doc, nsIPresShell *presShell, nsIDocShellTreeItem *docShellAsItem, nsIDocShellTreeOwner *treeOwner, nsIScrollableFrame *scrollable, const float pixels_max_height, const float prefered_width)
{
   float height = pixels_max_height;
   float delta = pixels_max_height * 0.5f;

   while (delta > 10) // TODO: Tradeoff - lower value, more accurate, but takes longer to calculate.
   {
      nsresult rv = presShell->ResizeReflow((nscoord)prefered_width, (nscoord)height);

      if (rv != NS_OK)
         return -1.0f;

      doc->FlushPendingNotifications(Flush_Layout);

      nsMargin scrollbars = scrollable->GetActualScrollbarSizes();

      bool vertical_scroll_bar = scrollbars.left != 0 || scrollbars.right != 0;

      if (vertical_scroll_bar)
      {
         // If we need a vertical scroll bar when at the maximum setting, there is nothing we can do about it,
         // so just return, acknowledging this to be the case.
         if (height == pixels_max_height)
            return height;

         height += delta;
      }
      else
         height -= delta;

      delta *= 0.5f;
   }

   return height;
}


NS_IMETHODIMP nsGlobalWindow::SizeToContentConstrained(PRUint32 pixels_max_width, PRUint32 pixels_max_height, float aspect_ratio)
{
   if (!mDocShell) {
      return NS_FAIL;
   }

   // The content viewer does a check to make sure that it's a content
   // viewer for a toplevel docshell.
   nsCOMPtr<nsIContentViewer> cv;
   mDocShell->GetContentViewer(getter_AddRefs(cv));
   if (!cv) {
      aError.Throw(NS_ERROR_FAILURE);
      return;
   }

   // Make sure the new size is following the CheckSecurityWidthAndHeight
   // rules.
   nsCOMPtr<nsIDocShellTreeOwner> treeOwner = GetTreeOwner();
   if (!treeOwner) {
      aError.Throw(NS_ERROR_FAILURE);
      return;
   }

   nsCOMPtr<nsIPresShell> presShell = GetDocShell()->GetPresShell();
   NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);


   // Flush out all content and style updates. We can't use a resize reflow
   // because it won't change some sizes that a style change reflow will.
   nsGlobalWindow* rootWindow =
      static_cast<nsGlobalWindow *>(GetPrivateRoot());
   if (rootWindow) {
      rootWindow->FlushPendingNotifications(Flush_Layout);
   }

   nsIFrame *root = presShell->GetRootFrame();

   if (!root)
      return NS_ERROR_NOT_INITIALIZED;

   root = presShell->GetRootFrame();
   NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);

   nsIViewManager* vm = presShell->GetViewManager();

   nsRefPtr<nsDeviceContext> dc;
   vm->GetDeviceContext((*getter_AddRefs(dc)));

   nsRefPtr<nsRenderingContext> rcx;
   dc->CreateRenderingContext(*getter_AddRefs(rcx));

   nsCOMPtr<nsPresContext> presContext;
   GetPresContext(getter_AddRefs(presContext));
   NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

   nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
   docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
   NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

   // Get scroll bar dimensions.
   float scrollbar_width, scrollbar_height;
   nsIScrollableFrame *scrollable = _GetScrollInfo(presShell, presContext, rcx, scrollbar_width, scrollbar_height);

   if (!scrollable)
      return NS_ERROR_NOT_INITIALIZED;

   float width = 0.0f;

#if 1
      // Perform binary search to figure out ideal width by brute force.
      width = _GetPreferedWidth(mDocument, presShell, docShellAsItem, treeOwner, scrollable, scrollbar_height, float(pixels_max_width));
#else
      // can't use this implementation because GetPrefWidth is unrealiable.
      
      if (mDocument)
         mDocument->FlushPendingNotifications(Flush_Layout);

      nscoord prefWidth = root->GetPrefWidth(rcx);
      width = (float)presContext->AppUnitsToDevPixels(nscoord(prefWidth));

      const float default_width = 1024.0f;

      // GECKO BUG WORKAROUND: GetPrefWidth sometimes fails and the failure generates a result of (960 internal units) (16 pixels).
      // We workaround this bug by selecting a sensible default.
      if (width < 20)
      {
         width = pixels_max_width < default_width ? pixels_max_width : default_width;
         width -= scrollbar_width; // In case we'll need a vertical scroll bar, ensure we've left room for it.
         width--;  // Subtract one to ensure a clean power of two default size in final resize.
      }
#endif

   // Use a binary partition search to figure out ideal height by brute force.
   // TODO: This does not currently work.
   float height = _GetPreferedHeight(mDocument, presShell, docShellAsItem, treeOwner, scrollable, float(pixels_max_height), width);

   if (height < 20.0f)
   {
      const float default_height = 1024.0f;

      height = pixels_max_height < default_height ? pixels_max_height : default_height;
      height -= scrollbar_height; // In case we'll need a horizontal scroll bar, ensure we've left room for it.
      height--;  // Subtract one to ensure a clean power of two default size in final resize.
   }

   bool hscroll = false, vscroll = false;

   // Figure out if we're going to need scroll bars and adjust size accordingly to compensate.
   if (PRUint32(width) >= pixels_max_width)
   {
      hscroll = true;
      width = float(pixels_max_width) - 1.0f;   // Compensate for +1 in SizeShellTo.
      height += scrollbar_height;
   }

   if (PRUint32(height) >= pixels_max_height)
   {
      vscroll = true;
      height = float(pixels_max_height) - 1.0f;   // Compensate for +1 in SizeShellTo.
      width += scrollbar_width;
   }

   // Adjust content dimensions to fit desired aspect ratio ...

   // An aspect ratio of <= 0 means size exactly to content (do not perform aspect adjust).

   if ((height > 1.0f) && (aspect_ratio > 0.0f))
   {
      _AdvAspectAdjust(hscroll, vscroll, aspect_ratio, width, height);

      // Truncation to max permissible resolution ... should not be required, but just in case for now.
      if (PRUint32(width) >= pixels_max_width)
         width = float(pixels_max_width) - 1.0f;    // Compensate for +1 in SizeShellTo.

      if (PRUint32(height) >= pixels_max_height)
         height = float(pixels_max_height) - 1.0f;  // Compensate for +1 in SizeShellTo.
   }

#if _ADV_DEBUG_RESIZE
   _AdvLogResizeInfo((nsIWidget*)mWindow, hscroll, vscroll, aspect_ratio, width, height);
#endif

   // +1 to compensate ensure unnecessary scroll bars do not appear as a result of floating point errors in size calculations.
   return treeOwner->SizeShellTo(docShellAsItem, PRInt32(width) + 1, PRInt32(height) + 1);

#endif

}

#endif // ADV_SW_NS_DOCUMENT_VIEWER_PATCH_CPP

*/