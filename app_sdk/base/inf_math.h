/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity App SDK

File        : inf_math.h

Description : Definition of basic math types.

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


#ifndef INF_MATH_H
#define INF_MATH_H

#include <assert.h>

// Basic types ...

#include "inf_basic_types.h"

namespace Infinity
{
typedef uint8 tristate;
const tristate incomplete = 2;
typedef char       Char;
}



#if defined _WIN32 || defined _WIN64

#ifndef INF_DIRECTX_MATH

// For now to compile on xp / vs2010
#if _MSC_VER <= 1600
#define INF_DIRECTX_MATH 0
#else
#define INF_DIRECTX_MATH 1
#endif

#endif

#else


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#define INF_DIRECTX_MATH 0

typedef Infinity::uint32 DWORD;
typedef Infinity::uint8  BYTE;
typedef wchar_t WCHAR;

#undef HRESULT
#undef S_FALSE
#undef S_OK
typedef Infinity::uint32 HRESULT;
typedef struct _RECT { Infinity::uint32 left, top, bottom, right; } RECT;
typedef Infinity::uint32 LARGE_INTEGER;    // TODO: FIXME.
typedef Infinity::uint32 D3DRENDERSTATETYPE;

const HRESULT S_FALSE = 0;
const HRESULT S_OK = 1;
typedef void* HMODULE;
typedef void* HINSTANCE;
typedef void* HANDLE;
typedef void* HDC;

typedef struct
{
   Infinity::uint32 cx;
   Infinity::uint32 cy;
} SIZE;

typedef struct
{
   Infinity::uint32 x;
   Infinity::uint32 y;
} POINT;

#endif



#ifndef FLT_MAX
#define FLT_MAX         3.402823466e+38F
#endif

const float INF_PI = 3.14159265358979323846f; // TODO: check this is accurate.

#if INF_DIRECTX_MATH

#include <DirectXMath.h>

typedef unsigned __int64 uint64;
typedef __int64          int64;

namespace Infinity
{

   typedef DirectX::XMFLOAT2          Vector2;
   typedef DirectX::XMFLOAT3          Vector3;
   typedef DirectX::XMFLOAT4          Vector4;
   typedef DirectX::XMMATRIX         Matrix44;

   inline bool Equal(const Infinity::Vector2 &a, const Infinity::Vector2 &b)
   {
      auto vector1 = DirectX::XMLoadFloat2(&a);
      auto vector2 = DirectX::XMLoadFloat2(&b);
      return DirectX::XMVector2Equal(vector1, vector2);
   }

   inline bool Equal(const Infinity::Vector3 &a, const Infinity::Vector3 &b)
   {
      auto vector1 = DirectX::XMLoadFloat3(&a);
      auto vector2 = DirectX::XMLoadFloat3(&b);
      return DirectX::XMVector3Equal(vector1, vector2);
   }

   inline bool Equal(const Infinity::Vector4 &a, const Infinity::Vector4 &b)
   {
      auto vector1 = DirectX::XMLoadFloat4(&a);
      auto vector2 = DirectX::XMLoadFloat4(&b);
      return DirectX::XMVector4Equal(vector1, vector2);
   }

   inline void Scale(Infinity::Vector2 &v, float scale)
   {
      auto src_v4 = DirectX::XMLoadFloat2(&v);
      auto src_v4_s = DirectX::XMVectorScale(src_v4, scale);
      XMStoreFloat2(&v, src_v4_s);
   }

   inline void Scale(Infinity::Vector3 &v, float scale)
   {
      auto src_v4 = DirectX::XMLoadFloat3(&v);
      auto src_v4_s = DirectX::XMVectorScale(src_v4, scale);
      XMStoreFloat3(&v, src_v4_s);
   }

   inline void Scale(Infinity::Vector4 &v, float scale)
   {
      // TODO[OPT]: When working, collapse this as v4 is native type.
      auto src_v4 = DirectX::XMLoadFloat4(&v);
      auto src_v4_s = DirectX::XMVectorScale(src_v4, scale);
      XMStoreFloat4(&v, src_v4_s);
   }

   inline void Scale(Infinity::Vector2 &dest, const Infinity::Vector2 &src, float scale)
   {
      auto src_v4 = DirectX::XMLoadFloat2(&src);
      auto src_v4_s = DirectX::XMVectorScale(src_v4, scale);
      XMStoreFloat2(&dest, src_v4_s);
   }

