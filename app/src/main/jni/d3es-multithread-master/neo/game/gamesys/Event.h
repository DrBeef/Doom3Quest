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

#ifndef __SYS_EVENT_H__
#define __SYS_EVENT_H__

#include "idlib/containers/LinkList.h"
#include "cm/CollisionModel.h"

// Event are used for scheduling tasks and for linking script commands.

#define D_EVENT_MAXARGS				8			// if changed, enable the CREATE_EVENT_CODE define in Event.cpp to generate switch statement for idClass::ProcessEventArgPtr.
												// running the game will then generate c:\doom\base\events.txt, the contents of which should be copied into the switch statement.

// stack size of idVec3, aligned to native pointer size
#define E_EVENT_SIZEOF_VEC			((sizeof(idVec3) + (sizeof(intptr_t) - 1)) & ~(sizeof(intptr_t) - 1))

#define D_EVENT_VOID				( ( char )0 )
#define D_EVENT_INTEGER				'd'
#define D_EVENT_FLOAT				'f'
#define D_EVENT_VECTOR				'v'
#define D_EVENT_STRING				's'
#define D_EVENT_ENTITY				'e'
#define	D_EVENT_ENTITY_NULL			'E'			// event can handle NULL entity pointers
#define D_EVENT_TRACE				't'

#define MAX_EVENTS					4096

class idClass;
class idTypeInfo;

class idEventDef {
private:
	const char					*name;
	const char					*formatspec;
	unsigned int				formatspecIndex;
	int							returnType;
	int							numargs;
	size_t						argsize;
	int							argOffset[ D_EVENT_MAXARGS ];
	int							eventnum;
	const idEventDef *			next;

	static idEventDef *			eventDefList[MAX_EVENTS];
	static int					numEventDefs;

public:
								idEventDef( const char *command, const char *formatspec = NULL, char returnType = 0 );

	const char					*GetName( void ) const;
	const char					*GetArgFormat( void ) const;
	unsigned int				GetFormatspecIndex( void ) const;
	char						GetReturnType( void ) const;
	int							GetEventNum( void ) const;
	int							GetNumArgs( void ) const;
	size_t						GetArgSize( void ) const;
	int							GetArgOffset( int arg ) const;

	static int					NumEventCommands( void );
	static const idEventDef		*GetEventCommand( int eventnum );
	static const idEventDef		*FindEvent( const char *name );
};

class idSaveGame;
class idRestoreGame;

class idEvent {
private:
	const idEventDef			*eventdef;
	byte						*data;
	int							time;
	idClass						*object;
	const idTypeInfo			*typeinfo;

	idLinkList<idEvent>			eventNode;

	static idDynamicBlockAlloc<byte, 16 * 1024, 256> eventDataAllocator;


public:
	static bool					initialized;

								~idEvent();

	static idEvent				*Alloc( const idEventDef *evdef, int numargs, va_list args );
	static void					CopyArgs( const idEventDef *evdef, int numargs, va_list args, intptr_t data[ D_EVENT_MAXARGS ]  );

	void						Free( void );
	void						Schedule( idClass *object, const idTypeInfo *cls, int time );
	byte						*GetData( void );

	static void					CancelEvents( const idClass *obj, const idEventDef *evdef = NULL );
	static void					ClearEventList( void );
	static void					ServiceEvents( void );
	static void					ServiceFastEvents();
	static void					Init( void );
	static void					Shutdown( void );

	// save games
	static void					Save( idSaveGame *savefile );					// archives object for save game file
	static void					Restore( idRestoreGame *savefile );				// unarchives object from save game file
	static void					SaveTrace( idSaveGame *savefile, const trace_t &trace );
	static void					RestoreTrace( idRestoreGame *savefile, trace_t &trace );

};

/*
================
idEvent::GetData
================
*/
ID_INLINE byte *idEvent::GetData( void ) {
	return data;
}

/*
================
idEventDef::GetName
================
*/
ID_INLINE const char *idEventDef::GetName( void ) const {
	return name;
}

/*
================
idEventDef::GetArgFormat
================
*/
ID_INLINE const char *idEventDef::GetArgFormat( void ) const {
	return formatspec;
}

/*
================
idEventDef::GetFormatspecIndex
================
*/
ID_INLINE unsigned int idEventDef::GetFormatspecIndex( void ) const {
	return formatspecIndex;
}

/*
================
idEventDef::GetReturnType
================
*/
ID_INLINE char idEventDef::GetReturnType( void ) const {
	return returnType;
}

/*
================
idEventDef::GetNumArgs
================
*/
ID_INLINE int idEventDef::GetNumArgs( void ) const {
	return numargs;
}

/*
================
idEventDef::GetArgSize
================
*/
ID_INLINE size_t idEventDef::GetArgSize( void ) const {
	return argsize;
}

/*
================
idEventDef::GetArgOffset
================
*/
ID_INLINE int idEventDef::GetArgOffset( int arg ) const {
	assert( ( arg >= 0 ) && ( arg < D_EVENT_MAXARGS ) );
	return argOffset[ arg ];
}

/*
================
idEventDef::GetEventNum
================
*/
ID_INLINE int idEventDef::GetEventNum( void ) const {
	return eventnum;
}

#endif /* !__SYS_EVENT_H__ */
