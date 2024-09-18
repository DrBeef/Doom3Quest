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
#include "framework/Game.h"
#include "../game/Vr.h"
#include "sys/platform.h"
#include "idlib/containers/List.h"
#include "framework/EventLoop.h"
#include "framework/Session.h"
#include "framework/DemoFile.h"
#include "renderer/ModelManager.h"
#include "renderer/Material.h"
#include "renderer/GuiModel.h"
#include "renderer/VertexCache.h"
#include "renderer/RenderWorld_local.h"

#include "renderer/tr_local.h"

idRenderSystemLocal	tr;
idRenderSystem	*renderSystem = &tr;


/*
=====================
R_PerformanceCounters

This prints both front and back end counters, so it should
only be called when the back end thread is idle.
=====================
*/
static void R_PerformanceCounters( void ) {
	if ( r_showPrimitives.GetInteger() != 0 ) {

		float megaBytes = globalImages->SumOfUsedImages() / ( 1024*1024.0 );

		if ( r_showPrimitives.GetInteger() > 1 ) {
			common->Printf( "v:%i ds:%i t:%i/%i v:%i/%i st:%i sv:%i image:%5.1f MB\n",
			                tr.pc.c_numViews,
			                backEnd.pc.c_drawElements + backEnd.pc.c_shadowElements,
			                backEnd.pc.c_drawIndexes / 3,
			                ( backEnd.pc.c_drawIndexes - backEnd.pc.c_drawRefIndexes ) / 3,
			                backEnd.pc.c_drawVertexes,
			                ( backEnd.pc.c_drawVertexes - backEnd.pc.c_drawRefVertexes ),
			                backEnd.pc.c_shadowIndexes / 3,
			                backEnd.pc.c_shadowVertexes,
			                megaBytes
			              );
		} else {
			common->Printf( "views:%i draws:%i tris:%i (shdw:%i) (vbo:%i) image:%5.1f MB\n",
			                tr.pc.c_numViews,
			                backEnd.pc.c_drawElements + backEnd.pc.c_shadowElements,
			                ( backEnd.pc.c_drawIndexes + backEnd.pc.c_shadowIndexes ) / 3,
			                backEnd.pc.c_shadowIndexes / 3,
			                backEnd.pc.c_vboIndexes / 3,
			                megaBytes
			              );
		}
	}

	if ( r_showDynamic.GetBool() ) {
		common->Printf( "callback:%i md5:%i dfrmVerts:%i dfrmTris:%i tangTris:%i guis:%i\n",
		                tr.pc.c_entityDefCallbacks,
		                tr.pc.c_generateMd5,
		                tr.pc.c_deformedVerts,
		                tr.pc.c_deformedIndexes/3,
		                tr.pc.c_tangentIndexes/3,
		                tr.pc.c_guiSurfs
		              );
	}

	if ( r_showCull.GetBool() ) {
		common->Printf( "%i sin %i sclip  %i sout %i bin %i bout\n",
		                tr.pc.c_sphere_cull_in, tr.pc.c_sphere_cull_clip, tr.pc.c_sphere_cull_out,
		                tr.pc.c_box_cull_in, tr.pc.c_box_cull_out );
	}

	if ( r_showAlloc.GetBool() ) {
		common->Printf( "alloc:%i free:%i\n", tr.pc.c_alloc, tr.pc.c_free );
	}

	if ( r_showInteractions.GetBool() ) {
		common->Printf( "createInteractions:%i createLightTris:%i createShadowVolumes:%i\n",
		                tr.pc.c_createInteractions, tr.pc.c_createLightTris, tr.pc.c_createShadowVolumes );
	}
	if ( r_showDefs.GetBool() ) {
		common->Printf( "viewEntities:%i  shadowEntities:%i  viewLights:%i\n", tr.pc.c_visibleViewEntities,
		                tr.pc.c_shadowViewEntities, tr.pc.c_viewLights );
	}
	if ( r_showUpdates.GetBool() ) {
		common->Printf( "entityUpdates:%i  entityRefs:%i  lightUpdates:%i  lightRefs:%i\n",
		                tr.pc.c_entityUpdates, tr.pc.c_entityReferences,
		                tr.pc.c_lightUpdates, tr.pc.c_lightReferences );
	}
	if ( r_showMemory.GetBool() ) {
		int	m1 = frameData ? frameData->memoryHighwater : 0;
		common->Printf( "frameData: %i (%i)\n", R_CountFrameData(), m1 );
	}

	memset( &tr.pc, 0, sizeof( tr.pc ) );
	memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}



