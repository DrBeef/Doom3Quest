/*
 Simple DirectMedia Layer
 Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>
 
 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.
 
 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:
 
 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
 */

/**
 *  \defgroup Compatibility SDL 1.2 Compatibility API
 */
/*@{*/

/**
 *  \file SDL_compat_events.h
 *
 *  This file contains events for backwards compatibility with SDL 1.2.
 */

/*@}*/

#ifndef _SDL_compat_events_h
#define _SDL_compat_events_h

enum {
    SDL_EVENT_COMPAT1 = 0x7000,
    SDL_EVENT_COMPAT2,
    SDL_EVENT_COMPAT3,
};

typedef struct SDL_ActiveEvent
{
    Uint32 type;
    Uint32 timestamp;
    Uint8 gain;
    Uint8 state;
} SDL_ActiveEvent;

typedef struct SDL_ResizeEvent
{
    Uint32 type;
    Uint32 timestamp;
    int w;
    int h;
} SDL_ResizeEvent;

#define SDL_EVENT_GET_ACTIVE(event_ptr) ((SDL_ActiveEvent*)(event_ptr))
#define SDL_EVENT_GET_RESIZE(event_ptr) ((SDL_ResizeEvent*)(event_ptr))

#endif /* _SDL_compat_events_h */

/* vi: set ts=4 sw=4 expandtab: */