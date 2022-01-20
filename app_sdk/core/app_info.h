/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : app_info.h

Description : Program base class definition.

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


#ifndef INF_APP_INFO_H
#define INF_APP_INFO_H

class Pipe;

class Program
{
public:
   Program()
   {
      m_device_id = 0;
      m_pipe = 0;
      m_data_produced_handle = 0;
      m_trigger_quit = false;
      m_ipc_ready = false;
   }

   ~Program()
   {
   }

   bool CmdLine_Parameter_Consume(const char *token, const char *value);

   unsigned int m_device_id;
   bool m_auto_loop;
   bool m_play_trigger;
   bool m_trigger_quit;
   uint32_t m_preferred_language;
   float m_volume;

   Pipe * m_pipe;
   bool   m_ipc_ready;

   void * m_data_produced_handle;
   
   // For inter-process communications
   void * m_parent_hwnd;  // Message window of the process that invoked us.
   size_t m_parent_texture; // Address of parent texture that invoked us in its address space.

}; 

#endif // INF_APP_INFO_H