/*
====================
R_IssueRenderCommands

Called by R_EndFrame each frame
====================
*/
static void R_IssueRenderCommands( volatile frameData_t *fd ) {
	if ( fd->cmdHead->commandId == RC_NOP
	        && !fd->cmdHead->next ) {
		// nothing to issue
		return;
	}

	// r_skipBackEnd allows the entire time of the back end
	// to be removed from performance measurements, although
	// nothing will be drawn to the screen.  If the prints
	// are going to a file, or r_skipBackEnd is later disabled,
	// usefull data can be received.

	// r_skipRender is usually more usefull, because it will still
	// draw 2D graphics
	if ( !r_skipBackEnd.GetBool() ) {
		RB_ExecuteBackEndCommands( fd->cmdHead );
	}
}

/*
============
R_GetCommandBuffer

Returns memory for a command buffer (stretchPicCommand_t,
drawSurfsCommand_t, etc) and links it to the end of the
current command chain.
============
*/
void *R_GetCommandBuffer( int bytes ) {
	emptyCommand_t	*cmd;

	cmd = (emptyCommand_t *)R_FrameAlloc( bytes );
	cmd->next = NULL;
	frameData->cmdTail->next = &cmd->commandId;
	frameData->cmdTail = cmd;

	return (void *)cmd;
}


/*
====================
R_ClearCommandChain

Called after every buffer submission
and by R_ToggleSmpFrame
====================
*/
void R_ClearCommandChain( void ) {
	// clear the command chain
	frameData->cmdHead = frameData->cmdTail = (emptyCommand_t *)R_FrameAlloc( sizeof( *frameData->cmdHead ) );
	frameData->cmdHead->commandId = RC_NOP;
	frameData->cmdHead->next = NULL;
}

/*
=================
R_ViewStatistics
=================
*/
static void R_ViewStatistics( viewDef_t *parms ) {
	// report statistics about this view
	if ( !r_showSurfaces.GetBool() ) {
		return;
	}
	common->Printf( "view:%p surfs:%i\n", parms, parms->numDrawSurfs );
}

