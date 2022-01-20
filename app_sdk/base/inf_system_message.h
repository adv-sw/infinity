/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_system_message.h

Description : System message definition.

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


#pragma once

#if defined WIN32 || defined WIN64 || defined _WIN64
#else
#include "X11/keysym.h"
uint32_t KeyState(size_t key);
#endif


#if defined _WIN32 || defined _WIN64

#include <windows.h>

#define KeyState(x) /*Win32*/::GetAsyncKeyState( x )

#define KEY_ESCAPE VK_ESCAPE
#define KEY_TAB VK_TAB

#define KEY_F1 VK_F1
#define KEY_F2 VK_F2
#define KEY_F3 VK_F3
#define KEY_F4 VK_F4
#define KEY_F5 VK_F5
#define KEY_F6 VK_F6
#define KEY_F7 VK_F7
#define KEY_F8 VK_F8
#define KEY_F9 VK_F9
#define KEY_F10 VK_F10
#define KEY_F11 VK_F11
#define KEY_F12 VK_F12

#define KEY_SPACE 0x20



// INF prefix to avoid clashes with other SDKS.
#define INF_KEY_UP    VK_UP
#define INF_KEY_DOWN  VK_DOWN
#define INF_KEY_LEFT  VK_LEFT
#define INF_KEY_RIGHT VK_RIGHT



#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#define GAMEPAD_BUTTON_A 0x1000
#define GAMEPAD_BUTTON_B 0x1001
#define GAMEPAD_BUTTON_X 0x1002
#define GAMEPAD_BUTTON_Y 0x1003
#define GAMEPAD_BUTTON_L1 0x1004
#define GAMEPAD_BUTTON_R1 0x1005
#define GAMEPAD_BUTTON_START 0x1006
#define TOUCH_DOUBLE_TAP 0x1100

#elif defined(VK_USE_PLATFORM_IOS_MVK)
// Use numeric keys instead of function keys.
// Use main keyboard plus/minus instead of keypad plus/minus
// Use Delete key instead of Escape key.
#define KEY_ESCAPE 0x33
#define KEY_F1 '1'
#define KEY_F2 '2'
#define KEY_F3 '3'
#define KEY_F4 '4'
#define KEY_W 'w'
#define KEY_A 'a'
#define KEY_S 's'
#define KEY_D 'd'
#define KEY_P 'p'
#define KEY_SPACE ' '
#define KEY_KPADD '+'
#define KEY_KPSUB '-'
#define KEY_B 'b'
#define KEY_F 'f'
#define KEY_L 'l'
#define KEY_N 'n'
#define KEY_O 'o'
#define KEY_T 't'

#elif defined(VK_USE_PLATFORM_MACOS_MVK)
// For compatibility with iOS UX and absent keypad on MacBook:
// - Use numeric keys instead of function keys
// - Use main keyboard plus/minus instead of keypad plus/minus
// - Use Delete key instead of Escape key
#define KEY_ESCAPE 0x33
#define KEY_F1 0x12
#define KEY_F2 0x13
#define KEY_F3 0x14
#define KEY_F4 0x15
#define KEY_W 0x0D
#define KEY_A 0x00
#define KEY_S 0x01
#define KEY_D 0x02
#define KEY_P 0x23
#define KEY_SPACE 0x31
#define KEY_KPADD 0x18
#define KEY_KPSUB 0x1B
#define KEY_B 0x0B
#define KEY_F 0x03
#define KEY_L 0x25
#define KEY_N 0x2D
#define KEY_O 0x1F
#define KEY_T 0x11

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <linux/input.h>
#elif defined(__linux__)
#define KEY_ESCAPE XK_Escape
#define KEY_CONTROL XK_Control_L
#define KEY_F1 XK_F1
#define KEY_F2 XK_F2
#define KEY_F3 XK_F3
#define KEY_F4 XK_F4
#define KEY_F5 XK_F5
#define KEY_F6 XK_F6
#define KEY_F7 XK_F7
#define KEY_F8 XK_F8
#define KEY_F9 XK_F9
#define KEY_F10 XK_F10
#define KEY_F11 XK_F11
#define KEY_F12 XK_F12

#define KEY_A XK_A
#define KEY_Z XK_Z

#define KEY_a XK_a
#define KEY_z XK_z

#define KEY_0 XK_0
#define KEY_9 XK_9



#define KEY_SPACE 0x41
#define KEY_KPADD 0x56
#define KEY_KPSUB 0x52

// INF prefix to avoid clashes with other SDKS.
#define INF_KEY_UP    XK_Up
#define INF_KEY_DOWN  XK_Down
#define INF_KEY_LEFT  XK_Left
#define INF_KEY_RIGHT XK_Right

#define KEY_TAB  XK_Tab

#define KEY_LBUTTON XK_Pointer_Button_Dflt
#define KEY_RBUTTON XK_Pointer_Button1
#define KEY_MBUTTON XK_Pointer_Button2


#define WM_KEYDOWN 1
#define WM_KEYUP  2
#define WM_SYSKEYDOWN 3


// TODO: Resolve these properly

#define VK_DELETE 13
#define VK_INSERT 14
#define VK_ESCAPE 15
#define VK_TAB    16

#define VK_CONTROL 0xf0000000

#define VK_RETURN  0xffff0001
#define VK_LWIN    0xffff0002
#define VK_RWIN    0xffff0003
#define VK_APPS    0xffff0004


#endif // INF_SYSTEM_MESSAGE_H
