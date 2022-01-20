/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_interprocess_msg.h

Description : App<->Infinity inter-process communictions definition.

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


#ifndef INF_INTERPROCESS_MSG_H
#define INF_INTERPROCESS_MSG_H 1

// Inter-Process Communications (IPC)

// Kernel -> App
// TODO: Ensure these stay in sync with Parent_Message_ID
#define INFINITY_APP_CURSOR_POSITION  1
#define INFINITY_APP_PLAY             2
#define INFINITY_APP_PAUSE            3
#define INFINITY_APP_VOLUME           4
#define INFINITY_APP_VISIBLE          5
#define INFINITY_APP_OPEN             6
#define INFINITY_APP_BUTTON_STATE     7
#define INFINITY_APP_WHEEL_STATE      8
#define INFINITY_APP_EXPIRE_INSTANCE  9
#define INFINITY_APP_SYSTEM_MSG      10
#define INFINITY_APP_FOCUS           11
#define INFINITY_APP_CURSOR          12
#define INFINITY_APP_DIALOG_COMPLETE 13
#define INFINITY_APP_REDRAW          14
#define INFINITY_APP_COOKIE_QUERY    15
#define INFINITY_APP_COOKIE_ASSIGN   16
#define INFINITY_APP_THEME_CHANGE    17
#define INFINITY_APP_TERMINATE       18
#define INFINITY_APP_EXCLUSIVE       19
#define INFINITY_APP_AUDIO_STATUS    20
#define INFINITY_APP_AUDIO_REQUEST   21
#define INFINITY_APP_TELEMETRY       22


// App -> Kernel
#define INFINITY_DELIVER_FRAME     0
#define INFINITY_DISPLAY_OVERLAY   1
#define INFINITY_AUDIO_FORMAT      2
#define INFINITY_AUDIO_DATA        3
#define INFINITY_PLAYBACK_ERROR    4
#define INFINITY_COMPLETE          5
#define INFINITY_DIALOG_OPEN       6
#define INFINITY_APP_COOKIE_VALUE  7
#define INFINITY_APP_COOKIE_UPDATE 8
#define INFINITY_APP_EXIT          9
#define INFINITY_APP_CONNECTED     10
#define INFINITY_AUDIO_TIME        11
#define INFINITY_FORMAT_CHANGED    12
#define INFINITY_LOG               13
#define INFINITY_PROGRESS          14
#define INFINITY_REQUEST_EXCLUSIVE 15


const char *Parent_Message_ID(uint32_t msg_id);


#endif // INF_INTERPROCESS_MSG_H
