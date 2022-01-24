/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity Managed App SDK

File        : inf_managed_class.h

Description : Definition of managed classes

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


#ifndef INF_MANAGED_CLASS_H
#define INF_MANAGED_CLASS_H

namespace Infinity
{


class Managed
{
public:
	Managed(uint32_t handle) : m_handle(handle) {}
   
   ~Managed() 
   { 
       release_native(m_type, m_handle); 
   }
   
   uint32_t GetHandle() { return m_handle; }
protected:
	uint32_t m_handle;
	uint32_t m_type;
};


class Attachment : public Managed
{
public:
	Attachment(uint32_t handle) : Managed(handle)
	{
		m_type = Type_Attachment;
	}
	
	Attachment(uint32_t handle, uint32_t type) : Managed(handle)
	{
		m_type = type;
	}
	
	void SetParameter(const char *id, const char *value)
	{ 
		uint32_t data[2];
		data[0] = (uint_ptr) id;
		data[1] = (uint_ptr) value;
		
		native_op(m_type, (uint32_t) Attachment_Op::SetParameter, m_handle, (uint_ptr) data);
	}
};


class Object;

class Node : public Managed
{
public:
	Node(uint32_t handle) : Managed(handle)
	{
		m_type = Type_Node;
	}
	
	void SetVelocity(const Vector3 &v)
	{
		native_op(m_type, (uint32_t) Node_Op::SetVelocity, m_handle, (uint_ptr) &v); 
	}
	
	void SetPosition(const Vector3 &v)
	{
		native_op(m_type, (uint32_t) Node_Op::SetPosition, m_handle, (uint_ptr) &v); 
	}
	
	void GetPosition(Vector3 &dest)
	{
		native_op(m_type, (uint32_t) Node_Op::GetPosition, m_handle, (uint_ptr) &dest);
	}
	
	Attachment *GetAttachment()
	{
		uint32_t att_type;
		uint32_t h = native_op(m_type, (uint32_t) Node_Op::GetAttachment, m_handle, (uint_ptr) &att_type);
		
		// TODO: Attachments of known derived types should return instances of those derived types.
		return new Attachment(h, att_type);
	}
	
	void Disconnect(bool zap)
	{
		native_op(m_type, (uint32_t) Node_Op::Disconnect, m_handle, (uint32_t) zap);
	}
};


class Mixer_Input : public Managed
{
public:
	Mixer_Input(uint32_t handle) : Managed(handle)
	{
		m_type = Type_Mixer_Input;
	}
	
	void SetTextureCoordinateScale(const Vector2 &v)
	{
		native_op(m_type, (uint32_t) Mixer_Input_Op::SetScale, m_handle, (uint_ptr) &v);
	}
	
	void SetTextureCoordinateOffset(const Vector2 &v)
	{
		native_op(m_type, (uint32_t) Mixer_Input_Op::SetOffset, m_handle, (uint_ptr) &v);
	}
};



class Material_Part : public Managed
{
public:
	Material_Part(uint32_t handle) : Managed(handle)
	{
		m_type = Type_Material_Part;
	}
	
	Mixer_Input *GetInput(uint32_t id)
	{
		return new Mixer_Input(native_op(m_type, (uint32_t) Material_Part_Op::GetInput, m_handle, id));
	}
};
	
	
class Material : public Managed
{
public:
	Material(uint32_t handle) : Managed(handle)
	{
		m_type = Type_Material;
	}
	
	Material_Part *GetPart(uint32_t id)
	{
		return new Material_Part(native_op(m_type, (uint32_t) Material_Op::GetPart, m_handle, id));
	}
	
    void GetColour(Vector3 &dest)
	{
		native_op(m_type, (uint32_t) Material_Op::GetColour, m_handle, (uint_ptr) &dest);
	}
	
	void SetColour(const Vector3 &c)
	{
		native_op(m_type, (uint32_t) Material_Op::SetColour, m_handle, (uint_ptr) &c);
	}
};


class Animation : public Managed
{
public:
	Animation(uint32_t handle) : Managed(handle)
	{
		m_type = Type_Animation;
	}
	