/*
=============
R_AddDrawViewCmd

This is the main 3D rendering command.  A single scene may
have multiple views if a mirror, portal, or dynamic texture is present.
=============
*/
void	R_AddDrawViewCmd( viewDef_t *parms ) {
	drawSurfsCommand_t	*cmd;

	cmd = (drawSurfsCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_DRAW_VIEW;

	cmd->viewDef = parms;

	if ( parms->viewEntitys ) {
		// save the command for r_lockSurfaces debugging
		tr.lockSurfacesCmd = *cmd;
	}

	tr.pc.c_numViews++;

	R_ViewStatistics( parms );
}


//=================================================================================


/*
======================
R_LockSurfaceScene

r_lockSurfaces allows a developer to move around
without changing the composition of the scene, including
culling.  The only thing that is modified is the
view position and axis, no front end work is done at all


Add the stored off command again, so the new rendering will use EXACTLY
the same surfaces, including all the culling, even though the transformation
matricies have been changed.  This allow the culling tightness to be
evaluated interactively.
======================
*/
void R_LockSurfaceScene( viewDef_t *parms ) {
	drawSurfsCommand_t	*cmd;
	viewEntity_t			*vModel;

	// set the matrix for world space to eye space
	R_SetViewMatrix( parms );

    tr.lockSurfacesCmd.viewDef->worldSpace = parms->worldSpace;

    // update the view origin and axis, and all
    // the entity matricies
    for (vModel = tr.lockSurfacesCmd.viewDef->viewEntitys; vModel; vModel = vModel->next) {
    	for (int eye = 0; eye < 3; ++eye) {
			myGlMultMatrix(vModel->modelMatrix,
						   tr.lockSurfacesCmd.viewDef->worldSpace.eyeViewMatrix[eye],
						   vModel->eyeViewMatrix[eye]);
		}
    }

	// add the stored off surface commands again
	cmd = (drawSurfsCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	*cmd = tr.lockSurfacesCmd;
}

/*
=============
R_CheckCvars

See if some cvars that we watch have changed
=============
*/
static void R_CheckCvars( void ) {
	globalImages->CheckCvars();

	//game->CheckRenderCvars();

	// gamma stuff
	if ( r_gamma.IsModified() || r_brightness.IsModified() ) {
		r_gamma.ClearModified();
		r_brightness.ClearModified();
		R_SetColorMappings();
	}
}

/*
=============
idRenderSystemLocal::idRenderSystemLocal
=============
*/
idRenderSystemLocal::idRenderSystemLocal( void ) {
	Clear();
}

/*
=============
idRenderSystemLocal::~idRenderSystemLocal
=============
*/
idRenderSystemLocal::~idRenderSystemLocal( void ) {
}

/*
=============
SetColor

This can be used to pass general information to the current material, not
just colors
=============
*/
void idRenderSystemLocal::SetColor( const idVec4 &rgba ) {
	guiModel->SetColor(rgba[0], rgba[1], rgba[2], hudOpacity * rgba[3]);
}


/*
=============
SetColor4
=============
*/
void idRenderSystemLocal::SetColor4( float r, float g, float b, float a ) {
	a = hudOpacity * a;
	guiModel->SetColor( r, g, b, a );
}

/*
=============
DrawStretchPic
=============
*/
void idRenderSystemLocal::DrawStretchPic( const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *material,
        bool clip, float min_x, float min_y, float max_x, float max_y ) {
	guiModel->DrawStretchPic( verts, indexes, vertCount, indexCount, material,
	                          clip, min_x, min_y, max_x, max_y );
}

/*
=============
DrawStretchPic

x/y/w/h are in the 0,0 to 640,480 range
=============
*/
void idRenderSystemLocal::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material ) {
	guiModel->DrawStretchPic( x, y, w, h, s1, t1, s2, t2, material );
}

/*
=============
DrawStretchTri

x/y/w/h are in the 0,0 to 640,480 range
=============
*/
void idRenderSystemLocal::DrawStretchTri( idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material ) {
	tr.guiModel->DrawStretchTri( p1, p2, p3, t1, t2, t3, material );
}

/*
=============
GlobalToNormalizedDeviceCoordinates
=============
*/
void idRenderSystemLocal::GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc ) {
	R_GlobalToNormalizedDeviceCoordinates( global, ndc );
}

/*
=============
GlobalToNormalizedDeviceCoordinates
=============
*/
void idRenderSystemLocal::GetGLSettings( int& width, int& height ) {
	width = glConfig.vidWidth;
	height = glConfig.vidHeight;
}

/*
=====================
idRenderSystemLocal::DrawSmallChar

small chars are drawn at native screen resolution
=====================
*/
void idRenderSystemLocal::DrawSmallChar( int x, int y, int ch, const idMaterial *material ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -SMALLCHAR_HEIGHT ) {
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
	                fcol, frow,
	                fcol + size, frow + size,
	                material );
}

/*
==================
idRenderSystemLocal::DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idRenderSystemLocal::DrawSmallStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) {
	idVec4		color;
	const unsigned char	*s;
	int			xx;

	// draw the colored text
	s = (const unsigned char*)string;
	xx = x;
	SetColor( setColor );
	while ( *s ) {
		if ( idStr::IsColor( (const char*)s ) ) {
			if ( !forceColor ) {
				if ( *(s+1) == C_COLOR_DEFAULT ) {
					SetColor( setColor );
				} else {
					color = idStr::ColorForIndex( *(s+1) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawSmallChar( xx, y, *s, material );
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}

/*
=====================
idRenderSystemLocal::DrawBigChar
=====================
*/
void idRenderSystemLocal::DrawBigChar( int x, int y, int ch, const idMaterial *material ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -BIGCHAR_HEIGHT ) {
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	DrawStretchPic( x, y, BIGCHAR_WIDTH, BIGCHAR_HEIGHT,
	                fcol, frow,
	                fcol + size, frow + size,
	                material );
}

