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

#include "framework/CVarSystem.h"
#include "renderer/qgl.h"

// vertex cache calls should only be made by the front end

const int NUM_VERTEX_FRAMES = 2;

typedef enum {
	TAG_FREE,
	TAG_USED,
	TAG_FIXED,    // for the temp buffers
	TAG_TEMP    // in frame temp area, not static area
} vertBlockTag_t;

typedef struct vertCache_s {
	GLuint vbo;
	bool indexBuffer;    // holds indexes instead of vertexes
	intptr_t offset;
	int size;        // may be larger than the amount asked for, due
	// to round up and minimum fragment sizes
	int tag;        // a tag of 0 is a free block
	struct vertCache_s **user;        // will be set to zero when purged
	struct vertCache_s *next, *prev;  // may be on the static list or one of the frame lists
	int frameUsed;      // it can't be purged if near the current frame
	void* frontEndMemory;
	bool frontEndMemoryDirty;
} vertCache_t;


class idVertexCache {
public:
	void Init();

	void Shutdown();

	// called when vertex programs are enabled or disabled, because
	// the cached data is no longer valid
	void PurgeAll();

	// Tries to allocate space for the given data in fast vertex
	// memory, and copies it over.
	// Alloc does NOT do a touch, which allows purging of things
	// created at level load time even if a frame hasn't passed yet.
	// These allocations can be purged, which will zero the pointer.
	void Alloc(void *data, int bytes, vertCache_t **buffer, bool indexBuffer);

	// This will be a real pointer with virtual memory,
	// but it will be an int offset cast to a pointer of ARB_vertex_buffer_object
	void *Position(vertCache_t *buffer);

	vertCache_t * CreateTempVbo(int bytes, bool indexBuffer);

	// automatically freed at the end of the next frame
	// used for specular texture coordinates and gui drawing, which
	// will change every frame.
	// will return NULL if the vertex cache is completely full
	// As with Position(), this may not actually be a pointer you can access.
	vertCache_t *AllocFrameTemp(void *data, int bytes, bool indexBuffer);

	// notes that a buffer is used this frame, so it can't be purged
	// out from under the GPU
	void Touch(vertCache_t *buffer);

	// this block won't have to zero a buffer pointer when it is purged,
	// but it must still wait for the frames to pass, in case the GPU
	// is still referencing it
	void Free(vertCache_t *buffer);

	// updates the counter for determining which temp space to use
	// and which blocks can be purged
	// Also prints debugging info when enabled
	void EndFrame();

	void BeginBackEnd(int which);

	void UnbindIndex();
	void UnbindVertex();

	int GetListNum();
	// listVertexCache calls this
	void List();


private:
	void InitMemoryBlocks(int size);

	void ActuallyFree(vertCache_t *block);

	static idCVar r_showVertexCache;
	static idCVar r_vertexBufferMegs;
	static idCVar r_freeVertexBuffer;

	int staticCountTotal;
	int staticAllocTotal;    // for end of frame purging

	int staticAllocThisFrame;  // debug counter
	int staticCountThisFrame;
	int dynamicAllocThisFrame[NUM_VERTEX_FRAMES];
	int dynamicCountThisFrame;
	int staticAllocThisFrame_Index;  // for Index buffers
	int staticCountThisFrame_Index;
	int dynamicAllocThisFrame_Index[NUM_VERTEX_FRAMES];
	int dynamicCountThisFrame_Index;

	int currentFrame;      // for purgable block tracking
	int listNum;        // currentFrame % NUM_VERTEX_FRAMES, determines which tempBuffers to use

	int staticAllocMaximum;
	int dynamicAllocMaximum;
   	int dynamicAllocMaximum_Index;

	int vboMax;

	vertCache_t *tempBuffers[NUM_VERTEX_FRAMES];    // allocated at startup
	vertCache_t *tempIndexBuffers[NUM_VERTEX_FRAMES];    // allocated at startup (for Index buffers)

	bool tempOverflow;      // had to alloc a temp in static memory

	idBlockAlloc<vertCache_t, 1024> headerAllocator;

	vertCache_t freeStaticHeaders;    // head of doubly linked list
	vertCache_t freeStaticIndexHeaders;    // head of doubly linked list

	vertCache_t freeDynamicHeaders;    // head of doubly linked list
	vertCache_t freeDynamicIndexHeaders;    // head of doubly linked list (Index buffers)

	vertCache_t dynamicHeaders[NUM_VERTEX_FRAMES];      // head of doubly linked list
	vertCache_t dynamicIndexHeaders[NUM_VERTEX_FRAMES];      // head of doubly linked list (Index buffers)

	vertCache_t staticHeaders;      // head of doubly linked list in MRU order,
	vertCache_t staticIndexHeaders;      // head of doubly linked list in MRU order,


	vertCache_t deferredFreeList[NUM_VERTEX_FRAMES];    // head of doubly linked list

	int frameBytes;        // for each of NUM_VERTEX_FRAMES frames

	int currentBoundVBO;
	int currentBoundVBO_Index;
};

extern idVertexCache vertexCache;
