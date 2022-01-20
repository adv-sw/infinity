/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Mozilla App

File        : adv_sw_patch_prompt.cpp

Description : 

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


#ifndef ADV_SW_PATCH_PROMPT_CPP
#define ADV_SW_PATCH_PROMPT_CPP

#include "_update_info.h"

#include "adv_sw_patch.h"

#include "nsIEmbeddingSiteWindow.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebBrowserChrome.h"

// For GetNativeWindowRenderTarget
#include "adv_sw_base_window.cpp"

XPCOM_API(HWND) hwndForDOMWindow(mozIDOMWindowProxy *window);

HWND GetHWNDForDOMWindow(nsGlobalWindowOuter *aWindow)
{
	//nsCOMPtr<nsPIDOMWindowOuter> pidomwindow = aWindow ? aWindow->AsOuter() : nullptr;
	
   nsCOMPtr<nsIBaseWindow> ppBaseWindow = do_QueryInterface(aWindow->GetDocShell());
  
   if (!ppBaseWindow)
      return 0;

   nsCOMPtr<nsIWidget> ppWidget;
   ppBaseWindow->GetMainWidget(getter_AddRefs(ppWidget));

   auto hwnd = (HWND) (ppWidget->GetNativeData(NS_NATIVE_WIDGET));

   return (HWND) GetNativeWindowRenderTarget(hwnd);
}


#include "..\..\..\..\mozilla\ipc\chromium\src\base\message_loop.h"
bool __cdecl Infinity_App_Processing();
extern bool __inf_modal_dialog;

nsresult Adv_ModalDialog(nsGlobalWindowOuter *gw, const nsAutoString &title, 
                         const nsAutoString &message, nsAString *editable, bool *ok)
{
   HWND hWnd = GetHWNDForDOMWindow(gw);

   UpdateInfo *info = _GetUpdateInfo(hWnd, true);  // Wait for access.

   if (!info)
      return NS_ERROR_FAILURE;

   // Title ...
   info->DialogParam_Set("title", (void*) title.get());
     
   // Message ...
   info->DialogParam_Set("i_1", (void*) message.get());  // Information  1
 
   if (editable)
   {
      // i_t_ = input text control n
      info->DialogParam_Set("i_t_1", (void*) editable->Data());
   }

   info->OpenModalDialog();
   _ReleaseUpdateInfo(info);

   // Modal dialogs must spin here, waiting for processing to complete before continuing.
   bool done = false;

   __inf_modal_dialog = true;
   MessageLoop::current()->Run();
   __inf_modal_dialog = false;

   // Dialog complete. Extract return status.

   info = _GetUpdateInfo(hWnd, true);

   if (info)
   {
      PRInt32 button = info->GetDialogExitCode();

      if (ok)
         *ok = (bool) button;
     
      if (editable)
      {
		   void *value = nullptr;
      
         // t_ = text control n.
         if (info->DialogParam_Get("i_t_1", value))
            editable->Assign((wchar_t*) value);  // One of the rare places Infinity currently passes wchar_t
      }

      info->ResetDialog();

      _ReleaseUpdateInfo(info);
   }


   return NS_OK;
}


nsresult Adv_RequestExclusive(nsGlobalWindowOuter *gw, bool enable)
{
   HWND hWnd = GetHWNDForDOMWindow(gw);

   UpdateInfo *info = _GetUpdateInfo(hWnd, true);  // Wait for access.

   if (!info)
      return NS_ERROR_FAILURE;

   info->m_exclusive = enable;
   info->m_exclusive_trigger = true;

   _ReleaseUpdateInfo(info);

   return NS_OK;
}


#endif // ADV_SW_PATCH_PROMPT_CPP



#if 0

// Prev implementation includes additional functionality - here for future reference.