/*
==================
idRenderSystemLocal::DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idRenderSystemLocal::DrawBigStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) {
	idVec4		color;
	const char	*s;
	int			xx;

	// draw the colored text
	s = string;
	xx = x;
	SetColor( setColor );
	while ( *s ) {
		if ( idStr::IsColor( s ) ) {
			if ( !forceColor ) {
				if ( *(s+1) == C_COLOR_DEFAULT ) {
					SetColor( setColor );
				} else {
					color = idStr::ColorForIndex( *(s+1) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawBigChar( xx, y, *s, material );
		xx += BIGCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}

//======================================================================================

/*
====================
BeginFrame
====================
*/
void idRenderSystemLocal::BeginFrame( int windowWidth, int windowHeight ) {
	setBufferCommand_t	*cmd;

	if ( !glConfig.isInitialized ) {
		return;
	}

	guiModel->Clear();

	// for the larger-than-window tiled rendering screenshots
	if ( tiledViewport[0] ) {
		windowWidth = tiledViewport[0];
		windowHeight = tiledViewport[1];
	}

	glConfig.vidWidth = windowWidth;
	glConfig.vidHeight = windowHeight;

	renderCrops[0].x = 0;
	renderCrops[0].y = 0;
	renderCrops[0].width = windowWidth;
	renderCrops[0].height = windowHeight;
	currentRenderCrop = 0;

	// screenFraction is just for quickly testing fill rate limitations
	if ( r_screenFraction.GetInteger() != 100 ) {
		int	w = SCREEN_WIDTH * r_screenFraction.GetInteger() / 100.0f;
		int h = SCREEN_HEIGHT * r_screenFraction.GetInteger() / 100.0f;
		CropRenderSize( w, h );
	}


	// this is the ONLY place this is modified
	frameCount++;

	// just in case we did a common->Error while this
	// was set
	guiRecursionLevel = 0;

	// the first rendering will be used for commands like
	// screenshot, rather than a possible subsequent remote
	// or mirror render
//	primaryWorld = NULL;

	// set the time for shader effects in 2D rendering
	frameShaderTime = eventLoop->Milliseconds() * 0.001;

	//
	// draw buffer stuff
	//
	cmd = (setBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_SET_BUFFER;
	cmd->frameCount = frameCount;
	cmd->buffer = 0;

}

void idRenderSystemLocal::WriteDemoPics() {
	session->writeDemo->WriteInt( DS_RENDER );
	session->writeDemo->WriteInt( DC_GUI_MODEL );
	guiModel->WriteToDemo( session->writeDemo );
}

void idRenderSystemLocal::DrawDemoPics() {
	demoGuiModel->EmitFullScreen();
}


void GLimp_ActivateContext();
void GLimp_DeactivateContext();

int idRenderSystemLocal::BackendThreadRunner(void *localRenderSystem)
{
	idRenderSystemLocal *local = (idRenderSystemLocal*)localRenderSystem;
	local->BackendThread();

	return 0;
}

void idRenderSystemLocal::BackendThreadWait()
{
	while(!backendFinished)
    {
        //usleep(1000 * 3);
        Sys_WaitForEvent(TRIGGER_EVENT_BACKEND_FINISHED);
        //usleep(500);
    }
}

void idRenderSystemLocal::BackendThread()
{
	GLimp_ActivateContext();

	while( 1 )
	{
		if( useSpinLock )
    	{
			while(!backendThreadRun)
			{
				if(spinLockDelay)
					usleep(spinLockDelay);
			}
			backendThreadRun = false;
    	}
    	else
    	{
			// LOGI("Wait TRIGGER_EVENT_RUN_BACKEND");
			Sys_WaitForEvent(TRIGGER_EVENT_RUN_BACKEND);
			// LOGI("Done TRIGGER_EVENT_RUN_BACKEND");
		}

		// Thread will be woken up to either shutdown or render
		if( backendThreadShutdown )
		{
			LOGI("Backend thread ending..");
			// Release context
			GLimp_DeactivateContext();

			// Finish thread
			break;
		}
		else
		{
			BackendThreadTask();
		}
	}
}


void idRenderSystemLocal::BackendThreadTask()
{
	idImage * img;
	// Purge all images
	while( (img = globalImages->GetNextPurgeImage()) != NULL )
	{
		//LOGI("IMAGE PURGE!");
		img->PurgeImage();
	}

	// Load all images
	while( (img = globalImages->GetNextAllocImage()) != NULL )
	{
		// LOGI("IMAGE LOAD!");
		img->ActuallyLoadImage( false );
	}


	if( useSpinLock )
	{
		imagesFinished = true;
	}
	else
	{
		Sys_TriggerEvent(TRIGGER_EVENT_IMAGES_PROCESSES);
	}

	vertexCache.BeginBackEnd(vertListToRender);

	R_IssueRenderCommands(fdToRender);

	// Take screen shot
	if(pixels)
	{
		qglReadPixels( pixelsCrop->x, pixelsCrop->y, pixelsCrop->width, pixelsCrop->height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pixels );
		pixels = NULL;
		pixelsCrop = NULL;
	}

	backendFinished = true;
	Sys_TriggerEvent(TRIGGER_EVENT_BACKEND_FINISHED);
}

void idRenderSystemLocal::BackendThreadExecute()
{
	//LOGI("BackendThreadRun called..");
	imagesFinished = false;
	backendFinished = false;

	if(multithreadActive)
	{
		if ( !renderThread.threadHandle ) {
			LOGI("Starting new backend thread");

			GLimp_DeactivateContext();
			backendThreadShutdown = false;
			Sys_CreateThread( &idRenderSystemLocal::BackendThreadRunner, this, renderThread, "renderThread" );
		}

		// Start Thread
		if( useSpinLock )
		{
			backendThreadRun = true;
		}
		else
		{
			// LOGI("Trigger TRIGGER_EVENT_RUN_BACKEND");
			Sys_TriggerEvent(TRIGGER_EVENT_RUN_BACKEND);
		}
	}
	else // No multithread, just execute in sequence
	{
		BackendThreadTask();
	}
}

void idRenderSystemLocal::BackendThreadShutdown()
{
	LOGI("Shutting down backend thread");

	if( multithreadActive && renderThread.threadHandle )
	{
		// Wait for thread to be ready
		BackendThreadWait();
		// Set shutdown flag
		backendThreadShutdown = true;

		// Start Thread
		if( useSpinLock )
		{
			backendThreadRun = true;
		}
		else
		{
			// LOGI("Trigger TRIGGER_EVENT_RUN_BACKEND");
			Sys_TriggerEvent(TRIGGER_EVENT_RUN_BACKEND);
		}

		// Join thread and wait until finished
		Sys_DestroyThread(renderThread);

		// Clear handle
		renderThread.threadHandle = 0;

		//Take GL context
		GLimp_ActivateContext();
	}
}

void idRenderSystemLocal::RenderCommands(renderCrop_t *pc, byte *pix)
{
	// Only do rendering if the app is actually active
	if(windowActive)
	{
		//Wait for last backend rendering to finish
		BackendThreadWait();

		// Limit maximum FPS
		int maxFPS = r_maxFps.GetInteger();
		if(maxFPS)
		{
			unsigned int limit = 1000 / maxFPS;
			unsigned int currentTime = Sys_Milliseconds();
			int timeTook = currentTime - lastRenderTime;
			if(timeTook < limit)
			{
				usleep((limit - timeTook) * 1000);
			}
			lastRenderTime = Sys_Milliseconds();
		}

		// LOGI("---------------------NEW FRAME---------------------");

		// We have turned off multithreading, we need to shut it down
		if(multithreadActive && !r_multithread.GetBool())
		{
			BackendThreadShutdown();
			multithreadActive = false;
		}
		else if( !multithreadActive && r_multithread.GetBool() )
		{
			multithreadActive = true;
		}

		//Save the current vertexs and framedata to use for next render
		vertListToRender = vertexCache.GetListNum();
		fdToRender = frameData;

		//Save the potential pixel
		pixelsCrop = pc;
		pixels = pix;

		BackendThreadExecute();

		// Wait for the backend to load any images, this only really happens at level load time
		// Problem is image loading is not thread safe, hence the wait
		if(useSpinLock)
		{
			while(!imagesFinished)
			{
				if(spinLockDelay)
					usleep(spinLockDelay);
			}
		}
		else
		{
			Sys_WaitForEvent(TRIGGER_EVENT_IMAGES_PROCESSES);
		}
	}

	// If we are waiting for pixel data, make sure we wait for the backend to finish
	if(pix)
	{
		BackendThreadWait();
	}

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current buffers
	R_ToggleSmpFrame();

	// we can now release the vertexes used this frame
	vertexCache.EndFrame();

	R_ClearCommandChain();
}

/*
=============
EndFrame

Returns the number of msec spent in the back end
=============
*/
void idRenderSystemLocal::EndFrame( int *frontEndMsec, int *backEndMsec ) {
	emptyCommand_t *cmd;

	if ( !glConfig.isInitialized ) {
		return;
	}

	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();

	// save out timing information
	if ( frontEndMsec ) {
		*frontEndMsec = pc.frontEndMsec;
	}
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}

	// print any other statistics and clear all of them
	R_PerformanceCounters();

	// check for dynamic changes that require some initialization
	R_CheckCvars();

	// check for errors
	GL_CheckErrors();

	// add the swapbuffers command
	cmd = (emptyCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_SWAP_BUFFERS;

	// Render the commands. No pixel data passed so it will return immediatle if multithreading
	RenderCommands(0, 0);

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_END_FRAME );
		if ( r_showDemo.GetBool() ) {
			common->Printf( "write DC_END_FRAME\n" );
		}
	}
}

