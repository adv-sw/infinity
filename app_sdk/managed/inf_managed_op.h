/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity Managed App SDK

File        : inf_managed_op.h

Description : Definition of managed operations.

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


#ifndef INF_MANAGED_OP_H
#define INF_MANAGED_OP_H

// enum class allows us to have the same identifier in different enums.

enum class Zone_Op { GetScene = 1, FindSceneGraph, Loading };
enum class SceneGraph_Op { CreateNode = 1, CreateObject, GetRoot, FindMaterial, FindNode, GetAnimation };
enum class Material_Op { GetPart = 1, GetColour, SetColour };
enum class Material_Part_Op { GetInput = 1 };
enum class Mixer_Input_Op { SetScale = 1, SetOffset };
enum class Diagnostics_Op { Log = 1, Get };
enum class Sound_Manager_Op { Sample = 1, Get };
enum class Animation_Op { GetDuration = 1 };


// IMPORTANT: Ensure derived class parameters don't stomp base class methods.
enum class Attachment_Op { SetParameter = 1 }; 
enum class Object_Op { SetParameter = 1, SetGeometry, GetGeometry, Intersection, SetAnimationPlaybackOffset };
enum class Node_Op   { SetParameter = 1, SetPosition, GetPosition, Disconnect, SetVelocity, GetAttachment };


#endif // INF_MANAGED_OP_H