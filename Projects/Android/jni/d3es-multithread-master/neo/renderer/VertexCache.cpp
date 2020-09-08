/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
===========================================================================
 * Portions of this file are part of the D3wasm project (http://www.continuation-labs.com/projects/d3wasm)
 * Copyright (c) 2019 Gabriel Cuvillier.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
===========================================================================
*/

#include "sys/platform.h"
#include "framework/Common.h"
#include "renderer/tr_local.h"

#include "renderer/VertexCache.h"


static const int	FRAME_MEMORY_BYTES = 0x200000;
static const int	EXPAND_HEADERS = 1024;

idCVar idVertexCache::r_showVertexCache("r_showVertexCache", "0", CVAR_INTEGER | CVAR_RENDERER, "");
idCVar idVertexCache::r_vertexBufferMegs("r_vertexBufferMegs", "128", CVAR_INTEGER | CVAR_RENDERER, "");
idCVar idVertexCache::r_freeVertexBuffer("r_freeVertexBuffer", "1", CVAR_BOOL | CVAR_RENDERER, "");

idVertexCache		vertexCache;

/*
==============
R_ListVertexCache_f
==============
*/
static void R_ListVertexCache_f(const idCmdArgs& args) {
	vertexCache.List();
}

/*
==============
idVertexCache::ActuallyFree
==============
*/
void idVertexCache::ActuallyFree(vertCache_t* block) {
	if (!block) {
		common->Error("idVertexCache Free: NULL pointer");
	}

	if (block->user) {
		// let the owner know we have purged it
		*block->user = NULL;
		block->user = NULL;
	}

	// temp blocks are in a shared space that won't be freed
	if (block->tag != TAG_TEMP) {
		staticAllocTotal -= block->size;
		staticCountTotal--;

		if(block->vbo != -1 && r_freeVertexBuffer.GetBool())
		{
			if (block->indexBuffer)
            {
				if (block->vbo != currentBoundVBO_Index) {
					qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block->vbo);
				}
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block->vbo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, 0, GL_STREAM_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				//glDeleteBuffers(1, &block->vbo); // Doing this makes it slow AF
				//block->vbo = -1;
				currentBoundVBO_Index = -1;
            }
            else
            {
				if (block->vbo != currentBoundVBO) {
					qglBindBuffer(GL_ARRAY_BUFFER, block->vbo);
				}
				glBindBuffer(GL_ARRAY_BUFFER, block->vbo);
				glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STREAM_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				//glDeleteBuffers(1, &block->vbo); // Doing this makes it slow AF
				//block->vbo = -1;
				currentBoundVBO = -1;
            }
		}
	}

	block->tag = TAG_FREE;		// mark as free

	if(block->frontEndMemory)
	{
		free(block->frontEndMemory);
		block->frontEndMemory = NULL;
	}

	block->frontEndMemoryDirty = true;

	// unlink stick it back on the free list
	block->next->prev = block->prev;
	block->prev->next = block->next;


	// stick it on the front of the free list so it will be reused immediately
 	if (block->indexBuffer)
    {
  		block->next = freeStaticIndexHeaders.next;
		block->prev = &freeStaticIndexHeaders;
    }
    else
    {
		block->next = freeStaticHeaders.next;
		block->prev = &freeStaticHeaders;
	}

	block->next->prev = block;
	block->prev->next = block;
}
void idVertexCache::UnbindIndex()
{
	if(currentBoundVBO_Index != -1 )
	{
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		currentBoundVBO_Index = -1;
	}
}
void idVertexCache::UnbindVertex()
{
	if(currentBoundVBO != -1 )
	{
		qglBindBuffer(GL_ARRAY_BUFFER, 0);
		currentBoundVBO = -1;
	}
}

