#if 0

#ifdef INF_PLUGIN
#define INFINITY_PLUGIN_EXPORTS 1
#endif

#include <windows.h>
#include <inf_plugin_texture.h>
#include <inf_config.h>

#ifndef INFINITY_PLUGIN_SHARED_EXPORTS
#pragma comment(lib, INF_BASE_LIB)
#undef INF_SHARED_API
#define INF_SHARED_API
#endif

using namespace Infinity;

static unsigned int __mode;


static HINSTANCE __module_handle = 0;

BOOL APIENTRY DllMain(HANDLE hModule, uint32 reason_for_call, LPVOID lpReserved)
{
   switch (reason_for_call)
   {
   case DLL_PROCESS_ATTACH:
      __module_handle = (HINSTANCE)hModule;
      break;

   case DLL_THREAD_ATTACH:
      break;
   }

   return TRUE;
}


HINSTANCE GetCurrentModuleHandle()
{
   return __module_handle;
}


#include "nsXULAppAPI.h"
#include "winEmbed.h"

#include "WebBrowserChrome.h"

extern XRE_InitEmbedding2Type XRE_InitEmbedding2;
extern XRE_TermEmbeddingType XRE_TermEmbedding;

nsresult OpenWebPage(const char * url);

#include <inf_util.h>

INF_PLUGIN_API_C void PluginTexture_TerminateClass()
{
   XRE_TermEmbedding();

   // Go through a garbage collect cycle to ensure we clean up correctly.
   bool abort = false;
   Infinity::ProcessNativeWindowMessages(nullptr, &abort, 0);
}


class Mozilla_Webpage : public Infinity::PluginTexture
{
public:
   Mozilla_Webpage(void *_pdata) : Infinity::PluginTexture(_pdata) {}
   ~Mozilla_Webpage()
   {
      if (m_chrome)
         WebBrowserChromeUI::Destroy(m_chrome);
   }

   virtual Infinity::uint32 Async_Update();
   Infinity::String m_pending_location;
   nsIWebBrowserChrome *m_chrome;
};


INF_PLUGIN_API_C bool PluginTexture_DeleteInstance(void *inst)
{
   Mozilla_Webpage *w = (Mozilla_Webpage *)inst;
   delete w;
   return true;
}


INF_PLUGIN_API_C Infinity::PluginTexture *PluginTexture_CreateInstance(void *ptc)
{
   Mozilla_Webpage *w = new Mozilla_Webpage(nullptr);
   w->m_pending_location.Assign("file:///D:/test/scrollbar.html");
   return w;
}


INF_PLUGIN_API_C bool PluginTexture_Class_Processing(void *user)
{
   bool abort = false;
   Infinity::ProcessNativeWindowMessages(nullptr, &abort, 0);

   return true;
}

ATOM             MyRegisterClass(HINSTANCE hInstance);
int _Mozilla_Init(HINSTANCE hinst, bool init_xre);
int _Mozilla_Init2();

INF_PLUGIN_API_C bool PluginTexture_InitializeClass(Infinity::uint32 version, void *_pdata)
{
   HINSTANCE hinst = GetCurrentModuleHandle();
   MyRegisterClass(hinst);
   _Mozilla_Init(hinst, true);
   _Mozilla_Init2();
   return true;
}

nsresult OpenWebPage(nsCOMPtr<nsIWebBrowserChrome> chrome, const char *url);

Infinity::uint32 Mozilla_Webpage::Async_Update()
{
   if (!m_pending_location.IsEmpty())
   {
      OpenWebPage(m_chrome, m_pending_location);
      m_pending_location.Clear();
   }

   return 0;
}


#endif