	float GetDuration()
	{
		float dest;
		native_op(m_type, (uint32_t) Animation_Op::GetDuration, m_handle, (uint_ptr) &dest);
		return dest;
	}
};


class SceneGraph : public Managed
{
public:
	SceneGraph(uint32_t handle) : Managed(handle)
	{
		m_type = Type_SceneGraph;
	}
	
	Node *GetRoot()
	{
		auto handle = native_op(m_type, (uint32_t) SceneGraph_Op::GetRoot, m_handle);
		return handle ? new Node(handle) : nullptr;
	}
	
	Animation *GetAnimation()
	{
		auto handle = native_op(m_type, (uint32_t) SceneGraph_Op::GetAnimation, m_handle);
		return handle ? new Animation(handle) : nullptr;
	}
	
	Node *CreateNode(Node *parent)
	{
		Managed *thing = (Managed*) parent;
		return new Node(native_op(m_type, (uint32_t) SceneGraph_Op::CreateNode, m_handle, thing ? thing->GetHandle() : 0));
	}
	
	Object *CreateObject(Node *parent);
	
    Node *FindNode(const char *id)
	{
		auto handle = native_op(m_type, (uint32_t) SceneGraph_Op::FindNode, m_handle, (uint_ptr) id);
		return handle ? new Node(handle) : nullptr;
	}
	
	Material *FindMaterial(const char *id)
	{
		auto handle = native_op(m_type, (uint32_t) SceneGraph_Op::FindMaterial,  m_handle, (uint_ptr) id);
		return handle ? new Material(handle) : nullptr;
	}	
};


class Object : public Attachment
{
public:
	Object(uint32_t handle) : Attachment(handle)
	{
		m_type = Type_Object;
	}
	
	SceneGraph *GetGeometry()
	{
		auto handle = native_op(m_type, (uint32_t) Object_Op::GetGeometry, m_handle);
		return handle ? new SceneGraph(handle) : nullptr;
	}
	
	
	void SetAnimationPlaybackOffset(float t)
	{
		native_op(m_type, (uint32_t) Object_Op::SetAnimationPlaybackOffset, m_handle, uint_ptr(&t));
	}
	
	bool Intersection(Object *thing)
	{
		return native_op(m_type, (uint32_t) Object_Op::Intersection, m_handle, thing ? thing->GetHandle() : 0);
	}
	
	void SetGeometry(SceneGraph *geom)
	{
		Managed *thing = (Managed*) geom;
		native_op(m_type, (uint32_t) Object_Op::SetGeometry, m_handle, thing ? thing->GetHandle() : 0);
	}
};


class Sound_Manager : public Managed
{
public:
	Sound_Manager(): Managed(0)
	{
		m_type = Type_Sound_Manager;
		m_handle = native_op(m_type, (uint32_t) Sound_Manager_Op::Get);
	}
	
	float Sample(uint32_t channel, uint32_t freq)
	{
		uint32_t data[2] = { channel, freq };
		auto result = native_op(m_type, (uint32_t) Sound_Manager_Op::Sample, m_handle, (uint_ptr) data);
		
		return as_float(result);
	}
};


class Zone : public Managed
{
public:
	Zone(uint32_t handle): Managed(handle)
	{
		m_type = Type_Zone;
	}

    SceneGraph *GetScene()
	{
		auto handle = native_op(m_type, (uint32_t) Zone_Op::GetScene, m_handle);
		return handle ? new SceneGraph(handle) : nullptr;
	}
	
	bool Loading()
	{
		return native_op(m_type, (uint32_t) Zone_Op::Loading, m_handle);
	}
	
	SceneGraph *Find(const char *url)
	{
		auto handle = native_op(m_type, (uint32_t) Zone_Op::FindSceneGraph, m_handle, (uint_ptr) url);
		return handle ? new SceneGraph(handle) : nullptr;
	}
};


class Diagnostics : Managed
{
public:	
	Diagnostics(): Managed(0)
	{
		m_type = Type_Diagnostics;
		m_handle = native_op(m_type, (uint32_t) Diagnostics_Op::Get);
	}
	
	void Log(const char *s)
	{
		native_op(m_type, (uint32_t) Diagnostics_Op::Log, m_handle, (uint_ptr) s);
	}
};


}

#endif // INF_MANAGED_CLASS_H