/*
==============
idVertexCache::Position

this will be a real pointer with virtual memory,
but it will be an int offset cast to a pointer with
ARB_vertex_buffer_object

The ARB_vertex_buffer_object will be bound
==============
*/
void* idVertexCache::Position(vertCache_t* buffer) {
	if (!buffer || buffer->tag == TAG_FREE) {
		common->FatalError("idVertexCache::Position: bad vertCache_t");
	}


	if( buffer->indexBuffer && (r_useIndexVBO.GetBool() == false)  )
	{
		UnbindIndex();
		return (uint8_t*)buffer->frontEndMemory + buffer->offset;
	}
	else if( !buffer->indexBuffer && (r_useVertexVBO.GetBool() == false) )
	{
		UnbindVertex();
		return (uint8_t*)buffer->frontEndMemory + buffer->offset;
	}

	// Create VBO if does not exist
	if( buffer->vbo == -1 )
	{
		if( !buffer->frontEndMemory )
			LOGI("MEMORY NULL");
		qglGenBuffers(1, &buffer->vbo);

		if(buffer->vbo > vboMax)
			vboMax = buffer->vbo;
    }


	// the ARB vertex object just uses an offset
	if (r_showVertexCache.GetInteger() == 2) {
		if (buffer->tag == TAG_TEMP) {
			common->Printf("GL_ARRAY_BUFFER_ARB = %i + %zd (%i bytes)\n", buffer->vbo, buffer->offset, buffer->size);
		} else {
			common->Printf("GL_ARRAY_BUFFER_ARB = %i (%i bytes)\n", buffer->vbo, buffer->size);
		}
	}
	if (buffer->indexBuffer) {
		if (buffer->vbo != currentBoundVBO_Index) {
			qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->vbo);
			currentBoundVBO_Index = buffer->vbo;
		}
	} else {
		if (buffer->vbo != currentBoundVBO) {
			qglBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);
			currentBoundVBO = buffer->vbo;
		}
	}

	// Update any new data
	if (buffer->frontEndMemoryDirty){
		//LOGI("Uploading Static vertex");
		if (buffer->indexBuffer) {
            qglBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer->size, buffer->frontEndMemory, GL_STATIC_DRAW);
        } else {
            qglBufferData(GL_ARRAY_BUFFER, buffer->size, buffer->frontEndMemory, GL_STATIC_DRAW);
        }
        buffer->frontEndMemoryDirty = false;
	}

	return (void*)buffer->offset;
}

//================================================================================

/*
===========
idVertexCache::Init
===========
*/
void idVertexCache::Init() {
	common->Printf("Init Vertex Cache\n");

	cmdSystem->AddCommand("listVertexCache", R_ListVertexCache_f, CMD_FL_RENDERER, "lists vertex cache");

	currentBoundVBO = -1;
	currentBoundVBO_Index = -1;

	if (r_vertexBufferMegs.GetInteger() < 8) {
		r_vertexBufferMegs.SetInteger(8);
	}

	// initialize the cache memory blocks
	freeStaticHeaders.next = freeStaticHeaders.prev = &freeStaticHeaders;
	staticHeaders.next = staticHeaders.prev = &staticHeaders;
 	freeStaticIndexHeaders.next = freeStaticIndexHeaders.prev = &freeStaticIndexHeaders;
    staticIndexHeaders.next = staticIndexHeaders.prev = &staticIndexHeaders;

	freeDynamicHeaders.next = freeDynamicHeaders.prev = &freeDynamicHeaders;
	freeDynamicIndexHeaders.next = freeDynamicIndexHeaders.prev = &freeDynamicIndexHeaders;


	// set up the dynamic frame memory
	frameBytes = FRAME_MEMORY_BYTES;
	staticAllocTotal = 0;
	staticCountTotal = 0;

	staticAllocMaximum = 0;
	dynamicAllocMaximum = 0;
   	dynamicAllocMaximum_Index = 0;

	vboMax = 0;

	// Allocate the temporary buffers (number of temporary buffers is NUM_VERTEX_FRAMES)
	for (int i = 0; i < NUM_VERTEX_FRAMES; i++) {
		tempBuffers[i] = CreateTempVbo(frameBytes, false);
		tempIndexBuffers[i] = CreateTempVbo(frameBytes, true);
		dynamicHeaders[i].next = dynamicHeaders[i].prev = &dynamicHeaders[i];
        dynamicIndexHeaders[i].next = dynamicIndexHeaders[i].prev = &dynamicIndexHeaders[i];
		deferredFreeList[i].next = deferredFreeList[i].prev = &deferredFreeList[i];
	}

	EndFrame();
}

