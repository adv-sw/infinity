/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_pipe.h

Description : IPC communications pipe definition.

License : Copyright (c) 2002 - 2022, Advance Software Limited.

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



#ifndef INF_PIPE_H
#define INF_PIPE_H 1

// For Parameters

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#endif

#include "inf_basic_types.h"
#include "inf_dialog_info.h"

typedef Infinity::uint8   Cmd_ID;
typedef Infinity::uint8   Cmd_Target;


// Tightly packed for reading directly from streams.
#pragma pack(push, 1)

typedef struct
{
   Cmd_ID     id;
   Cmd_Target target;
} Cmd_Header;


typedef struct
{
   Infinity::uint32 button;
   Infinity::uint32 flags;
} Button_State;

typedef struct
{
   Infinity::uint32 msg_id;
   Infinity::uint32 wparam;
   Infinity::uint32 lparam;  // uint64_t ?
} System_Message;



// Define IPC video frame. 32 bit handle even on 64 bit systems to ensure serializing consistency over IPC bridge.
// MS Windows HANDLEs are always 32 bit, even on 64 bit systems which enables interoperability between process types.

typedef struct {
   Infinity::uint32  surface_handle;
   float   timestamp;
} Display_Frame;


typedef struct {
   Infinity::uint32 surface_handle;
   RECT dest;
} Display_Overlay;



typedef struct
{
   float time;
   float observer_pos_ws[3];
   float item_pos_ws[3];
} Msg_Telemetry;


#pragma pack(pop)

typedef bool(*Event_CB)(const Cmd_Header &cmd);


#define INF_PIPE_ID "\\\\.\\pipe\\inf_app_"


class Pipe
{
public:
   Pipe();
   ~Pipe();

   bool Create(size_t uid);
   bool Connect();
   bool Ensure_Connection();

   void Close();

   bool Message_Send(Cmd_ID id, Cmd_Target t, void *data, size_t len, bool write_len);

   bool Available(size_t required);

   bool Read(void *dest, size_t buffer_len);
   bool Write(const void *buffer, size_t buffer_len);

   // Caller must delete [] returned string.
   char *ReadString();

   // Caller must delete [] returned string.
   wchar_t *ReadWString();

   size_t m_id;
   HANDLE m_handle;
   bool m_async;

#if defined WIN32 || defined WIN64 || defined _WIN64
   OVERLAPPED m_overlapped;
#endif


   bool m_server; // Identifies which end we are.
   bool m_live;

};

bool Msg_Parent(Cmd_ID id, Cmd_Target t, void *data, size_t len, bool write_len=false);
size_t Strings_Serialize(Infinity::uint8 *&dest, const char **str, Infinity::uint32 num_strings, size_t additional_required = 0);
void Parameters_Receive(Pipe *pipe, ::Parameters &parms);
void Parameters_Send(Pipe *pipe, Cmd_Target t, Cmd_ID cmd_id, ::Parameters *params);

bool Program_Processing(Event_CB);
void Present(Infinity::uint32 surface_handle, float time, Cmd_Target target);
void Overlay(Infinity::uint32 surface_handle, RECT &r, Cmd_Target t);

#endif // INF_PIPE_H