/*
=====================
RenderViewToViewport

Converts from SCREEN_WIDTH / SCREEN_HEIGHT coordinates to current cropped pixel coordinates
=====================
*/
void idRenderSystemLocal::RenderViewToViewport( const renderView_t *renderView, idScreenRect *viewport ) {
	renderCrop_t	*rc = &renderCrops[currentRenderCrop];

	float wRatio = (float)rc->width / SCREEN_WIDTH;
	float hRatio = (float)rc->height / SCREEN_HEIGHT;

	viewport->x1 = idMath::Ftoi( rc->x + renderView->x * wRatio );
	viewport->x2 = idMath::Ftoi( rc->x + floor( ( renderView->x + renderView->width ) * wRatio + 0.5f ) - 1 );
	viewport->y1 = idMath::Ftoi( ( rc->y + rc->height ) - floor( ( renderView->y + renderView->height ) * hRatio + 0.5f ) );
	viewport->y2 = idMath::Ftoi( ( rc->y + rc->height ) - floor( renderView->y * hRatio + 0.5f ) - 1 );
}

static int RoundDownToPowerOfTwo( int v ) {
	int	i;

	for ( i = 0 ; i < 20 ; i++ ) {
		if ( ( 1 << i ) == v ) {
			return v;
		}
		if ( ( 1 << i ) > v ) {
			return 1 << ( i-1 );
		}
	}
	return 1<<i;
}