/*
===========
idVertexCache::PurgeAll

Used when toggling vertex programs on or off, because
the cached data isn't valid
===========
*/
void idVertexCache::PurgeAll() {
	while (staticHeaders.next != &staticHeaders) {
		ActuallyFree(staticHeaders.next);
	}
 	while (staticIndexHeaders.next != &staticIndexHeaders) {
        ActuallyFree(staticIndexHeaders.next);
    }

	currentBoundVBO = -1;
	currentBoundVBO_Index = -1;
}

/*
===========
idVertexCache::Shutdown
===========
*/
void idVertexCache::Shutdown() {
	//	PurgeAll();	// !@#: also purge the temp buffers

	headerAllocator.Shutdown();

	currentBoundVBO = -1;
	currentBoundVBO_Index = -1;
}

vertCache_t* idVertexCache::CreateTempVbo(int bytes, bool indexBuffer)
{
	vertCache_t* block = headerAllocator.Alloc();

	block->next = NULL;
    block->prev = NULL;
    block->frontEndMemory = NULL;
	block->offset = 0;
	block->tag = TAG_FIXED;
	block->indexBuffer = indexBuffer;
	block->frontEndMemoryDirty = false;

#if USE_MAP
#else
	block->frontEndMemory = malloc(bytes + 16 );
#endif

	qglGenBuffers(1, &block->vbo);

	if (indexBuffer) {
	    qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block->vbo);
	    currentBoundVBO_Index = block->vbo;
	    qglBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)bytes, 0, GL_STREAM_DRAW);
	}
	else{
	    qglBindBuffer(GL_ARRAY_BUFFER, block->vbo);
	    currentBoundVBO = block->vbo;
	    qglBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)bytes, 0, GL_STREAM_DRAW);
	}

	return block;
}


/*
===========
idVertexCache::Alloc
===========
*/
void idVertexCache::Alloc(void* data, int size, vertCache_t** buffer, bool indexBuffer) {
	vertCache_t* block;

	if (size <= 0) {
		common->Error("idVertexCache::Alloc: size = %i\n", size);
	}

	// if we can't find anything, it will be NULL
	*buffer = NULL;

 	if (indexBuffer)
    {
		// if we don't have any remaining unused headers, allocate some more
		if (freeStaticIndexHeaders.next == &freeStaticIndexHeaders) {

			for (int i = 0; i < EXPAND_HEADERS; i++) {
				block = headerAllocator.Alloc();
				block->next = freeStaticIndexHeaders.next;
				block->prev = &freeStaticIndexHeaders;
				block->next->prev = block;
				block->prev->next = block;
				block->frontEndMemory = NULL;
				block->frontEndMemoryDirty = true;
				block->vbo = -1;
			}
		}
    }
    else
    {
		// if we don't have any remaining unused headers, allocate some more
		if (freeStaticHeaders.next == &freeStaticHeaders) {

			for (int i = 0; i < EXPAND_HEADERS; i++) {
				block = headerAllocator.Alloc();
				block->next = freeStaticHeaders.next;
				block->prev = &freeStaticHeaders;
				block->next->prev = block;
				block->prev->next = block;
				block->frontEndMemory = NULL;
				block->frontEndMemoryDirty = true;
				block->vbo = -1;
			}
		}
	}

   	if (indexBuffer)
	{
		// move it from the freeStaticIndexHeaders list to the staticHeaders list
		block = freeStaticIndexHeaders.next;
		block->next->prev = block->prev;
		block->prev->next = block->next;
		block->next = staticIndexHeaders.next;
		block->prev = &staticIndexHeaders;
		block->next->prev = block;
		block->prev->next = block;
	}
	else
	{
		// move it from the freeStaticHeaders list to the staticHeaders list
		block = freeStaticHeaders.next;
		block->next->prev = block->prev;
		block->prev->next = block->next;
		block->next = staticHeaders.next;
		block->prev = &staticHeaders;
		block->next->prev = block;
		block->prev->next = block;
	}

	block->offset = 0;
	block->tag = TAG_USED;

	// save data for debugging
	if (indexBuffer) {
		staticAllocThisFrame_Index += block->size;
		staticCountThisFrame_Index++;
	} else {
		staticAllocThisFrame += block->size;
		staticCountThisFrame++;
	}
	staticCountTotal++;
	staticAllocTotal += size;

	if(staticAllocTotal > staticAllocMaximum)
		staticAllocMaximum = staticAllocTotal;

	// this will be set to zero when it is purged
	block->user = buffer;
	*buffer = block;

	// allocation doesn't imply used-for-drawing, because at level
	// load time lots of things may be created, but they aren't
	// referenced by the GPU yet, and can be purged if needed.
	block->frameUsed = currentFrame - NUM_VERTEX_FRAMES;

	block->indexBuffer = indexBuffer;

	// TODO, make this more efficient...
#if 0
	if( block->frontEndMemory && size < block->size)
	{

	}
	else
	{
		free(block->frontEndMemory);
		block->frontEndMemory = malloc(size + 16);
		block->size = size;
	}
#else
	if( block->frontEndMemory )
		free(block->frontEndMemory);
	block->frontEndMemory = malloc(size + 16);
	block->size = size;
#endif

	memcpy( block->frontEndMemory, data, size );
	block->frontEndMemoryDirty = true;

	//Position(block);
}

