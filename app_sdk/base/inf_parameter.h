/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_parameter.h

Description : Generic parameter passing.

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


#ifndef INF_PARAMETER_H
#define INF_PARAMETER_H

#include "inf_basic_types.h"
#include "../managed/inf_managed_types.h"

#include <string>

namespace Infinity
{

#define INF_PARAM_SET(type_enum, type)\
   if ( m_param_type != (type_enum) ) {\
      if (m_value) { delete m_value; m_value = nullptr; }\
      m_param_type = (type_enum); } \
     if (m_value) * ( type * ) m_value = v; \
     else m_value = new type (v);

#define INF_RETURN_PARAM(type_enum, type )\
   if ( m_param_type != type_enum ) return nullptr ;\
   return INF_CAST_PTR( const type , m_value );

#define INF_RETURN_PARAM_DEREF(type_enum, type )\
   if ( m_param_type != type_enum ) return nullptr ;\
   return * INF_CAST_PTR( const type, m_value );

      
class Parameter
{
public:

   Parameter() { m_value = nullptr; m_param_type = Type_Unknown; }
   ~Parameter() { if (m_value && (m_param_type != Type_Ptr)) delete m_value; }

   void Clear() { if (m_value && (m_param_type != Type_Ptr)) delete m_value;  m_value = nullptr; m_param_type = Type_Unknown; }

   void Set(const Vector2 &v) { INF_PARAM_SET(Type_Vector2, Vector2) }
   void Set(const Vector3 &v) { INF_PARAM_SET(Type_Vector3, Vector3) }
   void Set(const Vector4 &v) { INF_PARAM_SET(Type_Vector4, Vector4) }

   void Set(bool v) { INF_PARAM_SET(Type_Bool, bool) }
   void Set(uint8 v) { INF_PARAM_SET(Type_UInt, size_t) }
   void Set(uint16 v) { INF_PARAM_SET(Type_UInt, size_t) }
   void Set(uint32 v) { INF_PARAM_SET(Type_UInt, size_t) }
   void Set(size_t v) { INF_PARAM_SET(Type_UInt, size_t) }
   void Set(float v) { INF_PARAM_SET(Type_Scalar, float) }
   void Set(const Char *v) { INF_PARAM_SET(Type_String, const Char*) }

   // WARNING: We do not currently reference count or duplicate strings. 
   // Ensure whatever you're passing doesn't go out of scope until Parameter is disposed.
   // Caller is responsible for disposing of string when you're done with Parameter.
   void Set(Char *v) { INF_PARAM_SET(Type_String, Char*) }
   void Set(void *v) { m_value = v; m_param_type = Type_Ptr;  }


   // Extract ...

   const Char    *Get_String()  const { INF_RETURN_PARAM_DEREF(Type_String, Char*);  }
   const Vector2 *Get_Vector2() const { INF_RETURN_PARAM(Type_Vector2, Vector2) }
   const Vector3 *Get_Vector3() const { INF_RETURN_PARAM(Type_Vector3, Vector3) }
   const Vector4 *Get_Vector4() const { INF_RETURN_PARAM(Type_Vector4, Vector4) }
   const float   *Get_Scalar()  const { INF_RETURN_PARAM(Type_Scalar, float) }
   const bool    *Get_Bool()    const { INF_RETURN_PARAM(Type_Bool, bool) }
   const size_t  *Get_UInt()    const { INF_RETURN_PARAM(Type_UInt, size_t) }
   void          *Get_Ptr()     const { return (m_param_type == Type_Ptr) ? m_value : nullptr; }

   void *m_value;
   Type  m_param_type;
};

}

#endif // INF_PARAMETER_H
