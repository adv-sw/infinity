/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_message.cpp

Description : IPC message implementation.

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


#define INF_ASSERT_H

#include <math.h>

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#endif

#include "inf_basic_types.h"
#include "inf_math.h"
#include "inf_system_message.h"
#include "inf_message.h"
#include "..\core\inf_interprocess_msg.h"
#include <string>


#define MSG_ALLOC_DIAGNOSTICS 0

using namespace Infinity;


Message_Queue* Infinity::Message_Queue_Create()
{
   return new Message_Queue();
}


void Infinity::Delete(Message_Queue* mq)
{
   if (mq)
   {
      // Zap any unprocessed messages ...
      for (;;)
      {
         Message* msg = mq->Pop();

         if (msg)
            DeleteMessage(msg);
         else
            break;
      }

      delete mq;
   }
}


Message* Infinity::Message_Allocate(uint16 id, uint16 data_size)
{
   return new Message(id, data_size);
}


Message* Infinity::CreateMessage(uint16 id, const std::string& str)
{
   size_t len = str.length();

   if (len < 65534)
   {
      Message* msg = new Message(id, uint16(len + 1));
      memcpy(msg->data, str.c_str(), len);
      ((uint8*)msg->data)[len] = '\0'; // Add a null terminator.
      return msg;
   }
   else
      return nullptr;
}


#if MSG_ALLOC_DIAGNOSTICS
std::list<Message*> __current;
#endif

Message::Message(uint16 _id, uint16 _count)
{
   id = _id;
   count = _count;
   data = (count > 0 ? new uint8[count] : nullptr);

#if MSG_ALLOC_DIAGNOSTICS
   __current.push_back(this);
#endif
}


Message::~Message()
{
   if (count > 0)
      delete[] data;
}


void Infinity::DeleteMessage(Message* msg)
{
   assert(msg);

   if (msg)
   {
#if MSG_ALLOC_DIAGNOSTICS
      bool found = false;
      for (auto i = __current.begin(); i != __current.end(); ++i)
      {
         if (*i == msg)
         {
            __current.erase(i);
            found = true;
            break;
         }
      }

      INF_ASSERT(found);
#endif

      delete msg;
   }
}


#if INF_MSG_OPTIMIZED

static bool __message_queue_full_warning_issued = false;

void Message_Queue::Push(Message_Destination dest, Message* msg)
{
   msg->m_dest = dest;

   bool full = (((m_push_slot_index + 1) % INF_MESSAGE_QUEUE_CAPACITY) == m_pop_slot_index);

   if (full)
   {
      if (!__message_queue_full_warning_issued)
      {
#if INF_MQ_DIAGNOSTICS
         Infinity::String s;
         s.Printf("mq[%p] : full at %d. attempted msg_id: %d", this, m_pop_slot_index, msg->id);
         Infinity::Log(s);
#endif
         __message_queue_full_warning_issued = true;

#if defined _WIN32 || defined _WIN64
         //::Beep(2000, 100);
#endif
      }

      // Reject the message. circular buffer full, can't store.
      // TODO: Could dynamically reallocate a larger buffer to prevent future stalling.
      // however, a full buffer generally reflects message production occuring faster
      // than consumption, so there's a root load balancing issue that needs to be addressed.
      return;
   }

#if INF_MQ_DIAGNOSTICS
   Infinity::String s;
   s.Printf("mq[%p] : push at %d", this, m_push_slot_index);
   Infinity::Log(s);
#endif

   m_slots[m_push_slot_index++] = msg;

   if (m_pop_slot_index < 0)
      m_pop_slot_index = 0; // Content available - start standard circular traverse.

   if (m_push_slot_index == INF_MESSAGE_QUEUE_CAPACITY)
      m_push_slot_index = 0;
}