/*
===========
idVertexCache::Touch
===========
*/
void idVertexCache::Touch(vertCache_t* block) {
	if (!block) {
		common->Error("idVertexCache Touch: NULL pointer");
	}

	if (block->tag == TAG_FREE) {
		common->FatalError("idVertexCache Touch: freed pointer");
	}
	if (block->tag == TAG_TEMP) {
		common->FatalError("idVertexCache Touch: temporary pointer");
	}

	block->frameUsed = currentFrame;

	// move to the head of the LRU list
	block->next->prev = block->prev;
	block->prev->next = block->next;

  	if (block->indexBuffer)
    {
		block->next = staticIndexHeaders.next;
		block->prev = &staticIndexHeaders;
		staticIndexHeaders.next->prev = block;
		staticIndexHeaders.next = block;
    }
    else
    {
		block->next = staticHeaders.next;
		block->prev = &staticHeaders;
		staticHeaders.next->prev = block;
		staticHeaders.next = block;
	}
}

/*
===========
idVertexCache::Free
===========
*/
void idVertexCache::Free(vertCache_t* block) {
	if (!block) {
		return;
	}

	if (block->tag == TAG_FREE) {
		common->FatalError("idVertexCache Free: freed pointer");
	}
	if (block->tag == TAG_TEMP) {
		common->FatalError("idVertexCache Free: temporary pointer");
	}

	// this block still can't be purged until the frame count has expired,
	// but it won't need to clear a user pointer when it is
	block->user = NULL;

	block->next->prev = block->prev;
	block->prev->next = block->next;

	block->next = deferredFreeList[listNum].next;
	block->prev = &deferredFreeList[listNum];
	deferredFreeList[listNum].next->prev = block;
	deferredFreeList[listNum].next = block;
}

