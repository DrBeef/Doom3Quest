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

#include <SDL.h>

#include "sys/platform.h"
#include "framework/Licensee.h"

#include "renderer/tr_local.h"

#include "../../Doom3Quest/VrCommon.h"

#if defined(_WIN32) && defined(ID_ALLOW_TOOLS)
#include "sys/win32/win_local.h"
#include <SDL_syswm.h>
#endif

/*
===================
GLimp_Init
===================
*/
extern "C" void Doom3Quest_GetScreenRes(int *width, int *height);

bool GLimp_Init(glimpParms_t parms) {
	common->Printf("Initializing OpenGL subsystem\n");

	int colorbits = 32;
	int depthbits = 24;
	int stencilbits = 8;

	Doom3Quest_GetScreenRes(&glConfig.vidWidth, &glConfig.vidHeight);
	glConfig.isFullscreen = true;

#if defined(_WIN32) && defined(ID_ALLOW_TOOLS)

#ifndef SDL_VERSION_ATLEAST(2, 0, 0)
#error "dhewm3 only supports the tools with SDL2, not SDL1!"
#endif

	// The tools are Win32 specific.  If building the tools
	// then we know we are win32 and we have to include this
	// config to get the editors to work.

	// Get the HWND for later use.
	SDL_SysWMinfo sdlinfo;
	SDL_version sdlver;
	SDL_VERSION(&sdlver);
	sdlinfo.version = sdlver;
	if (SDL_GetWindowWMInfo(window, &sdlinfo) && sdlinfo.subsystem == SDL_SYSWM_WINDOWS) {
		win32.hWnd = sdlinfo.info.win.window;
		win32.hDC = sdlinfo.info.win.hdc;
		// NOTE: hInstance is set in main()
		win32.hGLRC = qwglGetCurrentContext();

		PIXELFORMATDESCRIPTOR src =
		{
			sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
			1,								// version number
			PFD_DRAW_TO_WINDOW |			// support window
			PFD_SUPPORT_OPENGL |			// support OpenGL
			PFD_DOUBLEBUFFER,				// double buffered
			PFD_TYPE_RGBA,					// RGBA type
			32,								// 32-bit color depth
			0, 0, 0, 0, 0, 0,				// color bits ignored
			8,								// 8 bit destination alpha
			0,								// shift bit ignored
			0,								// no accumulation buffer
			0, 0, 0, 0, 					// accum bits ignored
			24,								// 24-bit z-buffer
			8,								// 8-bit stencil buffer
			0,								// no auxiliary buffer
			PFD_MAIN_PLANE,					// main layer
			0,								// reserved
			0, 0, 0							// layer masks ignored
		};
		memcpy(&win32.pfd, &src, sizeof(PIXELFORMATDESCRIPTOR));
	} else {
		// TODO: can we just disable them?
		common->Error("SDL_GetWindowWMInfo(), which is needed for Tools to work, failed!");
	}
#endif // defined(_WIN32) && defined(ID_ALLOW_TOOLS)

	common->Printf("Using %d color bits, %d depth, %d stencil display\n",
				   colorbits, depthbits, stencilbits);

	glConfig.colorBits = colorbits;
	glConfig.depthBits = depthbits;
	glConfig.stencilBits = stencilbits;

	glConfig.displayFrequency = 0;



	GLimp_WindowActive(true);

	return true;
}

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms(glimpParms_t parms) {
	return true;
}

/*
===================
GLimp_Shutdown
===================
*/
void GLimp_Shutdown() {
}

void GLimp_SetupFrame(int buffer /*unused*/) {

	Doom3Quest_processMessageQueue();

	Doom3Quest_prepareEyeBuffer();
}


/*
===================
GLimp_SwapBuffers
===================
*/
void GLimp_SwapBuffers() {
	Doom3Quest_finishEyeBuffer();

	//We can now submit the stereo frame
	Doom3Quest_submitFrame();
}

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma(unsigned short red[256], unsigned short green[256], unsigned short blue[256]) {

}

/*
=================
GLimp_ActivateContext
=================
*/

extern "C" void ActivateContext();
void GLimp_ActivateContext() {
	ActivateContext();
}

/*
=================
GLimp_DeactivateContext
=================
*/
extern "C" void DeactivateContext();
void GLimp_DeactivateContext() {
    DeactivateContext();
}

/*
===================
GLimp_ExtensionPointer
===================
*/
#ifdef __ANDROID__
#include <dlfcn.h>
#endif
GLExtension_t GLimp_ExtensionPointer(const char *name) {
#ifdef __ANDROID__
	static void *glesLib = NULL;

	if( !glesLib )
	{
	    int flags = RTLD_LOCAL | RTLD_NOW;
		//glesLib = dlopen("libGLESv2_CM.so", flags);
		glesLib = dlopen("libGLESv3.so", flags);
		if( !glesLib )
		{
			glesLib = dlopen("libGLESv2.so", flags);
		}
	}

	GLExtension_t ret =  (GLExtension_t)dlsym(glesLib, name);
	//common->Printf("GLimp_ExtensionPointer %s  %p\n",name,ret);
	return ret;
#endif
}

void GLimp_WindowActive(bool active)
{
	LOGI( "GLimp_WindowActive %d", active );

	tr.windowActive = active;

	if(!active)
	{
		tr.BackendThreadShutdown();
	}
}

void GLimp_GrabInput(int flags) {
}
