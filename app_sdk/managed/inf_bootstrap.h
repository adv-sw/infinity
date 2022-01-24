/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Project     : <><> Infinity Managed App SDK

File        : inf_bootstrap.h

Description : Binds Program to invoker via C interface.

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


#ifndef INF_BOOTSTRAP_H
#define INF_BOOTSTRAP_H


#include <infinity.h>

static Program *__app = nullptr;


INF_EXPORT uint32_t init(uint32_t type_version, uint32_t zone_handle, uint32_t scene_handle)  // TODO: uint32_t init_params & pass to Program_Create
{    
	if (type_version != Type_Version)
		return 0; // Incompatible.
	
    // WORKAROUND: Force the import. Compiler optimizing it out even though process function calls it. 
	create_native(0);
	
	uint32_t init_params = 0; // TODO: Pass in to init()
	__app = Program_Create(init_params);
	
	// Our parent added a ref when accessing these handles. Base class (Program) destructor removes those refs.
	__app->m_zone = new Zone(zone_handle);
    __app->m_sg   = new SceneGraph(scene_handle);  // Scene the script is included from, which is not necessarily the zone scene root, which is accessed using zone->GetScene()
	
	return __app ? __app->Init() : 0;
}


INF_EXPORT uint32_t terminate()
{    
	auto rc =  0;

	if (__app)
	{
		rc = __app->Terminate();
		delete __app;
	}
	
	return rc;
}


INF_EXPORT uint32_t process(float time)
{    
    return __app ? __app->Process(time) : 0;
}


INF_EXPORT void move(float x, float y, float z)
{    
    if (__app)
		__app->OnMove(x, y, z);
}


INF_EXPORT void button(uint32_t b)
{    
   if (__app)
	__app->OnButtonDown(b);
}



static Diagnostics * __d = nullptr;

void Log(const char *s)
{
	if (!__d)
		__d = new Diagnostics;
	
	__d->Log(s);
}




// Infinity object definitions that can't be inline here to keep compilation straightforward.

// We can do this here because inf_bootstrap.h should only been included into the file containing the Program definition, hence we won't end up with duplicate definitions.

Object *SceneGraph::CreateObject(Node *parent)
{
	return new Object(native_op(m_type, (uint32_t) SceneGraph_Op::CreateObject, m_handle, parent ? parent->GetHandle() : 0));
}



#endif // INF_BOOTSTRAP_H