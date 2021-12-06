#ifndef __MOZ_BASE_WIDGET_H
#define __MOZ_BASE_WIDGET_H

#include "nsIBaseWindow.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowser.h"
#include "nsIWidget.h"
 
inline void GetBaseWidget(nsIWebBrowserChrome *chrome, nsIWidget **widget)
{
   if (chrome)
   {
      nsCOMPtr<nsIWebBrowser> wb;
      chrome->GetWebBrowser(getter_AddRefs(wb));
      
      nsCOMPtr<nsIBaseWindow> webBrowserAsWin = do_QueryInterface(wb);

      if (webBrowserAsWin)
         webBrowserAsWin->GetMainWidget(widget);
   }
   else
		*widget = nullptr;
}

#endif // __MOZ_BASE_WIDGET_H