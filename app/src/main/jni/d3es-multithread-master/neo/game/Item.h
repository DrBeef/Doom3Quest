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

#ifndef __GAME_ITEM_H__
#define __GAME_ITEM_H__

#include "physics/Physics_RigidBody.h"
#include "Entity.h"

/*
===============================================================================

  Items the player can pick up or use.

===============================================================================
*/

class idItem : public idEntity {
public:
	CLASS_PROTOTYPE( idItem );

							idItem();
	virtual					~idItem();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	void					GetAttributes( idDict &attributes );
	virtual bool			GiveToPlayer( idPlayer *player );
	virtual bool			Pickup( idPlayer *player );
	virtual void			Think( void );
	virtual void			Present();

	enum {
		EVENT_PICKUP = idEntity::EVENT_MAXEVENTS,
		EVENT_RESPAWN,
		EVENT_RESPAWNFX,
		EVENT_MAXEVENTS
	};

	virtual void			ClientPredictionThink( void );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	// networking
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	idVec3					orgOrigin;
	bool					spin;
	bool					pulse;
	bool					canPickUp;

	// for item pulse effect
	int						itemShellHandle;
	const idMaterial *		shellMaterial;

	// used to update the item pulse effect
	mutable bool			inView;
	mutable int				inViewTime;
	mutable int				lastCycle;
	mutable int				lastRenderViewTime;

	bool					UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const;
	static bool				ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );

	void					Event_DropToFloor( void );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Event_Trigger( idEntity *activator );
	void					Event_Respawn( void );
	void					Event_RespawnFx( void );
};

class idItemPowerup : public idItem {
public:
	CLASS_PROTOTYPE( idItemPowerup );

							idItemPowerup();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();
	virtual bool			GiveToPlayer( idPlayer *player );

private:
	int						time;
	int						type;
};

class idObjective : public idItem {
public:
	CLASS_PROTOTYPE( idObjective );

							idObjective();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();

private:
	idVec3					playerPos;

	void					Event_Trigger( idEntity *activator );
	void					Event_HideObjective( idEntity *e );
	void					Event_GetPlayerPos();
	void					Event_CamShot();
};

class idVideoCDItem : public idItem {
public:
	CLASS_PROTOTYPE( idVideoCDItem );

	void					Spawn();
	virtual bool			GiveToPlayer( idPlayer *player );
};

class idPDAItem : public idItem {
public:
	CLASS_PROTOTYPE( idPDAItem );

	virtual bool			GiveToPlayer( idPlayer *player );
};

class idMoveableItem : public idItem {
public:
	CLASS_PROTOTYPE( idMoveableItem );

							idMoveableItem();
	virtual					~idMoveableItem();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual bool			Pickup( idPlayer *player );

	static void				DropItems( idAnimatedEntity *ent, const char *type, idList<idEntity *> *list );
	static idEntity	*		DropItem( const char *classname, const idVec3 &origin, const idMat3 &axis, const idVec3 &velocity, int activateDelay, int removeDelay );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	idPhysics_RigidBody		physicsObj;
	idClipModel *			trigger;
	const idDeclParticle *	smoke;
	int						smokeTime;
	int						nextSoundTime;

	void					Gib( const idVec3 &dir, const char *damageDefName );

	void					Event_DropToFloor( void );
	void					Event_Gib( const char *damageDefName );
};

class idMoveablePDAItem : public idMoveableItem {
public:
	CLASS_PROTOTYPE( idMoveablePDAItem );

	virtual bool			GiveToPlayer( idPlayer *player );
};

/*
===============================================================================

  Item removers.

===============================================================================
*/

class idItemRemover : public idEntity {
public:
	CLASS_PROTOTYPE( idItemRemover );

	void					Spawn();
	void					RemoveItem( idPlayer *player );

private:
	void					Event_Trigger( idEntity *activator );
};

class idObjectiveComplete : public idItemRemover {
public:
	CLASS_PROTOTYPE( idObjectiveComplete );

							idObjectiveComplete();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();

private:
	idVec3					playerPos;

	void					Event_Trigger( idEntity *activator );
	void					Event_HideObjective( idEntity *e );
	void					Event_GetPlayerPos();
};

#endif /* !__GAME_ITEM_H__ */
