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

#ifndef __GAME_WEAPON_H__
#define __GAME_WEAPON_H__

#include "script/Script_Thread.h"
#include "Entity.h"
#include "Light.h"
#include "Actor.h"
#include "Misc.h"
#include "Grabber.h"

class idGrabber;

/*
===============================================================================

	Player Weapon

===============================================================================
*/

extern const idEventDef EV_Weapon_State;

typedef enum {
	WP_READY,
	WP_OUTOFAMMO,
	WP_RELOAD,
	WP_HOLSTERED,
	WP_RISING,
	WP_LOWERING
} weaponStatus_t;

typedef enum { // Koz weapon enumerations
	WEAPON_NONE = -1,
	WEAPON_FISTS,
	WEAPON_CHAINSAW,
	WEAPON_PISTOL,
	WEAPON_SHOTGUN,
	WEAPON_MACHINEGUN,
	WEAPON_CHAINGUN,
	WEAPON_HANDGRENADE,
	WEAPON_PLASMAGUN,
	WEAPON_ROCKETLAUNCHER,
	WEAPON_BFG,
	WEAPON_SOULCUBE,
	WEAPON_SHOTGUN_DOUBLE,
	WEAPON_SHOTGUN_DOUBLE_MP,
	WEAPON_GRABBER,
	WEAPON_ARTIFACT,
	WEAPON_PDA,
	WEAPON_FLASHLIGHT,
	WEAPON_NUM_WEAPONS
} weapon_t;

// Koz flashlightOffsets - values are used to move flashlight model to 'mount' to the active weapon.  Hacky McCrappyHack was here.
const idVec3 flashlightOffsets[int(WEAPON_NUM_WEAPONS)] = {	idVec3( 0.0f, 0.0f, 0.0f ),			// WEAPON_FISTS
															   idVec3( 0.0f, 0.0f, 0.0f ),			// WEAPON_CHAINSAW
															   idVec3( -1.25f, -6.5f, 0.9f ),		// WEAPON_PISTOL
															   idVec3( -1.75f, -3.5f, 1.15f ),		// WEAPON_SHOTGUN
															   idVec3( -2.2f, -7.5f, 1.2f ),		// WEAPON_MACHINEGUN
															   idVec3( -2.3f, -11.25f, -4.5f ),	// WEAPON_CHAINGUN
															   idVec3( 0.0f, 0.0f, 0.0f ),			// WEAPON_HANDGRENADE
															   idVec3( -3.0f, -6.5f, 1.65f ),		// WEAPON_PLASMAGUN
															   idVec3( 4.4f, -14.5f, -3.5f ),		// WEAPON_ROCKETLAUNCHER
															   idVec3( -0.5f, -6.0f, 6.9f ),		// WEAPON_BFG
															   idVec3( 0.0f, 0.0f, 0.0f ),			// WEAPON_SOULCUBE
															   idVec3( -0.5f, -9.5f, -2.0f ),		// WEAPON_SHOTGUN_DOUBLE
															   idVec3( -0.5f, -9.0f, -2.0f ),		// WEAPON_SHOTGUN_DOUBLE_MP
															   idVec3( -4.25f, 6.0, 1.25f ),		// WEAPON_GRABBER
															   idVec3( 0.0f, 0.0f, 0.0f ),			// WEAPON_ARTIFACT
															   idVec3( 0.0f, 0.0f, 0.0f )			// WEAPON_PDA
};

typedef int ammo_t;
static const int AMMO_NUMTYPES = 16;

class idPlayer;

static const int LIGHTID_WORLD_MUZZLE_FLASH = 1;
static const int LIGHTID_VIEW_MUZZLE_FLASH = 100;

class idMoveableItem;

typedef struct
{
    char			name[64];
    char			particlename[128];
    bool			active;
    int				startTime;
    jointHandle_t	joint;			//The joint on which to attach the particle
    bool			smoke;			//Is this a smoke particle
    const idDeclParticle* particle;		//Used for smoke particles
    idFuncEmitter*  emitter;		//Used for non-smoke particles
} WeaponParticle_t;

typedef struct
{
    char			name[64];
    bool			active;
    int				startTime;
    jointHandle_t	joint;
    int				lightHandle;
    renderLight_t	light;
} WeaponLight_t;