   inline void Scale(Infinity::Vector3 &dest, const Infinity::Vector3 &src, const Infinity::Vector3 &scale)
   {
      auto src_a4 = DirectX::XMLoadFloat3(&src);
      auto src_b4 = DirectX::XMLoadFloat3(&scale);
      auto src_v4_s = DirectX::XMVectorMultiply(src_a4, src_b4);
      XMStoreFloat3(&dest, src_v4_s);
   }

   inline void Scale(Infinity::Vector3 &dest, const Infinity::Vector3 &src, float scale)
   {
      auto src_v4 = DirectX::XMLoadFloat3(&src);
      auto src_v4_s = DirectX::XMVectorScale(src_v4, scale);
      XMStoreFloat3(&dest, src_v4_s);
   }

   inline void Scale(Infinity::Vector4 &dest, const Infinity::Vector4 &src, float scale)
   {
      // TODO[OPT]: When working, collapse this as v4 is native type.
      auto src_v4 = DirectX::XMLoadFloat4(&src);
      auto src_v4_s = DirectX::XMVectorScale(src_v4, scale);
      XMStoreFloat4(&dest, src_v4_s);
   }

   inline void Add(Infinity::Vector2 &dest, const Infinity::Vector2 &v1, const Infinity::Vector2 &v2)
   {
      auto _v1 = DirectX::XMLoadFloat2(&v1);
      auto _v2 = DirectX::XMLoadFloat2(&v2);
      auto result = DirectX::XMVectorAdd(_v1, _v2);
      XMStoreFloat2(&dest, result);
   }

   inline void Add(Infinity::Vector3 &dest, const Infinity::Vector3 &v1, const Infinity::Vector3 &v2)
   {
      auto _v1 = DirectX::XMLoadFloat3(&v1);
      auto _v2 = DirectX::XMLoadFloat3(&v2);
      auto result = DirectX::XMVectorAdd(_v1, _v2);
      XMStoreFloat3(&dest, result);
   }

   inline void Add(Infinity::Vector4 &dest, const Infinity::Vector4 &v1, const Infinity::Vector4 &v2)
   {
      // TODO[OPT]: When working, collapse this as v4 is native type.
      auto _v1 = DirectX::XMLoadFloat4(&v1);
      auto _v2 = DirectX::XMLoadFloat4(&v2);
      auto result = DirectX::XMVectorAdd(_v1, _v2);
      XMStoreFloat4(&dest, result);
   }

   inline void Sub(Infinity::Vector2 &dest, const Infinity::Vector2 &v1, const Infinity::Vector2 &v2)
   {
      auto _v1 = DirectX::XMLoadFloat2(&v1);
      auto _v2 = DirectX::XMLoadFloat2(&v2);
      auto result = DirectX::XMVectorSubtract(_v1, _v2);
      XMStoreFloat2(&dest, result);
   }

   inline void Sub(Infinity::Vector3 &dest, const Infinity::Vector3 &v1, const Infinity::Vector3 &v2)
   {
      auto _v1 = DirectX::XMLoadFloat3(&v1);
      auto _v2 = DirectX::XMLoadFloat3(&v2);
      auto result = DirectX::XMVectorSubtract(_v1, _v2);
      XMStoreFloat3(&dest, result);
   }

   inline void Sub(Infinity::Vector4 &dest, const Infinity::Vector4 &v1, const Infinity::Vector4 &v2)
   {
      // TODO[OPT]: When working, collapse this as v4 is native type.
      auto _v1 = DirectX::XMLoadFloat4(&v1);
      auto _v2 = DirectX::XMLoadFloat4(&v2);
      auto result = DirectX::XMVectorSubtract(_v1, _v2);
      XMStoreFloat4(&dest, result);
   }
};

#else

#if (defined _WIN32 || defined _WIN64)

#include <d3dx9math.h>

namespace Infinity
{
typedef struct D3DXMATRIX  Matrix44;
typedef struct D3DXVECTOR2 Vector2;
typedef struct D3DXVECTOR3 Vector3;
typedef struct D3DXVECTOR4 Vector4;
}

#else

namespace Infinity
{

class Vector2
{
public:
   Vector2() {}
   Vector2(float _x, float _y) : x(_x), y(_y){}
   Vector2(const Vector2 &src) : x(src.x), y(src.y) {}

   void operator = (const Vector2 &src)
   {
      x = src.x;
      y = src.y;
   }

   bool operator == (const Vector2 &src) const
   {
      return (x == src.x) && (y == src.y);
   }

   bool operator != (const Vector2 &src) const
   {
      return (x != src.x) || (y != src.y);
   }

   Vector2 operator - () const
   {
      return Vector2(-x, -y);
   }

   Vector2 operator +  (const Vector2 &src) const
   {
      return Vector2(x + src.x, y + src.y);
   }

