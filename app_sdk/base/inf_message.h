/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_message.h

Description : IPC message definition

License : Copyright (c) 2002 - 2021, Advance Software Limited.

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

#ifndef INF_MESSAGE_H
#define INF_MESSAGE_H

#include "inf_math.h"

#define INF_MSG_OPTIMIZED 1

#include <list>
#include <mutex>

#if INF_MSG_OPTIMIZED
// Currently hardwired. 
// TODO: Support per class config for optimize/tuning if seems worth the effort.
#define INF_MESSAGE_QUEUE_CAPACITY 100
#endif


#if defined INF_SHARED_API_EXPORTS || defined KERNEL_EXPORTS
#include "inf_call.h"
#define INF_MSG_API INF_SHARED_API
#else
// External app
#define INF_MSG_API 
#endif


namespace Infinity
{
enum Clipboard_Command { Cut, Copy, Paste, Select_All, Select_None };

class PluginTexture;
typedef PluginTexture *  Message_Destination;

class Message
{
public:

   Message(uint16 _id, uint16 _count = 0);
   ~Message();

   uint16  id;
   uint16  count;
   uint8   *data;
   Message_Destination m_dest;
};


typedef std::list<Message *> Messages;

class Message_Queue
{
public:

#if INF_MSG_OPTIMIZED
   Message_Queue()
   {
      m_pop_slot_index = -1;
      m_push_slot_index = 0;
   }
#endif
   
   INF_MSG_API void Push(Message_Destination dest, Message *msg);
   INF_MSG_API Message *Pop();

   bool Lock(bool wait=true) 
   { 
      if (wait) 
      { 
         m_mutex.lock(); 
         return true; 
      }
      else
         return m_mutex.try_lock();
   }

   void Unlock() { m_mutex.unlock(); }

private:
   
   std::mutex m_mutex;

#if INF_MSG_OPTIMIZED
   Message *m_slots[INF_MESSAGE_QUEUE_CAPACITY];
   int32 m_pop_slot_index;
   int32 m_push_slot_index;
#else
   Messages  m_pending;
#endif

};


INF_MSG_API Message_Queue *Message_Queue_Create();
INF_MSG_API void Delete(Message_Queue *mq);

INF_MSG_API Message *Message_Allocate(uint16 id, uint16 data_size=0);

INF_MSG_API Message *CreateMessage(uint16 msg_id);
INF_MSG_API Message *CreateMessage(uint16 msg_id, void *param);
INF_MSG_API Message *CreateMessage(uint16 msg_id, uint32 param);
INF_MSG_API Message *CreateMessage(uint16 msg_id, void *param1, void *param2);
INF_MSG_API Message *CreateMessage(uint16 msg_id, float param);
INF_MSG_API Message *CreateMessage(uint16 msg_id, const Vector2 &param);
INF_MSG_API Message *CreateMessage(uint16 msg_id, const std::string &s);

INF_MSG_API void DeleteMessage(Message *msg);


#define INF_MESSAGE_TOKEN_VALUE            0x00

#define INF_MESSAGE_SYSTEM                 0x01

#define INF_MESSAGE_BUTTON_DOWN            0x02
#define INF_MESSAGE_BUTTON_UP              0x03
#define INF_MESSAGE_SCROLL_VERTICAL        0x04

#define INF_MESSAGE_IDENTIFIER_CHANGED     0x05
#define INF_MESSAGE_FILENAME_CHANGED       0x06

#define INF_MESSAGE_STOP                   0x07

//#define INF_MESSAGE_CURSOR_ENTER            0x08
#define INF_MESSAGE_FOCUS                  0x09
#define INF_MESSAGE_FOCUS_LOST             0x0A

#define INF_MESSAGE_OPEN                   0x0B
#define INF_MESSAGE_REFRESH                0x0C
#define INF_MESSAGE_GOTO_PREV              0x0D
#define INF_MESSAGE_GOTO_NEXT              0x0E
#define INF_MESSAGE_PRINT                  0x0F
#define INF_MESSAGE_PERCENTAGE_LOADED      0x10
#define INF_MESSAGE_CAN_NAVIGATE_FORWARDS  0x11
#define INF_MESSAGE_CAN_NAVIGATE_BACKWARDS 0x12
#define INF_MESSAGE_INVERT                 0x13
#define INF_MESSAGE_FLAGS                  0x14
#define INF_MESSAGE_EDIT_REVERT_ON_DEFOCUS 0x15
#define INF_MESSAGE_SET_CURSOR             0x16
#define INF_MESSAGE_ZAP_DESKTOP_CURSOR     0x17
#define INF_MESSAGE_EXCLUSIVE_REQUEST      0x18
#define INF_MESSAGE_SET_ASPECT             0x19

#define INF_MESSAGE_POPUP_CREATE           0x20
#define INF_MESSAGE_POPUP_OPEN             0x21
#define INF_MESSAGE_POPUP_CLOSE            0x22
#define INF_MESSAGE_ADD_HISTORY            0x23
#define INF_MESSAGE_SET_PLAYBACK_MODE      0x24

#define INF_MESSAGE_SET_DESIRED_WIDTH      0x25
#define INF_MESSAGE_SET_DESIRED_HEIGHT     0x26
#define INF_MESSAGE_SET_DIMENSIONS_2D      0x27

#define INF_MESSAGE_SOURCE_ACQUIRE         0x28

#define INF_MESSAGE_CHANGE_TYPE            0x29

}

#endif // INF_MESSAGE_H