/*
================
CropRenderSize

This automatically halves sizes until it fits in the current window size,
so if you specify a power of two size for a texture copy, it may be shrunk
down, but still valid.
================
*/
void	idRenderSystemLocal::CropRenderSize( int width, int height, bool makePowerOfTwo, bool forceDimensions ) {
	if ( !glConfig.isInitialized ) {
		return;
	}

	// close any gui drawing before changing the size
	guiModel->EmitFullScreen();
	guiModel->Clear();

	if ( width < 1 || height < 1 ) {
		common->Error( "CropRenderSize: bad sizes" );
	}

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_CROP_RENDER );
		session->writeDemo->WriteInt( width );
		session->writeDemo->WriteInt( height );
		session->writeDemo->WriteInt( makePowerOfTwo );

		if ( r_showDemo.GetBool() ) {
			common->Printf( "write DC_CROP_RENDER\n" );
		}
	}

	// convert from virtual SCREEN_WIDTH/SCREEN_HEIGHT coordinates to physical OpenGL pixels
	renderView_t renderView;
	renderView.x = 0;
	renderView.y = 0;
	renderView.width = width;
	renderView.height = height;

	idScreenRect	r;
	RenderViewToViewport( &renderView, &r );

	width = r.x2 - r.x1 + 1;
	height = r.y2 - r.y1 + 1;

	if ( forceDimensions ) {
		// just give exactly what we ask for
		width = renderView.width;
		height = renderView.height;
	}

	// if makePowerOfTwo, drop to next lower power of two after scaling to physical pixels
	if ( makePowerOfTwo ) {
		width = RoundDownToPowerOfTwo( width );
		height = RoundDownToPowerOfTwo( height );
		// FIXME: megascreenshots with offset viewports don't work right with this yet
	}

	renderCrop_t	*rc = &renderCrops[currentRenderCrop];

	// we might want to clip these to the crop window instead
	while ( width > glConfig.vidWidth ) {
		width >>= 1;
	}
	while ( height > glConfig.vidHeight ) {
		height >>= 1;
	}

	if ( currentRenderCrop == MAX_RENDER_CROPS ) {
		common->Error( "idRenderSystemLocal::CropRenderSize: currentRenderCrop == MAX_RENDER_CROPS" );
	}

	currentRenderCrop++;

	rc = &renderCrops[currentRenderCrop];

	rc->x = 0;
	rc->y = 0;
	rc->width = width;
	rc->height = height;
}