   Vector2 operator - (const Vector2 &src) const
   {
      return Vector2(x - src.x, y - src.y);
   }

   Vector2 operator * (float scale) const
   {
      return Vector2(scale*x, scale*y);
   }

   void operator *= (float scale)
   {
      x *= scale;
      y *= scale;
   }

   void operator -= (const Vector2 &src)
   {
      x -= src.x;
      y -= src.y;
   }

   void operator += (const Vector2 &src)
   {
      x += src.x;
      y += src.y;
   }

   float x, y;
};


class Vector3
{
public:
   Vector3() {}
   Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
   Vector3(const Vector3 &src) : x(src.x), y(src.y), z(src.z) {}

   void operator = (const Vector3 &src)
   {
      x = src.x;
      y = src.y;
      z = src.z;
   }

   bool operator == (const Vector3 &src) const
   {
      return (x == src.x) && (y == src.y) && (z == src.z);
   }

   bool operator != (const Vector3 &src) const
   {
      return (x != src.x) || (y != src.y) || (z != src.z);
   }

   Vector3 operator - () const
   {
      return Vector3(-x, -y, -z);
   }

   Vector3 operator +  (const Vector3 &src) const
   {
      return Vector3(x + src.x, y + src.y, z + src.z);
   }

   Vector3 operator - (const Vector3 &src) const
   {
      return Vector3(x - src.x, y - src.y, z - src.z);
   }

   Vector3 operator * (float scale) const
   {
      return Vector3(x *scale, y *scale, z * scale);
   }

   void operator *= (float scale)
   {
      x *= scale;
      y *= scale;
      z *= scale;
   }

   void operator += (const Vector3 &src)
   {
      x += src.x;
      y += src.y;
      z += src.z;
   }

   void operator -= (const Vector3  &src)
   {
      x -= src.x;
      y -= src.y;
      z -= src.z;
   }


   float x, y, z;
};


class INF_ALIGN_16 Vector4
{
public:
   Vector4() {}
   Vector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
   Vector4(const Vector4 &src) : x(src.x), y(src.y), z(src.z), w(src.w) {}

   void operator = (const Vector4 &src)
   {
      x = src.x;
      y = src.y;
      z = src.z;
      w = src.w;
   }

   bool operator == (const Vector4 &src) const
   {
      return (x == src.x) && (y == src.y) && (z == src.z) && (w == src.w);
   }

   bool operator != (const Vector4 &src) const
   {
      return (x != src.x) || (y != src.y) || (z != src.z) || (w != src.w);
   }

   Vector4 operator - () const
   {
      return Vector4(-x, -y, -z, -w);
   }

   Vector4 operator +  (const Vector4 &src) const
   {
      return Vector4(x + src.x, y + src.y, z + src.z, w + src.w);
   }

   Vector4 operator - (const Vector4 &src) const
   {
      return Vector4(x - src.x, y - src.y, z - src.z, w - src.w);
   }

   Vector4 operator * (float scale) const
   {
      return Vector4(x *scale, y * scale, z *scale, w * scale);
   }

   void operator *= (float scale)
   {
      x *= scale;
      y *= scale;
      z *= scale;
      w *= scale;
   }

   void operator += (const Vector4 &src)
   {
      x += src.x;
      y += src.y;
      z += src.z;
      w += src.w;
   }

   float x, y, z, w;
};




//typedef glm::mat4 Matrix44;
// #define GLM_MAT4


class INF_ALIGN_16 Matrix44
{
public:
   Matrix44() { assert((size_t(this) / 16) * 16 == size_t(this)); }
   Matrix44(float m0, float m1, float m2, float m3, float m4, float m5, float m6, float m7, float m8, float m9, float m10, float m11, float m12, float m13, float m14, float m15)
   {
      assert((size_t(this) / 16) * 16 == size_t(this));

      m[0][0] = m0;
      m[0][1] = m1;
      m[0][2] = m2;
      m[0][3] = m3;

      m[1][0] = m4;
      m[1][1] = m5;
      m[1][2] = m6;
      m[1][3] = m7;

      m[2][0] = m8;
      m[2][1] = m9;
      m[2][2] = m10;
      m[2][3] = m11;

      m[3][0] = m12;
      m[3][1] = m13;
      m[3][2] = m14;
      m[3][3] = m15;
   }

   Matrix44(const Matrix44 &src)
   {
      memcpy(m, src.m, sizeof(float)* 16);
   }

   union
   {
      // TODO: Remove references to this & zap it.
      struct
      {
         float _11, _12, _13, _14;
         float _21, _22, _23, _24;
         float _31, _32, _33, _34;
         float _41, _42, _43, _44;
      };