nsresult Adv_ModalDialog(nsIDOMWindow *parent, nsCOMPtr<nsIWindowWatcher> mWatcher, nsIDialogParamBlock *block)
{
   HWND hWnd = GetHWNDForDOMWindow(parent, mWatcher);

   UpdateInfo *info = _GetUpdateInfo(hWnd, true);  // Wait for access.

   if (!info)
      return NS_ERROR_FAILURE;

   // Extract dialog parameters from nsIDialogParamBlock ...
   PRUnichar *string = nullptr;
   char id[10];   

   PRInt32 num_buttons = 0;
   block->GetInt(nsPromptService::eNumberButtons, &num_buttons);
   
   // Button text ...
   for (PRInt32 b=0; b < num_buttons; b++)
   {
      string = nullptr;
      block->GetString(nsPromptService::eButton0Text+b, &string);

      if (string)
      {
         // Button n information. 
         // TODO: Put this on the button, or tooltip. It is ignored & hardwired for now.
         sprintf(id, "i_b_%d", b);   
         info->DialogParam_Set(id, (void*) string);
      }
   }

   // Message ...
   string=nullptr;
   block->GetString(nsPromptService::eMsg, &string);
   
   if (string)
      info->DialogParam_Set("i_1", (void*) string);  // Information  1
   
   // Title ...
   string=nullptr;
   block->GetString(nsPromptService::eDialogTitle, &string);
   
   if (string)
      info->DialogParam_Set("title", (void*) string);
  
   // Edit fields ...
   PRInt32 num_edit_fields = 0;
   block->GetInt(nsPromptService::eNumberEditfields, &num_edit_fields);

   // Grab icon class and figure out which text fields need obscuring.
   string=nullptr;
   block->GetString(nsPromptService::eIconClass, &string);

   int obscured_text_entry_field = -1; // none.

   if (string)
   {
      info->DialogParam_Set("g_1", (void*) string); // Graphic 1 is reserved for icons.

      NS_ConvertASCIItoUTF16 styleClass(kAuthenticationIconClass);

      if (!wcscmp(string, styleClass.get()))
         obscured_text_entry_field = num_edit_fields-1;
   }

   for (PRInt32 e=0; e < num_edit_fields; e++)
   {
      string=nullptr;
      block->GetString(nsPromptService::eEditfield1Msg+e, &string);
      sprintf(id, "i_t_%d", e+1);  // i_t_ = text control n information
      
      if (string)
         info->DialogParam_Set(id, (void*) string);

      string=nullptr;
      block->GetString(nsPromptService::eEditfield1Value+e, &string);
      
      sprintf(id, "t_%d", e+1);  // t_ = text control n.
      
      if (string)
         info->DialogParam_Set(id, (void*) string);

      if (e==obscured_text_entry_field)
      {
         sprintf(id, "o_t_%d", e+1);  // o = obscure control n.
         info->DialogParam_Set(id, 1);
      }
   }

   // Checkbox
   string=nullptr;
   block->GetString(nsPromptService::eCheckboxMsg, &string);
   
   if (string)
      info->DialogParam_Set("i_x_1", (void*) string);  // Checkbox n information (c is reserved for combos)

   PRInt32 check_value = 0;
   
   if (block->GetInt(nsPromptService::eCheckboxState, &check_value)==NS_OK)
      info->DialogParam_Set("x_1", (float)check_value);  // x_ = initial value of checkbox n.

   info->OpenModalDialog();
   _ReleaseUpdateInfo(info);

   // Modal dialogs must wait for processing to complete before continuing.
   bool done = false;
   
   while (!done)
   {
       info = _GetUpdateInfo(hWnd, false);

       if (info)
       {      
          done = info->IsDialogDataEntryComplete();
          _ReleaseUpdateInfo(info);
       }
       
       // Perform some thread message processing.
       _ProcessNativeWindowMessages(hWnd);

       // LOAD_BALANCE - yield.
       /*Win32*/::SwitchToThread();
   }

   // Dialog complete. Extract return status.

   info = _GetUpdateInfo(hWnd, true);

   if (info)
   {
      PRInt32 button = info->GetDialogExitCode();
      block->SetInt(nsPromptService::eButtonPressed, button);

      float scalar;
      
      if (info->DialogParam_Get("x_1", scalar))
         block->SetInt(nsPromptService::eCheckboxState, scalar);

      for (PRInt32 e=0; e < num_edit_fields; e++)
      {
         string=nullptr;

         sprintf(id, "t_%d", e+1);  // t_ = text control n.
         
		 void *value = nullptr;

         if (info->DialogParam_Get(id, value))
            block->SetString(nsPromptService::eEditfield1Value+e, (PRUnichar *) value);
      }

      info->ResetDialog();
      _ReleaseUpdateInfo(info);
   }

   return NS_OK;
}
 
#endif