/*
===========
idVertexCache::AllocFrameTemp

A frame temp allocation must never be allowed to fail due to overflow.
We can't simply sync with the GPU and overwrite what we have, because
there may still be future references to dynamically created surfaces.
===========
*/
vertCache_t* idVertexCache::AllocFrameTemp(void* data, int size, bool indexBuffer) {
	vertCache_t* block;

	if (size <= 0) {
		common->Error("idVertexCache::AllocFrameTemp: size = %i\n", size);
	}

	if (indexBuffer) {
		if (dynamicAllocThisFrame_Index[listNum] + size > frameBytes) {
			LOGI("WARNING DYNAMIC OVERFLOW!!");
			// if we don't have enough room in the temp block, allocate a static block,
			// but immediately free it so it will get freed at the next frame
			tempOverflow = true;
			Alloc(data, size, &block, indexBuffer);
			Free(block);
			return block;
		}
	} else {
		if (dynamicAllocThisFrame[listNum] + size > frameBytes) {
			LOGI("WARNING DYNAMIC OVERFLOW!!");
			// if we don't have enough room in the temp block, allocate a static block,
			// but immediately free it so it will get freed at the next frame
			tempOverflow = true;
			Alloc(data, size, &block, indexBuffer);
			Free(block);
			return block;
		}
	}

	// this data is just going on the shared dynamic list

	if (indexBuffer) {
		// if we don't have any remaining unused headers, allocate some more
		if (freeDynamicIndexHeaders.next == &freeDynamicIndexHeaders) {

			for (int i = 0; i < EXPAND_HEADERS; i++) {
				block = headerAllocator.Alloc();
				block->next = freeDynamicIndexHeaders.next;
				block->prev = &freeDynamicIndexHeaders;
				block->next->prev = block;
				block->prev->next = block;
			}
		}
	} else {
		// if we don't have any remaining unused headers, allocate some more
		if (freeDynamicHeaders.next == &freeDynamicHeaders) {

			for (int i = 0; i < EXPAND_HEADERS; i++) {
				block = headerAllocator.Alloc();
				block->next = freeDynamicHeaders.next;
				block->prev = &freeDynamicHeaders;
				block->next->prev = block;
				block->prev->next = block;
			}
		}
	}

	if (indexBuffer) {
		// move it from the freeIndexDynamicHeaders list to the dynamicIndexHeaders list
		block = freeDynamicIndexHeaders.next;
		block->next->prev = block->prev;
		block->prev->next = block->next;
		block->next = dynamicIndexHeaders[listNum].next;
		block->prev = &dynamicIndexHeaders[listNum];
		block->next->prev = block;
		block->prev->next = block;

	} else {
		// move it from the freeDynamicHeaders list to the dynamicHeaders list
		block = freeDynamicHeaders.next;
		block->next->prev = block->prev;
		block->prev->next = block->next;
		block->next = dynamicHeaders[listNum].next;
		block->prev = &dynamicHeaders[listNum];
		block->next->prev = block;
		block->prev->next = block;
	}

	block->frontEndMemory = NULL;
	block->frontEndMemoryDirty = false;

	// Try to align, might be faster
	size += 16;
	size &= 0xFFFFFFF0;

	block->size = size;

	block->tag = TAG_TEMP;
	block->indexBuffer = indexBuffer;
	if (indexBuffer) {

		block->offset = dynamicAllocThisFrame_Index[listNum];
		dynamicAllocThisFrame_Index[listNum] += block->size;
		dynamicCountThisFrame_Index++;
	} else {
		block->offset = dynamicAllocThisFrame[listNum];
		dynamicAllocThisFrame[listNum] += block->size;
		dynamicCountThisFrame++;
	}

	block->user = NULL;
	block->frameUsed = 0;

	// copy the data
	if (indexBuffer) {
		block->vbo = tempIndexBuffers[listNum]->vbo;
        memcpy( (char*)tempIndexBuffers[listNum]->frontEndMemory + block->offset, data, size);
        block->frontEndMemory = tempIndexBuffers[listNum]->frontEndMemory;
    } else {
    	block->vbo = tempBuffers[listNum]->vbo;
        memcpy( (char*)tempBuffers[listNum]->frontEndMemory + block->offset, data, size);
        block->frontEndMemory = tempBuffers[listNum]->frontEndMemory;
	}


	return block;
}