Message* Message_Queue::Pop()
{
   if (m_pop_slot_index < 0)
      return nullptr;

   if (m_pop_slot_index != m_push_slot_index)
   {
#if INF_MQ_DIAGNOSTICS
      Infinity::String s;
      s.Printf("mq[%p] : pop at %d", this, m_pop_slot_index);
      Infinity::Log(s);
#endif
      Message* msg = m_slots[m_pop_slot_index++];

      if (m_pop_slot_index == INF_MESSAGE_QUEUE_CAPACITY)
         m_pop_slot_index = 0;

      return msg;
   }

   return nullptr;
}

#else

void Message_Queue::Push(Message_Destination dest, Message* msg)
{
   msg->m_dest = dest;

   if (Lock(1))
   {
      m_pending.push_back(msg);
      Unlock();
   }
}


Message* Message_Queue::Pop()
{
   Message* msg = nullptr;

   if (Lock(3, false))
   {
      if (!m_pending.empty())
      {
         Messages::iterator i = m_pending.begin();
         msg = *i;
         m_pending.erase(i);
      }

      Unlock();
   }

   return msg;
}


/*
bool Message_Queue::Empty()
{
bool empty = true;

if (Lock(2))
{
empty = m_pending.empty();
Unlock();
}

return empty;
}*/



#endif // INF_MSG_OPTIMIZED



Message* Infinity::CreateMessage(uint16 msg_id)
{
   Message* msg = Message_Allocate(msg_id, 0);
   return msg;
}


Message* Infinity::CreateMessage(uint16 msg_id, void* param)
{
   Message* msg = Message_Allocate(msg_id, 0);

   void** ptr = INF_CAST_PTR(void*, &msg->data);
   *ptr = param;

   return msg;
}


Message* Infinity::CreateMessage(uint16 msg_id, uint32 param)
{
   Message* msg = Message_Allocate(msg_id, 0);

   uint32* ptr = INF_CAST_PTR(uint32, &msg->data);
   *ptr = param;

   return msg;
}


Message* Infinity::CreateMessage(uint16 msg_id, float param1)
{
   Message* msg = Message_Allocate(msg_id, 0);

   float* ptr = INF_CAST_PTR(float, &msg->data);
   *ptr = param1;

   return msg;
}


Message* Infinity::CreateMessage(uint16 msg_id, void* param1, void* param2)
{
   Message* msg = Message_Allocate(msg_id, sizeof(void*) * 2);

   void** ptr = (void**)msg->data;
   *(ptr++) = param1;
   *ptr = param2;

   return msg;
}


Message* Infinity::CreateMessage(uint16 msg_id, const Vector2& v)
{
   Message* msg = Message_Allocate(msg_id, sizeof(Vector2));

   Vector2* ptr = INF_CAST_PTR(Vector2, msg->data);
   *ptr = v;

   return msg;
}


// TODO: Ensure these stay in sync with their definitions in inf_interprocess_msg.h
const char* Parent_Message_ID(uint32_t msg_id)
{
   static const char* ids[] = {
   "null",
   "INFINITY_APP_CURSOR_POSITION",
   "INFINITY_APP_PLAY",
   "INFINITY_APP_PAUSE",
   "INFINITY_APP_VOLUME",
   "INFINITY_APP_VISIBLE",
   "INFINITY_APP_OPEN",
   "INFINITY_APP_BUTTON_STATE",
   "INFINITY_APP_WHEEL_STATE",
   "INFINITY_APP_EXPIRE_INSTANCE",
   "INFINITY_APP_SYSTEM_MSG",
   "INFINITY_APP_FOCUS",
   "INFINITY_APP_CURSOR",
   "INFINITY_APP_DIALOG_COMPLETE",
   "INFINITY_APP_REDRAW",
   "INFINITY_APP_COOKIE_QUERY",
   "INFINITY_APP_COOKIE_ASSIGN",
   "INFINITY_APP_THEME_CHANGE",
   "INFINITY_AUDIO_STATUS",
   "INFINITY_AUDIO_REQUEST",
   "INFINITY_TELEMETRY",
   "INFINITY_APP_TERMINATE"
   };

   if (msg_id > INFINITY_APP_TERMINATE)
      return "?";
   else
      return ids[msg_id];
}