      float m[4][4];
   };

   Matrix44 operator * (const Matrix44 &src) const
   {
      // TODO: Ensure we don't use this because glm uses GL backwards right to left evaluation.
      assert(false);
      return src;
   }
};

}
#endif

#endif


namespace Infinity
{


inline Matrix44 *Matrix44_Create(uint32 count = 1)
{
#if (defined _WIN32 || defined _WIN64)
   return (Matrix44*)_aligned_malloc(sizeof(Matrix44) * count, 16);
#else
   return (Matrix44*) aligned_alloc(16, sizeof(Matrix44) * count);
#endif
}


typedef Vector4 Quaternion;


const Vector3 One_Vector3(1.0f, 1.0f, 1.0f);
const Vector2 Zero_Vector2(0.0f, 0.0f);
const Vector2 One_Vector2(1.0f, 1.0f);
const Vector3 Zero_Vector3(0.0f, 0.0f, 0.0f);
const Vector4 Zero_Vector4(0.0f, 0.0f, 0.0f, 0.0f);

const Vector3 Max_Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
const Vector3 Neg_Max_Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

const Matrix44 Identity(1.0f, 0.0f, 0.0f, 0.0f,
   0.0f, 1.0f, 0.0f, 0.0f,
   0.0f, 0.0f, 1.0f, 0.0f,
   0.0f, 0.0f, 0.0f, 1.0f);



#if !INF_DIRECTX_MATH

inline bool Equal(const Infinity::Vector2 &a, const Infinity::Vector2 &b)
{
   return !!(a == b);
}

inline bool Equal(const Infinity::Vector3 &a, const Infinity::Vector3 &b)
{
   return !!(a == b);
}

inline void Scale(Infinity::Vector2 &dest, float scale)
{
   dest *= scale;
}

inline void Sub(Infinity::Vector2 &dest, const Infinity::Vector2 &a, const Infinity::Vector2 &b)
{
   dest = a - b;
}

inline void Add(Infinity::Vector2 &dest, const Infinity::Vector2 &a, const Infinity::Vector2 &b)
{
   dest = a + b;
}


inline void Add(Infinity::Vector3 &dest, const Infinity::Vector3 &a, const Infinity::Vector3 &b)
{
   dest = a + b;
}

inline void Sub(Infinity::Vector3 &dest, const Infinity::Vector3 &a, const Infinity::Vector3 &b)
{
   dest = a - b;
}

inline void Scale(Infinity::Vector3 &dest, float scale)
{
   dest *= scale;
}

inline void Sub(Infinity::Vector4 &dest, const Infinity::Vector4 &a, const Infinity::Vector4 &b)
{
   dest = a - b;
}

#endif // !INF_DIRECTX_MATH



inline float DotProduct(const Vector3 &v1, const Vector3 &v2)
{
   return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

inline float DotProduct(const Vector4& _v1, const Vector3& v2)
{
   const Vector3& v1 = (*(const Vector3*)&_v1);
   return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}


inline float DotProduct(const Vector4 v1, const Vector4& v2)
{
   return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z) + (v1.w * v2.w);
}



class Plane
{
public:
	Plane() {}

   Plane(const Vector3 &plane_normal, float d)
	{
      m_equation.x = plane_normal.x;
      m_equation.y = plane_normal.y;
      m_equation.z = plane_normal.z;
      m_equation.w = d;  // Ensure sign of this is correct. Value is -ve of distance from origin of plane to satisfy plane eqn ax+by+cz+d = 0 equality.
	}

	Plane(const Vector3 &point_on_plane, const Vector3 &plane_normal)
	{
      m_equation.x = plane_normal.x;
      m_equation.y = plane_normal.y;
      m_equation.z = plane_normal.z;
      
      m_equation.w = -(point_on_plane.x * plane_normal.x + point_on_plane.y * plane_normal.y + point_on_plane.z * plane_normal.z);
      //m_equation.w = -DotProduct(plane_normal, point_on_plane);
	}

	Plane(float a, float b, float c, float d)
	{
      m_equation.x = a;
      m_equation.y = b;
      m_equation.z = c;
      m_equation.w = d;
	}

   // Distance from a point to the plane.
   // -ve values indicate the point is behind the plane.
   // +ve values indicate the point is in front of the plane.
   float GetDistance(const Vector3 &p) const
   {
      Vector4 _p(p.x, p.y, p.z, 1);
      return DotProduct(m_equation, _p);
   }

   Vector4 m_equation;// xyz: Plane normal. w = d, -ve of distance from origin to plane (along plane normal).
};


};

#endif // INF_MATH_H