#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020
#define GL_MAP_WRITE_BIT 0x0002
void  idVertexCache::BeginBackEnd(int which)
{
//LOGI("BeginBackEnd list = %d, size index = %d, size = %d", listNum,dynamicAllocThisFrame_Index,dynamicAllocThisFrame);

#if USE_MAP
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER,  tempIndexBuffers[which]->vbo);
    currentBoundVBO_Index =  tempIndexBuffers[which]->vbo;
	qglUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );

	currentBoundVBO_Index =   tempIndexBuffers[(which + 1)  % NUM_VERTEX_FRAMES]->vbo;
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currentBoundVBO_Index );
	tempIndexBuffers[(which + 1)  % NUM_VERTEX_FRAMES]->frontEndMemory = qglMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, 0, FRAME_MEMORY_BYTES, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
#else
	if( r_useIndexVBO.GetBool() )
	{
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempIndexBuffers[which]->vbo);
    	currentBoundVBO_Index =  tempIndexBuffers[which]->vbo;
		//qglBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0, dynamicAllocThisFrame_Index[which], tempIndexBuffers[which]->frontEndMemory);
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, dynamicAllocThisFrame_Index[which], tempIndexBuffers[which]->frontEndMemory, GL_STREAM_DRAW);
	}
#endif


#if USE_MAP
	qglBindBuffer(GL_ARRAY_BUFFER,  tempBuffers[which]->vbo);
	currentBoundVBO = tempBuffers[which]->vbo;
	qglUnmapBuffer( GL_ARRAY_BUFFER );
	currentBoundVBO = tempBuffers[(which + 1)  % NUM_VERTEX_FRAMES]->vbo;

	qglBindBuffer(GL_ARRAY_BUFFER, currentBoundVBO);
	tempBuffers[(which + 1)  % NUM_VERTEX_FRAMES]->frontEndMemory = qglMapBufferRange( GL_ARRAY_BUFFER, 0, FRAME_MEMORY_BYTES, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
#else
	if( r_useVertexVBO.GetBool() )
	{
		qglBindBuffer(GL_ARRAY_BUFFER, tempBuffers[which]->vbo);
    	currentBoundVBO = tempBuffers[which]->vbo;
		//qglBufferSubData(GL_ARRAY_BUFFER,0, dynamicAllocThisFrame[which], tempBuffers[which]->frontEndMemory);
		qglBufferData(GL_ARRAY_BUFFER, dynamicAllocThisFrame[which], tempBuffers[which]->frontEndMemory, GL_STREAM_DRAW);
	}
#endif

	if(dynamicAllocThisFrame_Index[which] > dynamicAllocMaximum_Index)
		dynamicAllocMaximum_Index = dynamicAllocThisFrame_Index[which];

	if(dynamicAllocThisFrame[which] > dynamicAllocMaximum)
		dynamicAllocMaximum = dynamicAllocThisFrame[which];

}