class idWeapon : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( idWeapon );

							idWeapon();
	virtual					~idWeapon();

	weaponStatus_t			status;

	// Init
	void					Spawn( void );
    void					SetOwner( idPlayer* owner, int ownerHand );
	idPlayer*				GetOwner( void );
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
    void					SetFlashlightOwner( idPlayer* owner );
    int						GetHand();

	static void				CacheWeapon( const char *weaponName );

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	// Weapon definition management
	void					Clear( void );
	void					GetWeaponDef( const char *objectname, int ammoinclip );
	bool					IsLinked( void );
	bool					IsWorldModelReady( void );
    weapon_t				IdentifyWeapon(); // Koz

	// GUIs
	const char *			Icon( void ) const;
	void					UpdateGUI( void );
    void					UpdateVRGUI();

    virtual void			SetModel( const char *modelname );
	bool					GetGlobalJointTransform( bool viewModel, const jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis );
	void					SetPushVelocity( const idVec3 &pushVelocity );
	bool					UpdateSkin( void );

	// State control/player interface
	void					Think( void );
	void					Raise( void );
	void					PutAway( void );
	void					Reload( void );
	void					LowerWeapon( void );
	void					RaiseWeapon( void );
	void					HideWeapon( void );
	void					ShowWeapon( void );
	void					HideWorldModel( void );
	void					ShowWorldModel( void );
	void					OwnerDied( void );
	void					BeginAttack( void );
	void					EndAttack( void );
	bool					IsReady( void ) const;
	bool					IsReloading( void ) const;
	bool					IsHolstered( void ) const;
	bool					ShowCrosshair( void ) const;
	idEntity *				DropItem( const idVec3 &velocity, int activateDelay, int removeDelay, bool died );
	bool					CanDrop( void ) const;
	void					WeaponStolen( void );
    void					ForceAmmoInClip();
    weaponStatus_t			GetStatus()
    {
        return status;
    };
	// Script state management
	virtual idThread *		ConstructScriptObject( void );
	virtual void			DeconstructScriptObject( void );
	void					SetState( const char *statename, int blendFrames );
	void					UpdateScript( void );
	void					EnterCinematic( void );
	void					ExitCinematic( void );
	void					NetCatchup( void );

	// Visual presentation
    void					PresentWeapon( bool showViewModel, int hand );
	int						GetZoomFov( void );
	void					GetWeaponAngleOffsets( int *average, float *scale, float *max );
	void					GetWeaponTimeOffsets( float *time, float *scale );
	bool					BloodSplat( float size );
    void					SetIsPlayerFlashlight( bool bl )
    {
        isPlayerFlashlight = bl;
    }
    void					FlashlightOn();
    void					FlashlightOff();

	// Ammo
	static ammo_t			GetAmmoNumForName( const char *ammoname );
	static const char		*GetAmmoNameForNum( ammo_t ammonum );
	static const char		*GetAmmoPickupNameForNum( ammo_t ammonum );
	ammo_t					GetAmmoType( void ) const;
	int						AmmoAvailable( void ) const;
	int						AmmoInClip( void ) const;
	void					ResetAmmoClip( void );
	int						ClipSize( void ) const;
	int						LowAmmo( void ) const;
	int						AmmoRequired( void ) const;
    int						AmmoCount() const;
	int						GetGrabberState() const;

    // Flashlight
    idAnimatedEntity* 		GetWorldModel()
    {
        return worldModel.GetEntity();
    }

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	enum {
		EVENT_RELOAD = idEntity::EVENT_MAXEVENTS,
		EVENT_ENDRELOAD,
		EVENT_CHANGESKIN,
		EVENT_MAXEVENTS
	};
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	virtual void			ClientPredictionThink( void );
    void					MuzzleFlashLight();
    void					RemoveMuzzleFlashlight();

    // Get a global origin and axis suitable for the laser sight or bullet tracing
    // Returns false for hands, grenades, and chainsaw.
    // Can't be const because a frame may need to be created.
    bool					GetMuzzlePositionWithHacks( idVec3& origin, idMat3& axis );

	void					GetProjectileLaunchOriginAndAxis( idVec3& origin, idMat3& axis );

    const idDeclEntityDef* GetDeclEntityDef()
    {
        return weaponDef;
    }

    friend class idPlayer;
    friend class idWeaponHolder;
    friend class idHolster;
    friend class idPlayerHand;
	friend class idGrabber;

