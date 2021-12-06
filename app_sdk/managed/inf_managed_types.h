/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity Managed App SDK

File        : inf_managed_types.h

Description : Definition of managed types

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


#ifndef INF_MANAGED_TYPES_H
#define INF_MANAGED_TYPES_H

// Sorted in approx order of access frequency to optimize cache hit likelihood.
// TODO: Run analysis to determine real-world frequency to optimize ordering for v2.
// TODO: Remove abstracts & those without managed reflection to keep list minimal & relevant.
// TODO: Every time this is changed, *every* managed script needs a recompile, so do so infrequently & bump version when this occurs.


namespace Infinity
{
   const unsigned long Type_Version = 1;

   enum Type {

   Type_Unknown = 0,

   Type_Node,
   
   Type_SceneGraph,
 
   Type_Object,
   Type_Mesh,
   Type_PrimitiveGroup,
   Type_Light,

   Type_Camera,
 
   Type_Texture,

   Type_Material,
   Type_Material_Part,
   Type_Mixer_Input,
   Type_Material_Override,
  
   Type_Sound_Manager,
   Type_Sound,

   Type_Animation,
   Type_Animation_Channel,

   Type_Emitter,
   Type_Instance,

   Type_Zone,

   Type_Vector4,
   Type_Transform,
   
   Type_Ray,
   Type_Skin,

   Type_CheckBox,
   Type_ComboBox,

   Type_Kernel,
   Type_Action,

   Type_Cursor,
   Type_Diagnostics,

   Type_Window,

   Type_Menu,
   Type_Menu_Component,

   Type_Font,

   Type_Attachment,
   Type_Behaviour,

   Type_Listener,
   
   Type_Parser,
   Type_Physics,
   Type_Protected,

   Type_Scalar,

   Type_String,

   Type_Timer,

   Type_Bool,
   Type_Vector2,
   Type_Vector3,
   Type_Data,
   Type_UInt,
   Type_Ptr,

   Type_Max
};


};

#endif // INF_MANAGED_TYPES_H