/*
===========
idVertexCache::EndFrame
===========
*/
void idVertexCache::EndFrame() {
	// display debug information
	if (r_showVertexCache.GetBool()) {
		int	staticUseCount = 0;
		int staticUseSize = 0;

		for (vertCache_t* block = staticHeaders.next; block != &staticHeaders; block = block->next) {
			if (block->frameUsed == currentFrame) {
				staticUseCount++;
				staticUseSize += block->size;
			}
		}

		const char* frameOverflow = tempOverflow ? "(OVERFLOW)" : "";

		common->Printf("vertex dynamic:%i=%ik%s, static alloc:%i=%ik used:%i=%ik total:%i=%ik\n",
		               dynamicCountThisFrame + dynamicCountThisFrame_Index, (dynamicAllocThisFrame[listNum] + dynamicAllocThisFrame_Index[listNum]) / 1024, frameOverflow,
		               staticCountThisFrame + staticCountThisFrame_Index, (staticAllocThisFrame + staticAllocThisFrame_Index) / 1024,
		               staticUseCount, staticUseSize / 1024,
		               staticCountTotal, staticAllocTotal / 1024);
	}

	if (staticAllocTotal > r_vertexBufferMegs.GetInteger() * 1024 * 1024) {
		static bool bOnce = true;
		if (bOnce) {
			common->Printf("VBO size exceeds %dMB. Consider updating r_vertexBufferMegs.\n", r_vertexBufferMegs.GetInteger());
			bOnce = false;
		}
	}

#if 0
	// if our total static count is above our working memory limit, start purging things
	while (staticAllocTotal > r_vertexBufferMegs.GetInteger() * 1024 * 1024) {
		// free the least recently used

	}
#endif


	currentFrame = tr.frameCount;

	listNum = currentFrame % NUM_VERTEX_FRAMES;

	staticAllocThisFrame = 0;
	staticCountThisFrame = 0;
	staticAllocThisFrame_Index = 0;
	staticCountThisFrame_Index = 0;
	dynamicAllocThisFrame_Index[listNum] = 0;
	dynamicCountThisFrame_Index = 0;
	dynamicAllocThisFrame[listNum] = 0;
	dynamicCountThisFrame = 0;
	tempOverflow = false;

	// free all the deferred free headers
	while (deferredFreeList[listNum].next != &deferredFreeList[listNum]) {
		ActuallyFree(deferredFreeList[listNum].next);
	}

	// free all the frame temp headers
	vertCache_t* block = dynamicHeaders[listNum].next;
	if (block != &dynamicHeaders[listNum]) {
		block->prev = &freeDynamicHeaders;
		dynamicHeaders[listNum].prev->next = freeDynamicHeaders.next;
		freeDynamicHeaders.next->prev = dynamicHeaders[listNum].prev;
		freeDynamicHeaders.next = block;

		dynamicHeaders[listNum].next = dynamicHeaders[listNum].prev = &dynamicHeaders[listNum];
	}

	block = dynamicIndexHeaders[listNum].next;
	if (block != &dynamicIndexHeaders[listNum]) {
		block->prev = &freeDynamicIndexHeaders;
		dynamicIndexHeaders[listNum].prev->next = freeDynamicIndexHeaders.next;
		freeDynamicIndexHeaders.next->prev = dynamicIndexHeaders[listNum].prev;
		freeDynamicIndexHeaders.next = block;

		dynamicIndexHeaders[listNum].next = dynamicIndexHeaders[listNum].prev = &dynamicIndexHeaders[listNum];
	}
#if 0
	if(currentFrame % 60 == 0)
	{
		common->Printf("Current static = %d, Max static = %08d, Max dynamic = %08d, Max dynamicI = %08d, vboMax = %d\n", staticAllocTotal, staticAllocMaximum, dynamicAllocMaximum, dynamicAllocMaximum_Index,vboMax);
	}
#endif
}

/*
=============
idVertexCache::GetListNum
=============
*/
int idVertexCache::GetListNum()
{
	return listNum;
}

/*
=============
idVertexCache::List
=============
*/
void idVertexCache::List(void) {
	int	numActive = 0;
	int frameStatic = 0;
	int	totalStatic = 0;

	vertCache_t* block;
	for (block = staticHeaders.next; block != &staticHeaders; block = block->next) {
		numActive++;

		totalStatic += block->size;
		if (block->frameUsed == currentFrame) {
			frameStatic += block->size;
		}
	}


	int	numFreeStaticHeaders = 0;
	for (block = freeStaticHeaders.next; block != &freeStaticHeaders; block = block->next) {
		numFreeStaticHeaders++;
	}

	int	numFreeDynamicHeaders = 0;
	for (block = freeDynamicHeaders.next; block != &freeDynamicHeaders; block = block->next) {
		numFreeDynamicHeaders++;
	}

	int	numFreeDynamicIndexHeaders = 0;
	for (block = freeDynamicIndexHeaders.next; block != &freeDynamicIndexHeaders; block = block->next) {
		numFreeDynamicIndexHeaders++;
	}

	common->Printf("%i megs working set\n", r_vertexBufferMegs.GetInteger());
	common->Printf("%i dynamic temp buffers of %ik\n", NUM_VERTEX_FRAMES, frameBytes / 1024);
	common->Printf("%5i active static headers\n", numActive);
	common->Printf("%5i free static headers\n", numFreeStaticHeaders);
	common->Printf("%5i free dynamic headers\n", numFreeDynamicHeaders + numFreeDynamicIndexHeaders);
}