private:
	// script control
	idScriptBool			WEAPON_ATTACK;
	idScriptBool			WEAPON_RELOAD;
	idScriptBool			WEAPON_NETRELOAD;
	idScriptBool			WEAPON_NETENDRELOAD;
	idScriptBool			WEAPON_NETFIRING;
	idScriptBool			WEAPON_RAISEWEAPON;
	idScriptBool			WEAPON_LOWERWEAPON;

	idThread *				thread;
	idStr					state;
	idStr					idealState;
	int						animBlendFrames;
	int						animDoneTime;
	bool					isLinked;
    bool					isPlayerFlashlight;
    bool					isPlayerLeftHand;

    int lastIdentifiedFrame = 0;
    weapon_t currentIdentifiedWeapon = WEAPON_NONE;
    weapon_t lastIdentifiedWeapon = WEAPON_NONE; // lastweapon holds the last actual weapon value, so the weapon enum will never return a value of 'weapon_flaslight'. nothing to do with the players previous weapon

    // precreated projectile
	idEntity				*projectileEnt;

	idPlayer *				owner;
	idEntityPtr<idAnimatedEntity>	worldModel;
    int						hand;

	// hiding (for GUIs and NPCs)
	int						hideTime;
	float					hideDistance;
	int						hideStartTime;
	float					hideStart;
	float					hideEnd;
	float					hideOffset;
	bool					hide;
	bool					disabled;

	// berserk
	int						berserk;

	// these are the player render view parms, which include bobbing
	idVec3					playerViewOrigin;
	idMat3					playerViewAxis;

	// the view weapon render entity parms
	idVec3					viewWeaponOrigin;
	idAngles				viewWeaponAngles;
	idMat3					viewWeaponAxis;

	// the muzzle bone's position, used for launching projectiles and trailing smoke
	idVec3					muzzleOrigin;
	idMat3					muzzleAxis;

	idVec3					pushVelocity;

	// weapon definition
	// we maintain local copies of the projectile and brass dictionaries so they
	// do not have to be copied across the DLL boundary when entities are spawned
	const idDeclEntityDef *	weaponDef;
	const idDeclEntityDef *	meleeDef;
	idDict					projectileDict;
	float					meleeDistance;
	idStr					meleeDefName;
	idDict					brassDict;
	int						brassDelay;
	idStr					icon;

	// view weapon gui light
	renderLight_t			guiLight;
	int						guiLightHandle;

    // Koz begin
    // VR Stat Gui - this is the 'watch' the player wears in VR to display stats.
    class idUserInterface*	lvrStatGui;
    // Koz end

	// muzzle flash
	renderLight_t			muzzleFlash;		// positioned on view weapon bone
	int						muzzleFlashHandle;

	renderLight_t			worldMuzzleFlash;	// positioned on world weapon bone
	int						worldMuzzleFlashHandle;

    float					fraccos;
    float					fraccos2;

	idVec3					flashColor;
	int						muzzleFlashEnd;
	int						flashTime;
	bool					lightOn;
	bool					silent_fire;
	bool					allowDrop;

	// effects
	bool					hasBloodSplat;

	// weapon kick
	int						kick_endtime;
	int						muzzle_kick_time;
	int						muzzle_kick_maxtime;
	idAngles				muzzle_kick_angles;
	idVec3					muzzle_kick_offset;

	// ammo management
	ammo_t					ammoType;
	int						ammoRequired;		// amount of ammo to use each shot.  0 means weapon doesn't need ammo.
	int						clipSize;			// 0 means no reload
	int						ammoClip;
	int						lowAmmo;			// if ammo in clip hits this threshold, snd_
	bool					powerAmmo;			// true if the clip reduction is a factor of the power setting when
												// a projectile is launched
	// mp client
	bool					isFiring;

	// zoom
	int						zoomFov;			// variable zoom fov per weapon

	// joints from models
	jointHandle_t			barrelJointView;
	jointHandle_t			flashJointView;
	jointHandle_t			ejectJointView;
	jointHandle_t			guiLightJointView;
	jointHandle_t			ventLightJointView;

	jointHandle_t			flashJointWorld;
	jointHandle_t			barrelJointWorld;
	jointHandle_t			ejectJointWorld;

    jointHandle_t			smokeJointView;

    // Koz
    jointHandle_t			weaponHandAttacher[2];
    idVec3					weaponHandDefaultPos[2];
    idMat3					weaponHandDefaultAxis[2];

    idVec3					laserSightOffset;
    // Koz end

    idHashTable<WeaponParticle_t>	weaponParticles;
    idHashTable<WeaponLight_t>		weaponLights;

	// sound
	const idSoundShader *	sndHum;

	// new style muzzle smokes
	const idDeclParticle *	weaponSmoke;			// null if it doesn't smoke
	int						weaponSmokeStartTime;	// set to gameLocal.time every weapon fire
	bool					continuousSmoke;		// if smoke is continuous ( chainsaw )
	const idDeclParticle *  strikeSmoke;			// striking something in melee
	int						strikeSmokeStartTime;	// timing
	idVec3					strikePos;				// position of last melee strike
	idMat3					strikeAxis;				// axis of last melee strike
	int						nextStrikeFx;			// used for sound and decal ( may use for strike smoke too )

	// nozzle effects
	bool					nozzleFx;			// does this use nozzle effects ( parm5 at rest, parm6 firing )
										// this also assumes a nozzle light atm
	int						nozzleFxFade;		// time it takes to fade between the effects
	int						lastAttack;			// last time an attack occured
	renderLight_t			nozzleGlow;			// nozzle light
	int						nozzleGlowHandle;	// handle for nozzle light

	idVec3					nozzleGlowColor;	// color of the nozzle glow
	const idMaterial *		nozzleGlowShader;	// shader for glow light
	float					nozzleGlowRadius;	// radius of glow light

	// weighting for viewmodel angles
	int						weaponAngleOffsetAverages;
	float					weaponAngleOffsetScale;
	float					weaponAngleOffsetMax;
	float					weaponOffsetTime;
	float					weaponOffsetScale;

	// flashlight
	void					AlertMonsters( void );

	// Visual presentation
	void					InitWorldModel( const idDeclEntityDef *def );
	void					MuzzleRise( idVec3 &origin, idMat3 &axis );
	void					UpdateNozzleFx( void );
	void					UpdateFlashPosition( void );

    void					UpdateWeaponClipPosition( idVec3 &origin, idMat3 &axis ); // Koz

	// script events
	void					Event_Clear( void );
	void					Event_GetOwner( void );
	void					Event_WeaponState( const char *statename, int blendFrames );
	void					Event_SetWeaponStatus( float newStatus );
	void					Event_WeaponReady( void );
	void					Event_WeaponOutOfAmmo( void );
	void					Event_WeaponReloading( void );
	void					Event_WeaponHolstered( void );
	void					Event_WeaponRising( void );
	void					Event_WeaponLowering( void );
	void					Event_UseAmmo( int amount );
	void					Event_AddToClip( int amount );
	void					Event_AmmoInClip( void );
	void					Event_AmmoAvailable( void );
	void					Event_TotalAmmoCount( void );
	void					Event_ClipSize( void );
	void					Event_PlayAnim( int channel, const char *animname );
	void					Event_PlayCycle( int channel, const char *animname );
	void					Event_AnimDone( int channel, int blendFrames );
	void					Event_SetBlendFrames( int channel, int blendFrames );
	void					Event_GetBlendFrames( int channel );
	void					Event_Next( void );
	void					Event_SetSkin( const char *skinname );
	void					Event_Flashlight( int enable );
	void					Event_GetLightParm( int parmnum );
	void					Event_SetLightParm( int parmnum, float value );
	void					Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void					Event_LaunchProjectiles( int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower );
	void					Event_CreateProjectile( void );
	void					Event_EjectBrass( void );
	void					Event_Melee( void );
	void					Event_GetWorldModel( void );
	void					Event_AllowDrop( int allow );
	void					Event_AutoReload( void );
	void					Event_NetReload( void );
	void					Event_IsInvisible( void );
	void					Event_NetEndReload( void );
    // Koz
    void					Event_GetWeaponSkin();
    void					Event_IsMotionControlled();
    void					CalculateHideRise( idVec3& origin, idMat3& axis );
    // Koz end
	idGrabber				grabber;
	int						grabberState;

	void					Event_Grabber( int enable );
	void					Event_GrabberHasTarget();
	void					Event_GrabberSetGrabDistance( float dist );
	void					Event_LaunchProjectilesEllipse(int num_projectiles, float spreada, float spreadb, float fuseOffset, float power);
	void					Event_LaunchPowerup(const char *powerup, float duration, int useAmmo);

	void					Event_StartWeaponSmoke();
	void					Event_StopWeaponSmoke();

	void					Event_StartWeaponParticle(const char *name);
	void					Event_StopWeaponParticle(const char *name);

	void					Event_StartWeaponLight(const char *name);
	void					Event_StopWeaponLight(const char *name);
};

ID_INLINE bool idWeapon::IsLinked( void ) {
	return isLinked;
}

ID_INLINE bool idWeapon::IsWorldModelReady( void ) {
	return ( worldModel.GetEntity() != NULL );
}

ID_INLINE idPlayer* idWeapon::GetOwner( void ) {
	return owner;
}

#endif /* !__GAME_WEAPON_H__ */