/*
================
UnCrop
================
*/
void idRenderSystemLocal::UnCrop() {
	if ( !glConfig.isInitialized ) {
		return;
	}

	if ( currentRenderCrop < 1 ) {
		common->Error( "idRenderSystemLocal::UnCrop: currentRenderCrop < 1" );
	}

	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();

	currentRenderCrop--;

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_UNCROP_RENDER );

		if ( r_showDemo.GetBool() ) {
			common->Printf( "write DC_UNCROP\n" );
		}
	}
}

/*
================
CaptureRenderToImage
================
*/
void idRenderSystemLocal::CaptureRenderToImage( const char *imageName ) {
	if ( !glConfig.isInitialized ) {
		return;
	}

	guiModel->EmitFullScreen();
	guiModel->Clear();

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_CAPTURE_RENDER );
		session->writeDemo->WriteHashString( imageName );

		if ( r_showDemo.GetBool() ) {
			common->Printf( "write DC_CAPTURE_RENDER: %s\n", imageName );
		}
	}

	// look up the image before we create the render command, because it
	// may need to sync to create the image
	idImage	*image = globalImages->ImageFromFile(imageName, TF_DEFAULT, true, TR_REPEAT, TD_DEFAULT);

	renderCrop_t *rc = &renderCrops[currentRenderCrop];

	copyRenderCommand_t *cmd = (copyRenderCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_COPY_RENDER;
	cmd->x = rc->x;
	cmd->y = rc->y;
	cmd->imageWidth = rc->width;
	cmd->imageHeight = rc->height;
	cmd->image = image;


	guiModel->Clear();
}

/*
==============
CaptureRenderToFile

==============
*/
void idRenderSystemLocal::CaptureRenderToFile( const char *fileName, bool fixAlpha ) {
	if ( !glConfig.isInitialized ) {
		return;
	}

	renderCrop_t *rc = &renderCrops[currentRenderCrop];

	guiModel->EmitFullScreen();
	guiModel->Clear();

	// include extra space for OpenGL padding to word boundaries
	int	c = ( rc->width + 4 ) * rc->height;
	byte *data = (byte *)R_StaticAlloc( c * 4 );

	// This will render the commands and will block untill finished and has the pixel data
	RenderCommands(rc, data);

	byte *data2 = (byte *)R_StaticAlloc( c * 4 );

	for ( int i = 0 ; i < c ; i++ ) {
		data2[ i * 4 ] = data[ i * 4 ];
		data2[ i * 4 + 1 ] = data[ i * 4 + 1 ];
		data2[ i * 4 + 2 ] = data[ i * 4 + 2 ];
		data2[ i * 4 + 3 ] = 0xff;
	}

	R_WriteTGA( fileName, data2, rc->width, rc->height, true );

	R_StaticFree( data );
	R_StaticFree( data2 );
}


/*
==============
AllocRenderWorld
==============
*/
idRenderWorld *idRenderSystemLocal::AllocRenderWorld() {
	idRenderWorldLocal *rw;
	rw = new idRenderWorldLocal;
	worlds.Append( rw );
	return rw;
}

/*
==============
FreeRenderWorld
==============
*/
void idRenderSystemLocal::FreeRenderWorld( idRenderWorld *rw ) {
	if ( primaryWorld == rw ) {
		primaryWorld = NULL;
	}
	worlds.Remove( static_cast<idRenderWorldLocal *>(rw) );
	delete rw;
}

/*
==============
PrintMemInfo
==============
*/
void idRenderSystemLocal::PrintMemInfo( MemInfo_t *mi ) {
	// sum up image totals
	globalImages->PrintMemInfo( mi );

	// sum up model totals
	renderModelManager->PrintMemInfo( mi );

	// compute render totals

}

/*
===============
idRenderSystemLocal::UploadImage
===============
*/
bool idRenderSystemLocal::UploadImage( const char *imageName, const byte *data, int width, int height  ) {
	idImage *image = globalImages->GetImage( imageName );
	if ( !image ) {
		return false;
	}
	image->UploadScratch( data, width, height );
	image->SetImageFilterAndRepeat();
	return true;
}

void idRenderSystemLocal::DirectFrameBufferStart()
{
	R_DirectFrameBufferStart();
}

void idRenderSystemLocal::DirectFrameBufferEnd()
{
	R_DirectFrameBufferEnd();
}

