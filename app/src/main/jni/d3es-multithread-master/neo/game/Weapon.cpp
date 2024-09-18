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

#include <idlib/containers/Sort.h>
#include "sys/platform.h"
#include "framework/DeclEntityDef.h"
#include "framework/DeclSkin.h"
#include "framework/Session.h"

#include "gamesys/SysCvar.h"
#include "ai/AI.h"
#include "Player.h"
#include "Trigger.h"
#include "SmokeParticles.h"
#include "WorldSpawn.h"

#include "Weapon.h"
#include "Vr.h"

extern bool IsDoom3DemoVersion(); // DG: hack to support the Demo version of Doom3

// Koz begin
idClipModel* PDAclipModel; // Koz fixme pda more crappy globals.

idCVar vr_guiH( "vr_guiH", "100", CVAR_INTEGER, "" );
idCVar vr_guiA( "vr_guiA", "100", CVAR_INTEGER, "" );

idCVar vr_throwPower( "vr_throwPower", "3.4", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Throw power" );
// Koz end

/***********************************************************************

  idWeapon

***********************************************************************/

//
// event defs
//
const idEventDef EV_Weapon_Clear( "<clear>" );
const idEventDef EV_Weapon_GetOwner( "getOwner", NULL, 'e' );
const idEventDef EV_Weapon_Next( "nextWeapon" );
const idEventDef EV_Weapon_State( "weaponState", "sd" );
const idEventDef EV_Weapon_UseAmmo( "useAmmo", "d" );
const idEventDef EV_Weapon_AddToClip( "addToClip", "d" );
const idEventDef EV_Weapon_AmmoInClip( "ammoInClip", NULL, 'f' );
const idEventDef EV_Weapon_AmmoAvailable( "ammoAvailable", NULL, 'f' );
const idEventDef EV_Weapon_TotalAmmoCount( "totalAmmoCount", NULL, 'f' );
const idEventDef EV_Weapon_ClipSize( "clipSize", NULL, 'f' );
const idEventDef EV_Weapon_WeaponOutOfAmmo( "weaponOutOfAmmo" );
const idEventDef EV_Weapon_WeaponReady( "weaponReady" );
const idEventDef EV_Weapon_WeaponReloading( "weaponReloading" );
const idEventDef EV_Weapon_WeaponHolstered( "weaponHolstered" );
const idEventDef EV_Weapon_WeaponRising( "weaponRising" );
const idEventDef EV_Weapon_WeaponLowering( "weaponLowering" );
const idEventDef EV_Weapon_Flashlight( "flashlight", "d" );
const idEventDef EV_Weapon_LaunchProjectiles( "launchProjectiles", "dffff" );
const idEventDef EV_Weapon_CreateProjectile( "createProjectile", NULL, 'e' );
const idEventDef EV_Weapon_EjectBrass( "ejectBrass" );
const idEventDef EV_Weapon_Melee( "melee", NULL, 'd' );
const idEventDef EV_Weapon_GetWorldModel( "getWorldModel", NULL, 'e' );
const idEventDef EV_Weapon_AllowDrop( "allowDrop", "d" );
const idEventDef EV_Weapon_AutoReload( "autoReload", NULL, 'f' );
const idEventDef EV_Weapon_NetReload( "netReload" );
const idEventDef EV_Weapon_IsInvisible( "isInvisible", NULL, 'f' );
const idEventDef EV_Weapon_NetEndReload( "netEndReload" );
const idEventDef EV_Weapon_GrabberHasTarget( "grabberHasTarget", NULL, 'd' );
const idEventDef EV_Weapon_Grabber( "grabber", "d" );
const idEventDef EV_Weapon_Grabber_SetGrabDistance( "grabberGrabDistance", "f" );
// Koz
const idEventDef EV_Weapon_GetWeaponSkin( "getWeaponSkin", NULL, 's' );
const idEventDef EV_Weapon_IsMotionControlled( "isMotionControlled", NULL, 'd' );

//
// class def
//
CLASS_DECLARATION( idAnimatedEntity, idWeapon )
	EVENT( EV_Weapon_Clear,						idWeapon::Event_Clear )
	EVENT( EV_Weapon_GetOwner,					idWeapon::Event_GetOwner )
	EVENT( EV_Weapon_State,						idWeapon::Event_WeaponState )
	EVENT( EV_Weapon_WeaponReady,				idWeapon::Event_WeaponReady )
	EVENT( EV_Weapon_WeaponOutOfAmmo,			idWeapon::Event_WeaponOutOfAmmo )
	EVENT( EV_Weapon_WeaponReloading,			idWeapon::Event_WeaponReloading )
	EVENT( EV_Weapon_WeaponHolstered,			idWeapon::Event_WeaponHolstered )
	EVENT( EV_Weapon_WeaponRising,				idWeapon::Event_WeaponRising )
	EVENT( EV_Weapon_WeaponLowering,			idWeapon::Event_WeaponLowering )
	EVENT( EV_Weapon_UseAmmo,					idWeapon::Event_UseAmmo )
	EVENT( EV_Weapon_AddToClip,					idWeapon::Event_AddToClip )
	EVENT( EV_Weapon_AmmoInClip,				idWeapon::Event_AmmoInClip )
	EVENT( EV_Weapon_AmmoAvailable,				idWeapon::Event_AmmoAvailable )
	EVENT( EV_Weapon_TotalAmmoCount,			idWeapon::Event_TotalAmmoCount )
	EVENT( EV_Weapon_ClipSize,					idWeapon::Event_ClipSize )
	EVENT( AI_PlayAnim,							idWeapon::Event_PlayAnim )
	EVENT( AI_PlayCycle,						idWeapon::Event_PlayCycle )
	EVENT( AI_SetBlendFrames,					idWeapon::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,					idWeapon::Event_GetBlendFrames )
	EVENT( AI_AnimDone,							idWeapon::Event_AnimDone )
	EVENT( EV_Weapon_Next,						idWeapon::Event_Next )
	EVENT( EV_SetSkin,							idWeapon::Event_SetSkin )
	EVENT( EV_Weapon_Flashlight,				idWeapon::Event_Flashlight )
	EVENT( EV_Light_GetLightParm,				idWeapon::Event_GetLightParm )
	EVENT( EV_Light_SetLightParm,				idWeapon::Event_SetLightParm )
	EVENT( EV_Light_SetLightParms,				idWeapon::Event_SetLightParms )
	EVENT( EV_Weapon_LaunchProjectiles,			idWeapon::Event_LaunchProjectiles )
	EVENT( EV_Weapon_CreateProjectile,			idWeapon::Event_CreateProjectile )
	EVENT( EV_Weapon_EjectBrass,				idWeapon::Event_EjectBrass )
	EVENT( EV_Weapon_Melee,						idWeapon::Event_Melee )
	EVENT( EV_Weapon_GetWorldModel,				idWeapon::Event_GetWorldModel )
	EVENT( EV_Weapon_AllowDrop,					idWeapon::Event_AllowDrop )
	EVENT( EV_Weapon_AutoReload,				idWeapon::Event_AutoReload )
	EVENT( EV_Weapon_NetReload,					idWeapon::Event_NetReload )
	EVENT( EV_Weapon_IsInvisible,				idWeapon::Event_IsInvisible )
	EVENT( EV_Weapon_NetEndReload,				idWeapon::Event_NetEndReload )
	EVENT( EV_Weapon_Grabber,					idWeapon::Event_Grabber )
	EVENT( EV_Weapon_GrabberHasTarget,			idWeapon::Event_GrabberHasTarget )
	EVENT( EV_Weapon_Grabber_SetGrabDistance,	idWeapon::Event_GrabberSetGrabDistance )
	EVENT( EV_Weapon_GetWeaponSkin,				idWeapon::Event_GetWeaponSkin )
	EVENT( EV_Weapon_IsMotionControlled,		idWeapon::Event_IsMotionControlled )
END_CLASS

/***********************************************************************

	init

***********************************************************************/

/*
================
idWeapon::idWeapon()
================
*/
idWeapon::idWeapon() {
	owner					= NULL;
	worldModel				= NULL;
	weaponDef				= NULL;
	thread					= NULL;

	memset( &guiLight, 0, sizeof( guiLight ) );
	memset( &muzzleFlash, 0, sizeof( muzzleFlash ) );
	memset( &worldMuzzleFlash, 0, sizeof( worldMuzzleFlash ) );
	memset( &nozzleGlow, 0, sizeof( nozzleGlow ) );

	muzzleFlashEnd			= 0;
	flashColor				= vec3_origin;
	muzzleFlashHandle		= -1;
	worldMuzzleFlashHandle	= -1;
	guiLightHandle			= -1;
	nozzleGlowHandle		= -1;
	modelDefHandle			= -1;
	grabberState = -1;

	berserk					= 2;
	brassDelay				= 0;

	allowDrop				= true;
    isPlayerFlashlight = false;
    isPlayerLeftHand = false; // Koz

    lastIdentifiedFrame = 0;
    currentIdentifiedWeapon = WEAPON_NONE;
    lastIdentifiedWeapon = WEAPON_NONE; // lastweapon holds the last actual weapon value, so the weapon enum will never return a value of 'weapon_flaslight'. nothing to do with the players previous weapon

    fraccos = 0.0f;
    fraccos2 = 0.0f;

    Clear();

	fl.networkSync = true;
}

/*
================
idWeapon::~idWeapon()
================
*/
idWeapon::~idWeapon() {
	Clear();
	delete worldModel.GetEntity();
}


/*
================
idWeapon::Spawn
================
*/
void idWeapon::Spawn( void ) {
	if ( !gameLocal.isClient ) {
		// setup the world model
		worldModel = static_cast< idAnimatedEntity * >( gameLocal.SpawnEntityType( idAnimatedEntity::Type, NULL ) );
		worldModel.GetEntity()->fl.networkSync = true;
	}

	grabber.Initialize();

	thread = new idThread();
	thread->ManualDelete();
	thread->ManualControl();
}

/*
================
idWeapon::SetOwner

Only called at player spawn time, not each weapon switch
================
*/
void idWeapon::SetOwner( idPlayer* _owner, int ownerHand ) {
    assert( !owner );
    owner = _owner;
    hand = ownerHand;
    const char* handNames[ 2 ] = { "right", "left" };
    if( hand < 0 || hand >= 2 || hand == vr_weaponHand.GetInteger() )
    {
        SetName( va( "%s_weapon", owner->name.c_str() ) );
    }
    else
    {
        SetName( va( "%s_weapon_%s", owner->name.c_str(), handNames[ hand ] ) );
    }

    if( worldModel.GetEntity() )
    {
        worldModel.GetEntity()->SetName( va( "%s_worldmodel", name.c_str() ) );
    }
}

/*
================
idWeapon::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idWeapon::ShouldConstructScriptObjectAtSpawn( void ) const {
	return false;
}

/*
================
idWeapon::CacheWeapon
================
*/
void idWeapon::CacheWeapon( const char *weaponName ) {
	const idDeclEntityDef *weaponDef;
	const char *brassDefName;
	const char *clipModelName;
	idTraceModel trm;
	const char *guiName;

	weaponDef = gameLocal.FindEntityDef( weaponName, false );
	if ( !weaponDef ) {
		return;
	}

	// precache the brass collision model
	brassDefName = weaponDef->dict.GetString( "def_ejectBrass" );
	if ( brassDefName[0] ) {
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( brassDefName, false );
		if ( brassDef ) {
			brassDef->dict.GetString( "clipmodel", "", &clipModelName );
			if ( !clipModelName[0] ) {
				clipModelName = brassDef->dict.GetString( "model" );		// use the visual model
			}
			// load the trace model
			collisionModelManager->TrmFromModel( clipModelName, trm );
		}
	}

	guiName = weaponDef->dict.GetString( "gui" );
	if ( guiName[0] ) {
		uiManager->FindGui( guiName, true, false, true );
	}
}

/*
================
idWeapon::Save
================
*/
void idWeapon::Save( idSaveGame *savefile ) const {

	savefile->WriteInt( status );
	savefile->WriteObject( thread );
	savefile->WriteString( state );
	savefile->WriteString( idealState );
	savefile->WriteInt( animBlendFrames );
	savefile->WriteInt( animDoneTime );
	savefile->WriteBool( isLinked );

	savefile->WriteObject( owner );
	worldModel.Save( savefile );

	savefile->WriteInt( hideTime );
	savefile->WriteFloat( hideDistance );
	savefile->WriteInt( hideStartTime );
	savefile->WriteFloat( hideStart );
	savefile->WriteFloat( hideEnd );
	savefile->WriteFloat( hideOffset );
	savefile->WriteBool( hide );
	savefile->WriteBool( disabled );

	savefile->WriteInt( berserk );

	savefile->WriteVec3( playerViewOrigin );
	savefile->WriteMat3( playerViewAxis );

	savefile->WriteVec3( viewWeaponOrigin );
	savefile->WriteMat3( viewWeaponAxis );

	savefile->WriteVec3( muzzleOrigin );
	savefile->WriteMat3( muzzleAxis );

	savefile->WriteVec3( pushVelocity );

	savefile->WriteString( weaponDef->GetName() );
	savefile->WriteFloat( meleeDistance );
	savefile->WriteString( meleeDefName );
	savefile->WriteInt( brassDelay );
	savefile->WriteString( icon );

	savefile->WriteInt( guiLightHandle );
	savefile->WriteRenderLight( guiLight );

	savefile->WriteInt( muzzleFlashHandle );
	savefile->WriteRenderLight( muzzleFlash );

	savefile->WriteInt( worldMuzzleFlashHandle );
	savefile->WriteRenderLight( worldMuzzleFlash );

	savefile->WriteVec3( flashColor );
	savefile->WriteInt( muzzleFlashEnd );
	savefile->WriteInt( flashTime );

	savefile->WriteBool( lightOn );
	savefile->WriteBool( silent_fire );

	savefile->WriteInt( kick_endtime );
	savefile->WriteInt( muzzle_kick_time );
	savefile->WriteInt( muzzle_kick_maxtime );
	savefile->WriteAngles( muzzle_kick_angles );
	savefile->WriteVec3( muzzle_kick_offset );

	savefile->WriteInt( ammoType );
	savefile->WriteInt( ammoRequired );
	savefile->WriteInt( clipSize );
	savefile->WriteInt( ammoClip );
	savefile->WriteInt( lowAmmo );
	savefile->WriteBool( powerAmmo );

	// savegames <= 17
	savefile->WriteInt( 0 );

	savefile->WriteInt( zoomFov );

	savefile->WriteJoint( barrelJointView );
	savefile->WriteJoint( flashJointView );
	savefile->WriteJoint( ejectJointView );
	savefile->WriteJoint( guiLightJointView );
	savefile->WriteJoint( ventLightJointView );

	savefile->WriteJoint( flashJointWorld );
	savefile->WriteJoint( barrelJointWorld );
	savefile->WriteJoint( ejectJointWorld );

	savefile->WriteBool( hasBloodSplat );

	savefile->WriteSoundShader( sndHum );

	savefile->WriteParticle( weaponSmoke );
	savefile->WriteInt( weaponSmokeStartTime );
	savefile->WriteBool( continuousSmoke );
	savefile->WriteParticle( strikeSmoke );
	savefile->WriteInt( strikeSmokeStartTime );
	savefile->WriteVec3( strikePos );
	savefile->WriteMat3( strikeAxis );
	savefile->WriteInt( nextStrikeFx );

	savefile->WriteBool( nozzleFx );
	savefile->WriteInt( nozzleFxFade );

	savefile->WriteInt( lastAttack );

	savefile->WriteInt( nozzleGlowHandle );
	savefile->WriteRenderLight( nozzleGlow );

	savefile->WriteVec3( nozzleGlowColor );
	savefile->WriteMaterial( nozzleGlowShader );
	savefile->WriteFloat( nozzleGlowRadius );

	savefile->WriteInt( weaponAngleOffsetAverages );
	savefile->WriteFloat( weaponAngleOffsetScale );
	savefile->WriteFloat( weaponAngleOffsetMax );
	savefile->WriteFloat( weaponOffsetTime );
	savefile->WriteFloat( weaponOffsetScale );

	savefile->WriteBool( allowDrop );
	savefile->WriteObject( projectileEnt );

	savefile->WriteStaticObject( grabber );
	savefile->WriteInt( grabberState );

	savefile->WriteJoint( smokeJointView );

    // Koz begin
    for ( int i = 0; i < 2; i++ )
    {
        savefile->WriteJoint( weaponHandAttacher[i] );
        savefile->WriteVec3( weaponHandDefaultPos[i] );
        savefile->WriteMat3( weaponHandDefaultAxis[i] );
    }
    // Koz end

}
/*
================
idWeapon::SetFlashlightOwner

Only called at player spawn time, not each weapon switch
================
*/
void idWeapon::SetFlashlightOwner( idPlayer* _owner )
{
    assert( !owner );
    owner = _owner;
    SetName( va( "%s_weapon_flashlight", owner->name.c_str() ) );

    if( worldModel.GetEntity() )
    {
        worldModel.GetEntity()->SetName( va( "%s_weapon_flashlight_worldmodel", owner->name.c_str() ) );
    }
}


/*
================
idWeapon::Restore
================
*/
void idWeapon::Restore( idRestoreGame *savefile ) {

	savefile->ReadInt( (int &)status );
	savefile->ReadObject( reinterpret_cast<idClass *&>( thread ) );
	savefile->ReadString( state );
	savefile->ReadString( idealState );
	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( animDoneTime );
	savefile->ReadBool( isLinked );

	// Re-link script fields
	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	if (!IsDoom3DemoVersion()) // the demo assets don't support WEAPON_NETFIRING
		WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );

	savefile->ReadObject( reinterpret_cast<idClass *&>( owner ) );
	worldModel.Restore( savefile );

	savefile->ReadInt( hideTime );
	savefile->ReadFloat( hideDistance );
	savefile->ReadInt( hideStartTime );
	savefile->ReadFloat( hideStart );
	savefile->ReadFloat( hideEnd );
	savefile->ReadFloat( hideOffset );
	savefile->ReadBool( hide );
	savefile->ReadBool( disabled );

	savefile->ReadInt( berserk );

	savefile->ReadVec3( playerViewOrigin );
	savefile->ReadMat3( playerViewAxis );

	savefile->ReadVec3( viewWeaponOrigin );
	savefile->ReadMat3( viewWeaponAxis );

	savefile->ReadVec3( muzzleOrigin );
	savefile->ReadMat3( muzzleAxis );

	savefile->ReadVec3( pushVelocity );

	idStr objectname;
	savefile->ReadString( objectname );
	weaponDef = gameLocal.FindEntityDef( objectname );
	meleeDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_melee" ), false );

	const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_projectile" ), false );
	if ( projectileDef ) {
		projectileDict = projectileDef->dict;
	} else {
		projectileDict.Clear();
	}

	const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_ejectBrass" ), false );
	if ( brassDef ) {
		brassDict = brassDef->dict;
	} else {
		brassDict.Clear();
	}

	savefile->ReadFloat( meleeDistance );
	savefile->ReadString( meleeDefName );
	savefile->ReadInt( brassDelay );
	savefile->ReadString( icon );

	savefile->ReadInt( guiLightHandle );
	savefile->ReadRenderLight( guiLight );

	savefile->ReadInt( muzzleFlashHandle );
	savefile->ReadRenderLight( muzzleFlash );

	savefile->ReadInt( worldMuzzleFlashHandle );
	savefile->ReadRenderLight( worldMuzzleFlash );

	savefile->ReadVec3( flashColor );
	savefile->ReadInt( muzzleFlashEnd );
	savefile->ReadInt( flashTime );

	savefile->ReadBool( lightOn );
	savefile->ReadBool( silent_fire );

	savefile->ReadInt( kick_endtime );
	savefile->ReadInt( muzzle_kick_time );
	savefile->ReadInt( muzzle_kick_maxtime );
	savefile->ReadAngles( muzzle_kick_angles );
	savefile->ReadVec3( muzzle_kick_offset );

	savefile->ReadInt( (int &)ammoType );
	savefile->ReadInt( ammoRequired );
	savefile->ReadInt( clipSize );
	savefile->ReadInt( ammoClip );
	savefile->ReadInt( lowAmmo );
	savefile->ReadBool( powerAmmo );

	// savegame versions <= 17
	int foo;
	savefile->ReadInt( foo );

	savefile->ReadInt( zoomFov );

	savefile->ReadJoint( barrelJointView );
	savefile->ReadJoint( flashJointView );
	savefile->ReadJoint( ejectJointView );
	savefile->ReadJoint( guiLightJointView );
	savefile->ReadJoint( ventLightJointView );

	savefile->ReadJoint( flashJointWorld );
	savefile->ReadJoint( barrelJointWorld );
	savefile->ReadJoint( ejectJointWorld );

	savefile->ReadBool( hasBloodSplat );

	savefile->ReadSoundShader( sndHum );

	savefile->ReadParticle( weaponSmoke );
	savefile->ReadInt( weaponSmokeStartTime );
	savefile->ReadBool( continuousSmoke );
	savefile->ReadParticle( strikeSmoke );
	savefile->ReadInt( strikeSmokeStartTime );
	savefile->ReadVec3( strikePos );
	savefile->ReadMat3( strikeAxis );
	savefile->ReadInt( nextStrikeFx );

	savefile->ReadBool( nozzleFx );
	savefile->ReadInt( nozzleFxFade );

	savefile->ReadInt( lastAttack );

	savefile->ReadInt( nozzleGlowHandle );
	savefile->ReadRenderLight( nozzleGlow );

	savefile->ReadVec3( nozzleGlowColor );
	savefile->ReadMaterial( nozzleGlowShader );
	savefile->ReadFloat( nozzleGlowRadius );

	savefile->ReadInt( weaponAngleOffsetAverages );
	savefile->ReadFloat( weaponAngleOffsetScale );
	savefile->ReadFloat( weaponAngleOffsetMax );
	savefile->ReadFloat( weaponOffsetTime );
	savefile->ReadFloat( weaponOffsetScale );

	savefile->ReadBool( allowDrop );
	savefile->ReadObject( reinterpret_cast<idClass *&>( projectileEnt ) );

	savefile->ReadStaticObject( grabber );
	savefile->ReadInt( grabberState );

	savefile->ReadJoint( smokeJointView );

    for ( int i = 0; i < 2; i++ ) {
        savefile->ReadJoint(weaponHandAttacher[i]);
        savefile->ReadVec3(weaponHandDefaultPos[i]);
        savefile->ReadMat3(weaponHandDefaultAxis[i]);
    }
    // gui for stats device on player wrist in VR.
    //rvrStatGui = uiManager->FindGui( "guis/weapons/rvrstatgui.gui", true, false, true );
    lvrStatGui = uiManager->FindGui( "guis/weapons/lvrstatgui.gui", true, false, true );

    //scale = 1.0f; // koz code to scale weapon models in the world borks the viewmodel when loading quick or autosaves, so make sure scale is correct here.
}

/***********************************************************************

	Weapon definition management

***********************************************************************/

/*
================
idWeapon::Clear
================
*/
void idWeapon::Clear( void ) {
	CancelEvents( &EV_Weapon_Clear );

	DeconstructScriptObject();
	scriptObject.Free();

	WEAPON_ATTACK.Unlink();
	WEAPON_RELOAD.Unlink();
	WEAPON_NETRELOAD.Unlink();
	WEAPON_NETENDRELOAD.Unlink();
	if (WEAPON_NETFIRING.IsLinked())
		WEAPON_NETFIRING.Unlink();
	WEAPON_RAISEWEAPON.Unlink();
	WEAPON_LOWERWEAPON.Unlink();

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}
	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}
	if ( worldMuzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
		worldMuzzleFlashHandle = -1;
	}
	if ( guiLightHandle != -1 ) {
		gameRenderWorld->FreeLightDef( guiLightHandle );
		guiLightHandle = -1;
	}
	if ( nozzleGlowHandle != -1 ) {
		gameRenderWorld->FreeLightDef( nozzleGlowHandle );
		nozzleGlowHandle = -1;
	}

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.entityNum	= entityNumber;

	renderEntity.noShadow		= true;
	renderEntity.noSelfShadow	= true;
	renderEntity.customSkin		= NULL;

	// set default shader parms
	renderEntity.shaderParms[ SHADERPARM_RED ]	= 1.0f;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]= 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
	renderEntity.shaderParms[3] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = 0.0f;
	renderEntity.shaderParms[5] = 0.0f;
	renderEntity.shaderParms[6] = 0.0f;
	renderEntity.shaderParms[7] = 0.0f;

	if ( refSound.referenceSound ) {
		refSound.referenceSound->Free( true );
	}
	memset( &refSound, 0, sizeof( refSound_t ) );

	// setting diversity to 0 results in no random sound.  -1 indicates random.
	refSound.diversity = -1.0f;

	if ( owner ) {
		// don't spatialize the weapon sounds
		refSound.listenerId = owner->GetListenerId();
	}

	// clear out the sounds from our spawnargs since we'll copy them from the weapon def
	const idKeyValue *kv = spawnArgs.MatchPrefix( "snd_" );
	while( kv ) {
		spawnArgs.Delete( kv->GetKey() );
		kv = spawnArgs.MatchPrefix( "snd_" );
	}

	hideTime		= 300;
	hideDistance	= -15.0f;
	hideStartTime	= gameLocal.time - hideTime;
	hideStart		= 0.0f;
	hideEnd			= 0.0f;
	hideOffset		= 0.0f;
	hide			= false;
	disabled		= false;

	weaponSmoke		= NULL;
	weaponSmokeStartTime = 0;
	continuousSmoke = false;
	strikeSmoke		= NULL;
	strikeSmokeStartTime = 0;
	strikePos.Zero();
	strikeAxis = mat3_identity;
	nextStrikeFx = 0;

	icon			= "";

	playerViewAxis.Identity();
	playerViewOrigin.Zero();
	viewWeaponAxis.Identity();
	viewWeaponOrigin.Zero();
	muzzleAxis.Identity();
	muzzleOrigin.Zero();
	pushVelocity.Zero();

	status			= WP_HOLSTERED;
	state			= "";
	idealState		= "";
	animBlendFrames	= 0;
	animDoneTime	= 0;

	projectileDict.Clear();
	meleeDef		= NULL;
	meleeDefName	= "";
	meleeDistance	= 0.0f;
	brassDict.Clear();

	flashTime		= 250;
	lightOn			= false;
	silent_fire		= false;

	grabberState	= -1;
	grabber.Update( owner, this, true );

	ammoType		= 0;
	ammoRequired	= 0;
	ammoClip		= 0;
	clipSize		= 0;
	lowAmmo			= 0;
	powerAmmo		= false;

	kick_endtime		= 0;
	muzzle_kick_time	= 0;
	muzzle_kick_maxtime	= 0;
	muzzle_kick_angles.Zero();
	muzzle_kick_offset.Zero();

	zoomFov = 90;

	barrelJointView		= INVALID_JOINT;
	flashJointView		= INVALID_JOINT;
	ejectJointView		= INVALID_JOINT;
	guiLightJointView	= INVALID_JOINT;
	ventLightJointView	= INVALID_JOINT;

	barrelJointWorld	= INVALID_JOINT;
	flashJointWorld		= INVALID_JOINT;
	ejectJointWorld		= INVALID_JOINT;

	smokeJointView		= INVALID_JOINT;

	hasBloodSplat		= false;
	nozzleFx			= false;
	nozzleFxFade		= 1500;
	lastAttack			= 0;
	nozzleGlowHandle	= -1;
	nozzleGlowShader	= NULL;
	nozzleGlowRadius	= 10;
	nozzleGlowColor.Zero();

	weaponAngleOffsetAverages	= 0;
	weaponAngleOffsetScale		= 0.0f;
	weaponAngleOffsetMax		= 0.0f;
	weaponOffsetTime			= 0.0f;
	weaponOffsetScale			= 0.0f;

	allowDrop			= true;

	animator.ClearAllAnims( gameLocal.time, 0 );
	FreeModelDef();

	sndHum				= NULL;

	isLinked			= false;
	projectileEnt		= NULL;

	isFiring			= false;
}

/*
================
idWeapon::InitWorldModel
================
*/
void idWeapon::InitWorldModel( const idDeclEntityDef *def ) {
	idEntity *ent;

	ent = worldModel.GetEntity();

	assert( ent );
	assert( def );

	const char *model = def->dict.GetString( "model_world" );
	const char *attach = def->dict.GetString( "joint_attach" );

	ent->SetSkin( NULL );
	if ( model[0] && attach[0] ) {
		ent->Show();
		ent->SetModel( model );
		if ( ent->GetAnimator()->ModelDef() ) {
			ent->SetSkin( ent->GetAnimator()->ModelDef()->GetDefaultSkin() );
		}
		ent->GetPhysics()->SetContents( 0 );
		ent->GetPhysics()->SetClipModel( NULL, 1.0f );
		ent->BindToJoint( owner, attach, true );
		ent->GetPhysics()->SetOrigin( vec3_origin );
		ent->GetPhysics()->SetAxis( mat3_identity );

		// supress model in player views, but allow it in mirrors and remote views
		renderEntity_t *worldModelRenderEntity = ent->GetRenderEntity();
		if ( worldModelRenderEntity ) {
            // Koz begin
            if ( game->isVR && !strstr(def->dict.GetString("model_world"),"flashlight"))
            {
                // Koz dont show the worldmodel if the player body is shown in vr.
                worldModelRenderEntity->suppressSurfaceInViewID = 0;
                worldModelRenderEntity->suppressShadowInViewID = 0;
            }
            else
            {
                //Carl: Don't suppress drawing the weapon's world model or it's shadow in 1st person, if they want to see it (in VR)
                worldModelRenderEntity->suppressSurfaceInViewID = owner->entityNumber + 1;
                worldModelRenderEntity->suppressShadowInViewID = owner->entityNumber + 1;
            }
            // Koz end
			worldModelRenderEntity->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
		}
	} else {
		ent->SetModel( "" );
		ent->Hide();
	}

	flashJointWorld = ent->GetAnimator()->GetJointHandle( "flash" );
	barrelJointWorld = ent->GetAnimator()->GetJointHandle( "muzzle" );
	ejectJointWorld = ent->GetAnimator()->GetJointHandle( "eject" );
}

/*
================
idWeapon::GetWeaponDef
================
*/
void idWeapon::GetWeaponDef( const char *objectname, int ammoinclip ) {
	const char *shader;
	const char *objectType;
	const char *vmodel;
	const char *guiName;
	const char *projectileName;
	const char *brassDefName;
	const char *smokeName;
	int			ammoAvail;

	Clear();

	if ( !objectname || !objectname[ 0 ] ) {
		return;
	}

	assert( owner );

	weaponDef			= gameLocal.FindEntityDef( objectname );

	ammoType			= GetAmmoNumForName( weaponDef->dict.GetString( "ammoType" ) );
	ammoRequired		= weaponDef->dict.GetInt( "ammoRequired" );
	clipSize			= weaponDef->dict.GetInt( "clipSize" );
	lowAmmo				= weaponDef->dict.GetInt( "lowAmmo" );

	icon				= weaponDef->dict.GetString( "icon" );
	silent_fire			= weaponDef->dict.GetBool( "silent_fire" );
	powerAmmo			= weaponDef->dict.GetBool( "powerAmmo" );

	muzzle_kick_time	= SEC2MS( weaponDef->dict.GetFloat( "muzzle_kick_time" ) );
	muzzle_kick_maxtime	= SEC2MS( weaponDef->dict.GetFloat( "muzzle_kick_maxtime" ) );
	muzzle_kick_angles	= weaponDef->dict.GetAngles( "muzzle_kick_angles" );
	muzzle_kick_offset	= weaponDef->dict.GetVector( "muzzle_kick_offset" );

	hideTime			= SEC2MS( weaponDef->dict.GetFloat( "hide_time", "0.3" ) );
	hideDistance		= weaponDef->dict.GetFloat( "hide_distance", "-15" );

	// muzzle smoke
	smokeName = weaponDef->dict.GetString( "smoke_muzzle" );
	if ( *smokeName != '\0' ) {
		weaponSmoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	} else {
		weaponSmoke = NULL;
	}
	continuousSmoke = weaponDef->dict.GetBool( "continuousSmoke" );
	weaponSmokeStartTime = ( continuousSmoke ) ? gameLocal.time : 0;

	smokeName = weaponDef->dict.GetString( "smoke_strike" );
	if ( *smokeName != '\0' ) {
		strikeSmoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	} else {
		strikeSmoke = NULL;
	}
	strikeSmokeStartTime = 0;
	strikePos.Zero();
	strikeAxis = mat3_identity;
	nextStrikeFx = 0;

	// setup gui light
	memset( &guiLight, 0, sizeof( guiLight ) );
	const char *guiLightShader = weaponDef->dict.GetString( "mtr_guiLightShader" );
	if ( *guiLightShader != '\0' ) {
        // Koz begin
        // the PDA model was scaled by a factor of 2.5 to make it easier to read in VR.
        // The weapon guilight size isn't stored in the weapon def, it's hardcoded here,
        // so check if PDA and resize the guilight accordingly

        int lightRad = 3;
        if ( game->isVR )
        {
            const char* weapName = weaponDef->dict.GetString( "inv_name" );
            if ( strstr( weapName, "PDA" ) )
            {
                lightRad *= 3; // the PDA guilight needs to be bigger in VR
            }
        }
	    guiLight.shader = declManager->FindMaterial( guiLightShader, false );
        guiLight.lightRadius[0] = guiLight.lightRadius[1] = guiLight.lightRadius[2] = lightRad;
		guiLight.pointLight = true;
	}

	// setup the view model
	vmodel = weaponDef->dict.GetString( "model_view" );
	SetModel( vmodel );

    // Koz weapon shadows
    renderEntity.noShadow = false;

	// setup the world model
	InitWorldModel( weaponDef );
	worldModel.GetEntity()->GetRenderEntity()->noShadow = true; // Koz fixme only if using viewweapon for body model
	// copy the sounds from the weapon view model def into out spawnargs
	const idKeyValue *kv = weaponDef->dict.MatchPrefix( "snd_" );
	while( kv ) {
		spawnArgs.Set( kv->GetKey(), kv->GetValue() );
		kv = weaponDef->dict.MatchPrefix( "snd_", kv );
	}

    if ( game->isVR )
    {
        weaponHandAttacher[0] = animator.GetJointHandle( "RhandAttacher" );
        if ( weaponHandAttacher[0] != INVALID_JOINT )
        {
            // Koz debug common->Printf( "Weapon %s RhandAttacherJoint Found\n", objectname );
            animator.GetJointTransform( weaponHandAttacher[0], gameLocal.time, weaponHandDefaultPos[0], weaponHandDefaultAxis[0] );
            // Koz debug common->Printf( "Default pos %s default axis %s\n", weaponHandDefaultPos[0].ToString(), weaponHandDefaultAxis[0].ToAngles().ToString() );

        }

        weaponHandAttacher[1] = animator.GetJointHandle( "LhandAttacher" );
        if ( weaponHandAttacher[1] != INVALID_JOINT )
        {
            // Koz debug common->Printf( "Weapon %s LhandAttacherJoint Found\n", objectname );
            animator.GetJointTransform( weaponHandAttacher[1], gameLocal.time, weaponHandDefaultPos[1], weaponHandDefaultAxis[1] );
            // Koz debug common->Printf( "Default pos %s default axis %s\n", weaponHandDefaultPos[1].ToString(), weaponHandDefaultAxis[1].ToAngles().ToString() );
        }

        laserSightOffset = weaponDef->dict.GetVector( "laserSightOffset" );

    }

	// find some joints in the model for locating effects
	barrelJointView = animator.GetJointHandle( "barrel" );
	flashJointView = animator.GetJointHandle( "flash" );
	ejectJointView = animator.GetJointHandle( "eject" );
	guiLightJointView = animator.GetJointHandle( "guiLight" );
	ventLightJointView = animator.GetJointHandle( "ventLight" );

	idStr smokeJoint = weaponDef->dict.GetString( "smoke_joint" );
	if( smokeJoint.Length() > 0 )
	{
		smokeJointView = animator.GetJointHandle( smokeJoint );
	}
	else
	{
		smokeJointView = INVALID_JOINT;
	}

	// get the projectile
	projectileDict.Clear();

	projectileName = weaponDef->dict.GetString( "def_projectile" );
	if ( projectileName[0] != '\0' ) {
		const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( projectileName, false );
		if ( !projectileDef ) {
			gameLocal.Warning( "Unknown projectile '%s' in weapon '%s'", projectileName, objectname );
		} else {
			const char *spawnclass = projectileDef->dict.GetString( "spawnclass" );
			idTypeInfo *cls = idClass::GetClass( spawnclass );
			if ( !cls || !cls->IsType( idProjectile::Type ) ) {
				gameLocal.Warning( "Invalid spawnclass '%s' on projectile '%s' (used by weapon '%s')", spawnclass, projectileName, objectname );
			} else {
				projectileDict = projectileDef->dict;
			}
		}
	}

	// set up muzzleflash render light
	const idMaterial*flashShader;
	idVec3			flashTarget;
	idVec3			flashUp;
	idVec3			flashRight;
	float			flashRadius;
	bool			flashPointLight;

	weaponDef->dict.GetString( "mtr_flashShader", "", &shader );
	flashShader = declManager->FindMaterial( shader, false );
	flashPointLight = weaponDef->dict.GetBool( "flashPointLight", "1" );
	weaponDef->dict.GetVector( "flashColor", "0 0 0", flashColor );
	flashRadius		= (float)weaponDef->dict.GetInt( "flashRadius" );	// if 0, no light will spawn
	flashTime		= SEC2MS( weaponDef->dict.GetFloat( "flashTime", "0.25" ) );
	flashTarget		= weaponDef->dict.GetVector( "flashTarget" );
	flashUp			= weaponDef->dict.GetVector( "flashUp" );
	flashRight		= weaponDef->dict.GetVector( "flashRight" );

	memset( &muzzleFlash, 0, sizeof( muzzleFlash ) );
	muzzleFlash.lightId = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
	muzzleFlash.allowLightInViewID = owner->entityNumber+1;

	// the weapon lights will only be in first person
	guiLight.allowLightInViewID = owner->entityNumber+1;
	nozzleGlow.allowLightInViewID = owner->entityNumber+1;

	muzzleFlash.pointLight								= flashPointLight;
	muzzleFlash.shader									= flashShader;
	muzzleFlash.shaderParms[ SHADERPARM_RED ]			= flashColor[0];
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]			= flashColor[1];
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]			= flashColor[2];
	muzzleFlash.shaderParms[ SHADERPARM_TIMESCALE ]		= 1.0f;

	muzzleFlash.lightRadius[0]							= flashRadius;
	muzzleFlash.lightRadius[1]							= flashRadius;
	muzzleFlash.lightRadius[2]							= flashRadius;

	if ( !flashPointLight ) {
		muzzleFlash.target								= flashTarget;
		muzzleFlash.up									= flashUp;
		muzzleFlash.right								= flashRight;
		muzzleFlash.end									= flashTarget;
	}

	// the world muzzle flash is the same, just positioned differently
	worldMuzzleFlash = muzzleFlash;
	worldMuzzleFlash.suppressLightInViewID = owner->entityNumber+1;
	worldMuzzleFlash.allowLightInViewID = 0;
	worldMuzzleFlash.lightId = LIGHTID_WORLD_MUZZLE_FLASH + owner->entityNumber;

	//-----------------------------------

	nozzleFx			= weaponDef->dict.GetBool("nozzleFx");
	nozzleFxFade		= weaponDef->dict.GetInt("nozzleFxFade", "1500");
	nozzleGlowColor		= weaponDef->dict.GetVector("nozzleGlowColor", "1 1 1");
	nozzleGlowRadius	= weaponDef->dict.GetFloat("nozzleGlowRadius", "10");
	weaponDef->dict.GetString( "mtr_nozzleGlowShader", "", &shader );
	nozzleGlowShader = declManager->FindMaterial( shader, false );

	// get the melee damage def
	meleeDistance = weaponDef->dict.GetFloat( "melee_distance" );
	meleeDefName = weaponDef->dict.GetString( "def_melee" );
	if ( meleeDefName.Length() ) {
		meleeDef = gameLocal.FindEntityDef( meleeDefName, false );
		if ( !meleeDef ) {
			gameLocal.Error( "Unknown melee '%s'", meleeDefName.c_str() );
		}
	}

	// get the brass def
	brassDict.Clear();
	brassDelay = weaponDef->dict.GetInt( "ejectBrassDelay", "0" );
	brassDefName = weaponDef->dict.GetString( "def_ejectBrass" );

	if ( brassDefName[0] ) {
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( brassDefName, false );
		if ( !brassDef ) {
			gameLocal.Warning( "Unknown brass '%s'", brassDefName );
		} else {
			brassDict = brassDef->dict;
		}
	}

	if ( ( ammoType < 0 ) || ( ammoType >= AMMO_NUMTYPES ) ) {
		gameLocal.Warning( "Unknown ammotype in object '%s'", objectname );
	}

	ammoClip = ammoinclip;
	if ( ( ammoClip < 0 ) || ( ammoClip > clipSize ) ) {
		// first time using this weapon so have it fully loaded to start
		ammoClip = clipSize;
		ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		if ( ammoClip > ammoAvail ) {
			ammoClip = ammoAvail;
		}
	}

	renderEntity.gui[ 0 ] = NULL;
	guiName = weaponDef->dict.GetString( "gui" );
	if ( guiName[0] ) {
		renderEntity.gui[ 0 ] = uiManager->FindGui( guiName, true, false, true );
	}

    // Koz begin - gui for stats device on player wrist in VR.
    //rvrStatGui = uiManager->FindGui("guis/weapons/rvrstatgui.gui", true, false, true);
    lvrStatGui = uiManager->FindGui("guis/weapons/lvrstatgui.gui", true, false, true);
    // Koz end

	zoomFov = weaponDef->dict.GetInt( "zoomFov", "70" );
	berserk = weaponDef->dict.GetInt( "berserk", "2" );

	weaponAngleOffsetAverages = weaponDef->dict.GetInt( "weaponAngleOffsetAverages", "10" );
	weaponAngleOffsetScale = weaponDef->dict.GetFloat( "weaponAngleOffsetScale", "0.25" );
	weaponAngleOffsetMax = weaponDef->dict.GetFloat( "weaponAngleOffsetMax", "10" );

	weaponOffsetTime = weaponDef->dict.GetFloat( "weaponOffsetTime", "400" );
	weaponOffsetScale = weaponDef->dict.GetFloat( "weaponOffsetScale", "0.005" );

	if ( !weaponDef->dict.GetString( "weapon_scriptobject", NULL, &objectType ) ) {
		gameLocal.Error( "No 'weapon_scriptobject' set on '%s'.", objectname );
	}

	// setup script object
	if ( !scriptObject.SetType( objectType ) ) {
		gameLocal.Error( "Script object '%s' not found on weapon '%s'.", objectType, objectname );
	}

	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	if (!IsDoom3DemoVersion()) // the demo assets don't support WEAPON_NETFIRING
		WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );

	spawnArgs = weaponDef->dict;

	shader = spawnArgs.GetString( "snd_hum" );
	if ( shader && *shader ) {
		sndHum = declManager->FindSound( shader );
		StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
	}

	isLinked = true;

	// call script object's constructor
	ConstructScriptObject();

	// make sure we have the correct skin
	UpdateSkin();
}

/***********************************************************************

	GUIs

***********************************************************************/

/*
================
idWeapon::Icon
================
*/
const char *idWeapon::Icon( void ) const {
	return icon;
}

/*
===============
idWeapon::IdentifyWeapon
Koz return weapon enumeration
===============
*/

weapon_t idWeapon::IdentifyWeapon()
{
	int currentFrameNumber = common->GetFrameNumber();
    if ( lastIdentifiedFrame == currentFrameNumber ) // only check once per game frame.
    {
    	return currentIdentifiedWeapon;
    }

    lastIdentifiedFrame = currentFrameNumber;

    currentIdentifiedWeapon = WEAPON_NONE;

    if ( this )
    {
        if ( weaponDef != NULL )
        {
            idStr weaponName = weaponDef->GetName();

			if ( idStr::Icmp( "weapon_fists", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_FISTS;
			else if ( idStr::Icmp( "weapon_chainsaw", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_CHAINSAW;
			else if ( idStr::Icmp( "weapon_pistol", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_PISTOL;
			else if ( idStr::Icmp( "weapon_shotgun", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_SHOTGUN;
			else if ( idStr::Icmp( "weapon_machinegun", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_MACHINEGUN;
			else if ( idStr::Icmp( "weapon_chaingun", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_CHAINGUN;
			else if ( idStr::Icmp( "weapon_handgrenade", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_HANDGRENADE;
			else if ( idStr::Icmp( "weapon_plasmagun", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_PLASMAGUN;
			else if ( idStr::Icmp( "weapon_rocketlauncher", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_ROCKETLAUNCHER;
			else if ( idStr::Icmp( "weapon_bfg", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_BFG;
			else if ( idStr::Icmp( "weapon_soulcube", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_SOULCUBE;
			else if ( idStr::Icmp( "weapon_shotgun_double_mp", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_SHOTGUN_DOUBLE_MP;
			else if ( idStr::Icmp( "weapon_grabber", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_GRABBER;
			else if ( idStr::Icmp( "weapon_shotgun_double", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_SHOTGUN_DOUBLE;
			else if ( idStr::Icmp( "weapon_artifact", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_ARTIFACT;
			else if ( idStr::Icmp( "weapon_bloodstone_passive", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_ARTIFACT;
			else if ( idStr::Icmp( "weapon_bloodstone_active1", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_ARTIFACT;
			else if ( idStr::Icmp( "weapon_bloodstone_active2", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_ARTIFACT;
			else if ( idStr::Icmp( "weapon_bloodstone_active3", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_ARTIFACT;
			else if ( idStr::Icmp( "weapon_pda", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = WEAPON_PDA;
			else if ( idStr::Icmp( "weapon_flashlight_new", weaponDef->GetName() ) == 0 ) currentIdentifiedWeapon = lastIdentifiedWeapon;
			//else if ( idStr::Icmp( "weapon_flashlight_new", weaponDef->GetName() ) == 0 ) currentWeapon = WEAPON_FLASHLIGHT;
			//else if ( idStr::Icmp( "weapon_flashlight", weaponDef->GetName() ) == 0 ) currentWeapon = WEAPON_FLASHLIGHT;
			else currentIdentifiedWeapon = WEAPON_NONE;
			lastIdentifiedWeapon = currentIdentifiedWeapon;
        }
    }
    if ( currentIdentifiedWeapon == WEAPON_FLASHLIGHT ) common->Printf( "Identify weapon returned %s\n", weaponDef->GetName() );

    return currentIdentifiedWeapon;
}

/*
================
idWeapon::UpdateGUI
================
*/
void idWeapon::UpdateGUI( void ) {
    if ( game->isVR ) UpdateVRGUI();

	if ( !renderEntity.gui[ 0 ] ) {
		return;
	}

	if ( status == WP_HOLSTERED ) {
		return;
	}

	if ( owner->weaponGone ) {
		// dropping weapons was implemented wierd, so we have to not update the gui when it happens or we'll get a negative ammo count
		return;
	}

	if ( gameLocal.localClientNum != owner->entityNumber ) {
		// if updating the hud for a followed client
		if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
			idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
			if ( !p->spectating || p->spectator != owner->entityNumber ) {
				return;
			}
		} else {
			return;
		}
	}

	int inclip = AmmoInClip();
	int ammoamount = AmmoAvailable();

	if ( ammoamount < 0 ) {
		// show infinite ammo
		renderEntity.gui[ 0 ]->SetStateString( "player_ammo", "" );
	} else {
		// show remaining ammo
		renderEntity.gui[ 0 ]->SetStateString( "player_totalammo", va( "%i", ammoamount - inclip) );
		renderEntity.gui[ 0 ]->SetStateString( "player_ammo", ClipSize() ? va( "%i", inclip ) : "--" );
		renderEntity.gui[ 0 ]->SetStateString( "player_clips", ClipSize() ? va("%i", ammoamount / ClipSize()) : "--" );
		renderEntity.gui[ 0 ]->SetStateString( "player_allammo", va( "%i/%i", inclip, ammoamount - inclip ) );
	}
	renderEntity.gui[ 0 ]->SetStateBool( "player_ammo_empty", ( ammoamount == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "player_clip_empty", ( inclip == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "player_clip_low", ( inclip <= lowAmmo ) );

	//Grabber Gui Info
	renderEntity.gui[ 0 ]->SetStateString( "grabber_state", va( "%i", grabberState ) );
}

/***********************************************************************

	Model and muzzleflash

***********************************************************************/

/*
================
idWeapon::UpdateFlashPosition
================
*/
void idWeapon::UpdateFlashPosition( void ) {
	// the flash has an explicit joint for locating it
	GetGlobalJointTransform( true, flashJointView, muzzleFlash.origin, muzzleFlash.axis );

    if( isPlayerFlashlight )
    {
        static float pscale = 2.0f;
        static float yscale = 0.25f;

// 		static idVec3 baseAdjustPos = vec3_zero;	//idVec3( 0.0f, 10.0f, 0.0f );
// 		idVec3 adjustPos = baseAdjustPos;
//		muzzleFlash.origin += adjustPos.x * muzzleFlash.axis[1] + adjustPos.y * muzzleFlash.axis[0] + adjustPos.z * muzzleFlash.axis[2];
        muzzleFlash.origin += owner->GetViewBob();

//		static idAngles baseAdjustAng = ang_zero;	//idAngles( 0.0f, 10.0f, 0.0f );
        idAngles adjustAng = /*baseAdjustAng +*/ idAngles( fraccos * yscale, 0.0f, fraccos2 * pscale );
        idAngles bobAngles = owner->GetViewBobAngles();
        SwapValues( bobAngles.pitch, bobAngles.roll );
        adjustAng += bobAngles * 3.0f;
        muzzleFlash.axis = adjustAng.ToMat3() * muzzleFlash.axis /** adjustAng.ToMat3()*/;
    }

    // if the desired point is inside or very close to a wall, back it up until it is clear
    idVec3	start = muzzleFlash.origin - playerViewAxis[0] * 16;
    idVec3	end = muzzleFlash.origin + playerViewAxis[0] * 8;
    trace_t	tr;
    gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, owner );
    // be at least 8 units away from a solid
    muzzleFlash.origin = tr.endpos - playerViewAxis[0] * 8;

    muzzleFlash.noShadows = !g_weaponShadows.GetBool();

    // put the world muzzle flash on the end of the joint, no matter what
    GetGlobalJointTransform( false, flashJointWorld, worldMuzzleFlash.origin, worldMuzzleFlash.axis );
}

/*
================
idWeapon::MuzzleFlashLight
================
*/
void idWeapon::MuzzleFlashLight( void ) {

	if ( !lightOn && ( !g_muzzleFlash.GetBool() || !muzzleFlash.lightRadius[0] ) ) {
		return;
	}

	if ( flashJointView == INVALID_JOINT ) {
		return;
	}

	UpdateFlashPosition();

	// these will be different each fire
	muzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
	muzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]		= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

	worldMuzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
	worldMuzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]	= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

	// the light will be removed at this time
	muzzleFlashEnd = gameLocal.time + flashTime;

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
		gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );
	} else {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
		worldMuzzleFlashHandle = gameRenderWorld->AddLightDef( &worldMuzzleFlash );
	}
}

/*
================
idWeapon::UpdateVRGUI
================
*/
void idWeapon::UpdateVRGUI()
{

    float healthv = 0.0f;
    float armorv = 0.0f;
    float armorb = 0.0f;
    float healthb = 0.0f;

    idPlayer* player;
    player = gameLocal.GetLocalPlayer();

    if ( !player ) return;

	if (lvrStatGui == NULL)
	{
		common->Printf("Left Hand stat gui not found.\n");
		return;
	}

    if ( status == WP_HOLSTERED )
    {
        return;
    }

    if ( isPlayerFlashlight || isPlayerLeftHand ) return;

    if ( owner->weaponGone )
    {
        // dropping weapons was implemented weird, so we have to not update the gui when it happens or we'll get a negative ammo count
        return;
    }

    int inclip[2];
    int ammoamount[2];
    int clipsize[2];
    bool harvester[2];
    bool ammoEmpty[2];
    bool clipEmpty[2];
    bool clipLow[2];

    for ( int h = 0; h < 2; h++ )
    {
        inclip[h] = player->hands[h].weapon.GetEntity()->AmmoInClip();
        ammoamount[h] = player->hands[h].weapon.GetEntity()->AmmoAvailable();
        harvester[h] = player->hands[h].weapon.GetEntity()->IdentifyWeapon() == WEAPON_ARTIFACT || player->hands[h].weapon.GetEntity()->IdentifyWeapon() == WEAPON_SOULCUBE;
        clipsize[h] = player->hands[h].weapon.GetEntity()->ClipSize();


        ammoEmpty[h] = ( ammoamount[h] == 0 ) ;
        clipEmpty[h] = ( player->hands[h].weapon.GetEntity()->ClipSize() && inclip[h] == 0);
        clipLow[h] = ( player->hands[h].weapon.GetEntity()->ClipSize() && player->hands[h].weapon.GetEntity()->LowAmmo());
    }

    //int inclip = AmmoInClip();
    //int ammoamount = AmmoAvailable();

    // update the right hand statwatch


    /*if ( ammoamount[HAND_RIGHT] < 0 )
    {
        // show infinite ammo
        rvrStatGui->SetStateString( "player_ammo", "" );
    }
    else
    {
        // show remaining ammo
        rvrStatGui->SetStateString("player_totalammo", va("%i", ammoamount[HAND_RIGHT]));
        rvrStatGui->SetStateString("player_ammo", clipsize[HAND_RIGHT] ? va("%i", inclip[HAND_RIGHT]) : "--");
        rvrStatGui->SetStateString("player_clips", clipsize[HAND_RIGHT] ? va("%i", ammoamount[HAND_RIGHT] / clipsize[HAND_RIGHT]) : "--");
        rvrStatGui->SetStateString("player_allammo", va("%i/%i", inclip[HAND_RIGHT], ammoamount[HAND_RIGHT]));
    }
    rvrStatGui->SetStateBool("player_ammo_empty", ( ammoEmpty[HAND_RIGHT] ));
    rvrStatGui->SetStateBool("player_clip_empty", ( clipEmpty[HAND_RIGHT] ));
    rvrStatGui->SetStateBool("player_clip_low", ( clipLow[HAND_RIGHT] ));

    // koz todo
    // need to get the ammocount from the hand structures, not sure right now, in the interim just use the ammo amount.

    //Let the GUI know the total amount of ammo regardless of the ammo required value
    //rvrStatGui->SetStateString( "player_ammo_count", va( "%i", AmmoCount() ) );

    rvrStatGui->SetStateString("player_ammo_count", va("%i", ammoamount[HAND_RIGHT]));
    // koz end

	// koz todo also need to figure this out for dual guis
	//Grabber Gui Info
	rvrStatGui->SetStateString( "grabber_state", va( "%i", grabberState ) );

    //health and armor
	*/
    if ( player )
    {
        healthv = (float)player->health;
        if ( player->inventory.armor )
        {
            armorv = (float)player->inventory.armor;
        }

        //healthv = vr_guiH.GetInteger();
        //armorv = vr_guiA.GetInteger();

        healthv = idMath::ClampFloat( 0.0, 100.0, healthv );
        armorv = idMath::ClampFloat( 0.0, 100.0, armorv );

        if ( healthv >= 75.0 ) healthb = ( healthv / 100.0f ) ;
        if ( armorv >= 75.0 ) armorb = ( armorv / 100.0f ) ;

        healthb = idMath::ClampFloat( 0.0, 100.0, healthb );
        armorb = idMath::ClampFloat( 0.0, 100.0, armorb );

    }
	/*
    rvrStatGui->SetStateString( "player_health", va( "%f", healthv ) );
    //vrStatGui->SetStateString( "player_armor", va( "%i%%", armorv ) );
    rvrStatGui->SetStateString( "player_armor", va( "%f", armorv ) );
    rvrStatGui->SetStateString( "player_healthb", va( "%f", healthb ) );
    rvrStatGui->SetStateString( "player_armorb", va( "%f", armorb ) );
	*/

    lvrStatGui->SetStateString("player_health", va("%f", healthv));
    //vrStatGui->SetStateString( "player_armor", va( "%i%%", armorv ) );
    lvrStatGui->SetStateString("player_armor", va("%f", armorv));
    lvrStatGui->SetStateString("player_healthb", va("%f", healthb));
    lvrStatGui->SetStateString("player_armorb", va("%f", armorb));

    if (ammoamount[vr_weaponHand.GetInteger()] < 0)
    {
        // show infinite ammo
        lvrStatGui->SetStateString("player_ammo", "");
    }
    else
    {
        // show remaining ammo
        lvrStatGui->SetStateString("player_totalammo", va("%i", ammoamount[vr_weaponHand.GetInteger()]));
        lvrStatGui->SetStateString("player_ammo", clipsize[vr_weaponHand.GetInteger()] ? va("%i", inclip[vr_weaponHand.GetInteger()]) : "--");
        lvrStatGui->SetStateString("player_clips", clipsize[vr_weaponHand.GetInteger()] ? va("%i", ammoamount[vr_weaponHand.GetInteger()] / clipsize[vr_weaponHand.GetInteger()]) : "--");
        lvrStatGui->SetStateString("player_allammo", va("%i/%i", inclip[vr_weaponHand.GetInteger()], ammoamount[vr_weaponHand.GetInteger()]));
    }
    lvrStatGui->SetStateBool("player_ammo_empty", (ammoEmpty[vr_weaponHand.GetInteger()]));
    lvrStatGui->SetStateBool("player_clip_empty", (clipEmpty[vr_weaponHand.GetInteger()]));
    lvrStatGui->SetStateBool("player_clip_low", (clipLow[vr_weaponHand.GetInteger()]));

    // koz todo

    // need to get the ammocount from the hand structures, not sure right now, in the interim just use the ammo amount.
    //Let the GUI know the total amount of ammo regardless of the ammo required value
    //rvrStatGui->SetStateString( "player_ammo_count", va( "%i", AmmoCount() ) );

    lvrStatGui->SetStateString("player_ammo_count", va("%i", ammoamount[vr_weaponHand.GetInteger()]));
    // koz end

    // koz todo also need to figure this out for dual guis
	//Grabber Gui Info
	lvrStatGui->SetStateString("grabber_state", va("%i", grabberState));
}

/*
================
Koz
idWeapon::UpdateWeaponClipPosition
================
*/
void idWeapon::UpdateWeaponClipPosition( idVec3 &origin, idMat3 &axis )
{

    /*PDAclipModel->SetPosition( origin, axis );
    PDAclipModel->Link( gameLocal.clip );
    */

}

/*
================
idWeapon::UpdateSkin
================
*/
bool idWeapon::UpdateSkin( void ) {
	const function_t *func;

	if ( !isLinked ) {
		return false;
	}

	func = scriptObject.GetFunction( "UpdateSkin" );
	if ( !func ) {
		common->Warning( "Can't find function 'UpdateSkin' in object '%s'", scriptObject.GetTypeName() );
		return false;
	}

	// use the frameCommandThread since it's safe to use outside of framecommands
	gameLocal.frameCommandThread->CallFunction( this, func, true );
	gameLocal.frameCommandThread->Execute();

	return true;
}

/*
================
idWeapon::SetModel
================
*/
void idWeapon::SetModel( const char *modelname ) {
	assert( modelname );

	if ( modelDefHandle >= 0 ) {
		gameRenderWorld->RemoveDecals( modelDefHandle );
	}

	renderEntity.hModel = animator.SetModel( modelname );
	if ( renderEntity.hModel ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
		animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	} else {
		renderEntity.customSkin = NULL;
		renderEntity.callback = NULL;
		renderEntity.numJoints = 0;
		renderEntity.joints = NULL;
	}

	// hide the model until an animation is played
	Hide();
}

/*
================
idWeapon::GetGlobalJointTransform

This returns the offset and axis of a weapon bone in world space, suitable for attaching models or lights
================
*/
bool idWeapon::GetGlobalJointTransform( bool viewModel, const jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis ) {
	if ( viewModel ) {
		// view model
		if ( animator.GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
			offset = offset * viewWeaponAxis + viewWeaponOrigin;
			axis = axis * viewWeaponAxis;
			return true;
		}
	} else {
		// world model
		if ( worldModel.GetEntity() && worldModel.GetEntity()->GetAnimator()->GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
			offset = worldModel.GetEntity()->GetPhysics()->GetOrigin() + offset * worldModel.GetEntity()->GetPhysics()->GetAxis();
			axis = axis * worldModel.GetEntity()->GetPhysics()->GetAxis();
			return true;
		}
	}
	offset = viewWeaponOrigin;
	axis = viewWeaponAxis;
	return false;
}

// Carl: dual wielding, check which hand, if any, the weapon is in
int idWeapon::GetHand()
{
    if ( !owner )
        return -1;
    for( int h = 0; h < 2; h++ )
    {
        if( owner->hands[ h ].weapon.GetEntity() == this )
            return h;
    }
    return -1;
}

/*
================
idWeapon::GetMuzzlePositionWithHacks

Some weapons that have a barrel joint either have it pointing in the wrong
direction (rocket launcher), or don't animate it properly (pistol).

For good 3D TV / head mounted display work, we need to display a laser sight
in the world.

Fixing the animated meshes would be ideal, but hacking it in code is
the pragmatic move right now.

Returns false for hands, grenades, and chainsaw.
================
*/
bool idWeapon::GetMuzzlePositionWithHacks( idVec3& origin, idMat3& axis )
{

    static weapon_t currentWeap = WEAPON_NONE;
    idVec3	discardedOrigin;

    origin = playerViewOrigin;
    axis = playerViewAxis;

    currentWeap = IdentifyWeapon();

    switch ( currentWeap )
    {
        case WEAPON_NONE:
        case WEAPON_FISTS:
        case WEAPON_SOULCUBE:
        case WEAPON_PDA:
            origin = commonVr->lastViewOrigin; // Koz fixme set the origin and axis to the players view
            axis = commonVr->lastViewAxis;
            return false;
            break;

        case WEAPON_HANDGRENADE:
            origin = viewWeaponOrigin; // Koz fixme set the origin and axis to the weapon default
            axis = viewWeaponAxis;
            return false;
            break;

        case WEAPON_CHAINSAW:
        {
            if ( barrelJointView == INVALID_JOINT )
            {
                origin = viewWeaponOrigin; // Koz fixme set the origin and axis to the weapon default
                axis = viewWeaponAxis;
                return false;
                break;
            }
            else
            {
                GetGlobalJointTransform( true, barrelJointView, origin, axis );
                std::swap( axis[0], axis[2] ); // barrel joint points right, rotate &
                axis[0] = -axis[0]; // make it point forward.

                // the barrel joint is not actually aligned with the blade of the chainsaw.
                // Move the origin to align with the tip of the blade.
                // fixme -34 forward is a hack to compensate for an overly long
                // melee range value in VR. Otherwise you can saw something 3 feet away
                // from the tip of the blade.
                // should probably change the weapon def instead of using this mess.
                idVec3 move( -34.0f, 2.0f, -6.0f ); // fwd,up,right
                origin += move * axis;

            }
            return false;
            break;
        }

    }

    if ( barrelJointView != INVALID_JOINT )
    {
        GetGlobalJointTransform( true, barrelJointView, origin, axis );
    }
    else if ( guiLightJointView != INVALID_JOINT )
    {
        GetGlobalJointTransform( true, guiLightJointView, origin, axis );
    }
    else
    {
        return false;
    }

    switch ( currentWeap )
    {
        case WEAPON_PISTOL:
        {
            // muzzle doesn't animate during firing, Bod does
            const jointHandle_t bodJoint = animator.GetJointHandle( "Bod" );
            GetGlobalJointTransform( true, bodJoint, discardedOrigin, axis );
            break;
        }

        case WEAPON_ROCKETLAUNCHER:
            // joint doesn't point straight, so rotate it
            std::swap( axis[0], axis[2] );
            break;

        case WEAPON_SHOTGUN:
        {
            // joint doesn't point straight, so rotate it
            //const jointHandle_t bodJoint = animator.GetJointHandle( "trigger" );
            const jointHandle_t bodJoint = animator.GetJointHandle( "body" );
            GetGlobalJointTransform( true, bodJoint, discardedOrigin, axis );
            std::swap( axis[0], axis[2] );
            axis[0] = -axis[0];
            break;
        }

		case WEAPON_SHOTGUN_DOUBLE:
		{
			// joint doesn't point straight, so rotate it
			//const jointHandle_t bodJoint = animator.GetJointHandle("trigger");
			//GetGlobalJointTransform(true, bodJoint, discardedOrigin, axis);
			std::swap( axis[0], axis[2] ); // Koz fixme just swap like the rocketlauncher this should work now test and cleanup
			//axis[0] = -axis[0];
			//common->Printf( "GMPWH returning value for WEAPON_SHOTGUN_DOUBLE \n" );

			break;
		}

		case WEAPON_GRABBER:
		{

			idVec3 forward = axis[0];
			forward.Normalize();
			const float scaleOffset = 4.0f;
			forward *= scaleOffset;
			origin += forward;
			break;

		}

        case WEAPON_PLASMAGUN:
        {
            // the barrel of the plasma rifle is angled down by default, bring it up a little so it shoots straight.
            const idMat3 adj = idAngles( 0.0f, 4.0f, 0.0f ).ToMat3();
            axis = adj * axis;
            break;
        }
    }

    return true;

}

/*
================
idWeapon::GetProjectileLaunchOriginAndAxis
================
*/
void idWeapon::GetProjectileLaunchOriginAndAxis( idVec3& origin, idMat3& axis )
{
    assert( owner != NULL );
    if ( game->isVR )
    {
        static weapon_t curWeap = WEAPON_NONE;

        // Koz debug common->Printf( "GPLOAA getting muzzle position w/ hacks\n" ); // Koz delete me
        GetMuzzlePositionWithHacks( origin, axis );

        curWeap = IdentifyWeapon();

        switch ( curWeap )
        {
            case WEAPON_BFG:
            {
                // BFG pitches up when fired before the projectile is launched.
                // Hack to correct pitch so projectile doesn't shoot high.
                static idMat3 bfgCorrection = idAngles( 0.0, -15.0, 0.0 ).ToMat3();
                axis = bfgCorrection * axis;

            }
                break;

            case WEAPON_HANDGRENADE:
            {
                // if using motion controls, the muzzle axis should be the tracked direction of
                // hand movement, not the barrel axis. (unless the controller is mounted on something like a topshot, then you have a grenade launcher.)
                if ( commonVr->VR_USE_MOTION_CONTROLS && !vr_mountedWeaponController.GetBool() )
                {
                    axis = owner->hands[GetHand()].throwDirection.ToMat3();
                    break;
                }
            }
            default:
                break;

        }
        return;
    }

    // calculate the muzzle position
    if( barrelJointView != INVALID_JOINT && projectileDict.GetBool( "launchFromBarrel" ) )
    {
        // there is an explicit joint for the muzzle
        // GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
        GetMuzzlePositionWithHacks( origin, axis );
    }
    else
    {
        // go straight out of the view
        origin = playerViewOrigin;
        axis = playerViewAxis;
    }

    axis = playerViewAxis;	// Fix for plasma rifle not firing correctly on initial shot of a burst fire
}

/*
================
idWeapon::SetPushVelocity
================
*/
void idWeapon::SetPushVelocity( const idVec3 &pushVelocity ) {
	this->pushVelocity = pushVelocity;
}


/***********************************************************************

	State control/player interface

***********************************************************************/

/*
================
idWeapon::Think
================
*/
void idWeapon::Think( void ) {
	// do nothing because the present is called from the player through PresentWeapon
}

/*
================
idWeapon::Raise
================
*/
void idWeapon::Raise( void ) {
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = true;
	}
}

/*
================
idWeapon::PutAway
================
*/
void idWeapon::PutAway( void ) {
	hasBloodSplat = false;
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = true;
	}
}

/*
================
idWeapon::Reload
NOTE: this is only for impulse-triggered reload, auto reload is scripted
================
*/
void idWeapon::Reload( void ) {
	if ( isLinked ) {
		WEAPON_RELOAD = true;
	}
}

/*
================
idWeapon::LowerWeapon
================
*/
void idWeapon::LowerWeapon( void ) {

    if ( game->isVR && commonVr->handInGui ) return;// Koz never lower weapon if hand is in gui
    if ( !owner->GuiActive() && !gameLocal.inCinematic ) return;

    if ( !hide ) {
		hideStart	= 0.0f;
		hideEnd		= hideDistance;
		if ( gameLocal.time - hideStartTime < hideTime ) {
			hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
		} else {
			hideStartTime = gameLocal.time;
		}
		hide = true;
	}
}

/*
================
idWeapon::RaiseWeapon
================
*/
void idWeapon::RaiseWeapon( void ) {
	Show();

	if ( hide ) {
		hideStart	= hideDistance;
		hideEnd		= 0.0f;
		if ( gameLocal.time - hideStartTime < hideTime ) {
			hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
		} else {
			hideStartTime = gameLocal.time;
		}
		hide = false;
	}
}

/*
================
idWeapon::HideWeapon
================
*/
void idWeapon::HideWeapon( void ) {
	Hide();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
	muzzleFlashEnd = 0;
}

/*
================
idWeapon::ShowWeapon
================
*/
void idWeapon::ShowWeapon( void ) {
	Show();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}
	if ( lightOn ) {
		MuzzleFlashLight();
	}
}

/*
================
idWeapon::HideWorldModel
================
*/
void idWeapon::HideWorldModel( void ) {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
}

/*
================
idWeapon::ShowWorldModel
================
*/
void idWeapon::ShowWorldModel( void ) {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}
}

/*
================
idWeapon::OwnerDied
================
*/
void idWeapon::OwnerDied( void ) {
	if ( isLinked ) {
		SetState( "OwnerDied", 0 );
		thread->Execute();

		// Update the grabber effects
		if( grabberState != -1 )
		{
			grabber.Update( owner, this, hide );
		}
	}

	Hide();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}

	// don't clear the weapon immediately since the owner might have killed himself by firing the weapon
	// within the current stack frame
	PostEventMS( &EV_Weapon_Clear, 0 );
}

/*
================
idWeapon::BeginAttack
================
*/
void idWeapon::BeginAttack( void ) {
	if ( status != WP_OUTOFAMMO ) {
		lastAttack = gameLocal.time;
	}

	if ( !isLinked ) {
		return;
	}

	if ( !WEAPON_ATTACK ) {
		if ( sndHum && grabberState == -1 )  {   	// _D3XP :: don't stop grabber hum
			StopSound( SND_CHANNEL_BODY, false );
		}
	}
	WEAPON_ATTACK = true;

	int position = vr_weaponHand.GetInteger() ? 1 : 2;

	vrClientInfo *pVRClientInfo = owner->GetVRClientInfo();

    weapon_t currentWeapon = WEAPON_NONE;
    currentWeapon = IdentifyWeapon();
    if (currentWeapon == WEAPON_HANDGRENADE)
    {
        common->HapticEvent("handgrenade_init", position, 0, 100, 0, 0);
    }
    if (currentWeapon == WEAPON_CHAINGUN)
    {
		position = commonVr->GetWeaponStabilised() ? 4 : position;
		common->HapticEvent("chaingun_init", position, 0, 100, 0, 0);
    }
    if (currentWeapon == WEAPON_BFG)
    {
        common->HapticEvent("bfg_init", position, 0, 100, 0, 0);
    }
}

/*
================
idWeapon::EndAttack
================
*/
void idWeapon::EndAttack( void ) {
	if ( !WEAPON_ATTACK.IsLinked() ) {
		return;
	}
	if ( WEAPON_ATTACK ) {
		WEAPON_ATTACK = false;
		if( sndHum && grabberState == -1 )  {	// _D3XP :: don't stop grabber hum
			StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
		}

		weapon_t currentWeapon = WEAPON_NONE;
		currentWeapon = IdentifyWeapon();
		if (currentWeapon == WEAPON_CHAINSAW)
		{
			common->HapticEvent("chainsaw_idle", vr_weaponHand.GetInteger() ? 1 : 2, 1, 100, 0, 0);
		}
	}
}

/*
================
idWeapon::isReady
================
*/
bool idWeapon::IsReady( void ) const {
	return !hide && !IsHidden() && ( ( status == WP_RELOAD ) || ( status == WP_READY ) || ( status == WP_OUTOFAMMO ) );
}

/*
================
idWeapon::IsReloading
================
*/
bool idWeapon::IsReloading( void ) const {
	return ( status == WP_RELOAD );
}

/*
================
idWeapon::IsHolstered
================
*/
bool idWeapon::IsHolstered( void ) const {
	return ( status == WP_HOLSTERED );
}

/*
================
idWeapon::ShowCrosshair
================
*/
bool idWeapon::ShowCrosshair( void ) const {
	return !( state == idStr( WP_RISING ) || state == idStr( WP_LOWERING ) || state == idStr( WP_HOLSTERED ) );
}

/*
=====================
idWeapon::CanDrop
=====================
*/
bool idWeapon::CanDrop( void ) const {
	if ( !weaponDef || !worldModel.GetEntity() ) {
		return false;
	}
	const char *classname = weaponDef->dict.GetString( "def_dropItem" );
	if ( !classname[ 0 ] ) {
		return false;
	}
	return true;
}

/*
================
idWeapon::WeaponStolen
================
*/
void idWeapon::WeaponStolen( void ) {
	assert( !gameLocal.isClient );
	if ( projectileEnt ) {
		if ( isLinked ) {
			SetState( "WeaponStolen", 0 );
			thread->Execute();
		}
		projectileEnt = NULL;
	}

	// set to holstered so we can switch weapons right away
	status = WP_HOLSTERED;

	HideWeapon();
}

/*
=====================
idWeapon::DropItem
=====================
*/
idEntity * idWeapon::DropItem( const idVec3 &velocity, int activateDelay, int removeDelay, bool died ) {
	if ( !weaponDef || !worldModel.GetEntity() ) {
		return NULL;
	}
	if ( !allowDrop ) {
		return NULL;
	}
	const char *classname = weaponDef->dict.GetString( "def_dropItem" );
	if ( !classname[0] ) {
		return NULL;
	}
	StopSound( SND_CHANNEL_BODY, true );
	StopSound( SND_CHANNEL_BODY3, true );

	return idMoveableItem::DropItem( classname, worldModel.GetEntity()->GetPhysics()->GetOrigin(), worldModel.GetEntity()->GetPhysics()->GetAxis(), velocity, activateDelay, removeDelay );
}

/***********************************************************************

	Script state management

***********************************************************************/

/*
=====================
idWeapon::SetState
=====================
*/
void idWeapon::SetState( const char *statename, int blendFrames ) {
	const function_t *func;

	if ( !isLinked ) {
		return;
	}

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	thread->CallFunction( this, func, true );
	state = statename;

	animBlendFrames = blendFrames;
	if ( g_debugWeapon.GetBool() ) {
		gameLocal.Printf( "%d: weapon state : %s\n", gameLocal.time, statename );
	}

	idealState = "";
}


/***********************************************************************

	Particles/Effects

***********************************************************************/

/*
================
idWeapon::UpdateNozzelFx
================
*/
void idWeapon::UpdateNozzleFx( void ) {
	if ( !nozzleFx ) {
		return;
	}

	//
	// shader parms
	//
	int la = gameLocal.time - lastAttack + 1;
	float s = 1.0f;
	float l = 0.0f;
	if ( la < nozzleFxFade ) {
		s = ((float)la / nozzleFxFade);
		l = 1.0f - s;
	}
	renderEntity.shaderParms[5] = s;
	renderEntity.shaderParms[6] = l;

	if ( ventLightJointView == INVALID_JOINT ) {
		return;
	}

	//
	// vent light
	//
	if ( nozzleGlowHandle == -1 ) {
		memset(&nozzleGlow, 0, sizeof(nozzleGlow));
		if ( owner ) {
			nozzleGlow.allowLightInViewID = owner->entityNumber+1;
		}
		nozzleGlow.pointLight = true;
		nozzleGlow.noShadows = true;
		nozzleGlow.lightRadius.x = nozzleGlowRadius;
		nozzleGlow.lightRadius.y = nozzleGlowRadius;
		nozzleGlow.lightRadius.z = nozzleGlowRadius;
		nozzleGlow.shader = nozzleGlowShader;
		nozzleGlow.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
		nozzleGlow.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
		GetGlobalJointTransform( true, ventLightJointView, nozzleGlow.origin, nozzleGlow.axis );
		nozzleGlowHandle = gameRenderWorld->AddLightDef(&nozzleGlow);
	}

	GetGlobalJointTransform( true, ventLightJointView, nozzleGlow.origin, nozzleGlow.axis );

	nozzleGlow.shaderParms[ SHADERPARM_RED ] = nozzleGlowColor.x * s;
	nozzleGlow.shaderParms[ SHADERPARM_GREEN ] = nozzleGlowColor.y * s;
	nozzleGlow.shaderParms[ SHADERPARM_BLUE ] = nozzleGlowColor.z * s;
	gameRenderWorld->UpdateLightDef(nozzleGlowHandle, &nozzleGlow);
}


/*
================
idWeapon::BloodSplat
================
*/
bool idWeapon::BloodSplat( float size ) {
	float s, c;
	idMat3 localAxis, axistemp;
	idVec3 localOrigin, normal;

	if ( hasBloodSplat ) {
		return true;
	}

	hasBloodSplat = true;

	if ( modelDefHandle < 0 ) {
		return false;
	}

	if ( !GetGlobalJointTransform( true, ejectJointView, localOrigin, localAxis ) ) {
		return false;
	}

	localOrigin[0] += gameLocal.random.RandomFloat() * -10.0f;
	localOrigin[1] += gameLocal.random.RandomFloat() * 1.0f;
	localOrigin[2] += gameLocal.random.RandomFloat() * -2.0f;

	normal = idVec3( gameLocal.random.CRandomFloat(), -gameLocal.random.RandomFloat(), -1 );
	normal.Normalize();

	idMath::SinCos16( gameLocal.random.RandomFloat() * idMath::TWO_PI, s, c );

	localAxis[2] = -normal;
	localAxis[2].NormalVectors( axistemp[0], axistemp[1] );
	localAxis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	localAxis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	localAxis[0] *= 1.0f / size;
	localAxis[1] *= 1.0f / size;

	idPlane		localPlane[2];

	localPlane[0] = localAxis[0];
	localPlane[0][3] = -(localOrigin * localAxis[0]) + 0.5f;

	localPlane[1] = localAxis[1];
	localPlane[1][3] = -(localOrigin * localAxis[1]) + 0.5f;

	const idMaterial *mtr = declManager->FindMaterial( "textures/decals/duffysplatgun" );

	gameRenderWorld->ProjectOverlay( modelDefHandle, localPlane, mtr );

	return true;
}


/***********************************************************************

	Visual presentation

***********************************************************************/

/*
================
idWeapon::MuzzleRise

The machinegun and chaingun will incrementally back up as they are being fired
================
*/
void idWeapon::MuzzleRise( idVec3 &origin, idMat3 &axis ) {
	int			time;
	float		amount;
	idAngles	ang;
	idVec3		offset;

	time = kick_endtime - gameLocal.time;
	if ( time <= 0 ) {
		return;
	}

	if ( muzzle_kick_maxtime <= 0 ) {
		return;
	}

	if ( time > muzzle_kick_maxtime ) {
		time = muzzle_kick_maxtime;
	}

	amount = ( float )time / ( float )muzzle_kick_maxtime;
	ang		= muzzle_kick_angles * amount;
	offset	= muzzle_kick_offset * amount;

	origin = origin - axis * offset;
	axis = ang.ToMat3() * axis;
}

/*
================
idWeapon::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *idWeapon::ConstructScriptObject( void ) {
	const function_t *constructor;

	thread->EndThread();

	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for weapon", scriptObject.GetTypeName() );
	}

	// init the script object's data
	scriptObject.ClearObject();
	thread->CallFunction( this, constructor, true );
	thread->Execute();

	return thread;
}

/*
================
idWeapon::DeconstructScriptObject

Called during idEntity::~idEntity.  Calls the destructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
Not called during idGameLocal::MapShutdown.
================
*/
void idWeapon::DeconstructScriptObject( void ) {
	const function_t *destructor;

	if ( !thread ) {
		return;
	}

	// don't bother calling the script object's destructor on map shutdown
	if ( gameLocal.GameState() == GAMESTATE_SHUTDOWN ) {
		return;
	}

	thread->EndThread();

	// call script object's destructor
	destructor = scriptObject.GetDestructor();
	if ( destructor ) {
		// start a thread that will run immediately and end
		thread->CallFunction( this, destructor, true );
		thread->Execute();
		thread->EndThread();
	}

	// clear out the object's memory
	scriptObject.ClearObject();
}

/*
================
idWeapon::UpdateScript
================
*/
void idWeapon::UpdateScript( void ) {
	int	count;

	if ( !isLinked ) {
		return;
	}

	// only update the script on new frames
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( idealState.Length() ) {
		SetState( idealState, animBlendFrames );
	}

	// update script state, which may call Event_LaunchProjectiles, among other things
	count = 10;
	while( ( thread->Execute() || idealState.Length() ) && count-- ) {
		// happens for weapons with no clip (like grenades)
		if ( idealState.Length() ) {
			SetState( idealState, animBlendFrames );
		}
	}

	WEAPON_RELOAD = false;
}

/*
================
idWeapon::AlertMonsters
================
*/
void idWeapon::AlertMonsters( void ) {
	trace_t	tr;
	idEntity *ent;
	idVec3 end = muzzleFlash.origin + muzzleFlash.axis * muzzleFlash.target;

	gameLocal.clip.TracePoint( tr, muzzleFlash.origin, end, CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL | CONTENTS_FLASHLIGHT_TRIGGER, owner );
	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, muzzleFlash.origin, end, 0 );
		gameRenderWorld->DebugArrow( colorGreen, muzzleFlash.origin, tr.endpos, 2, 0 );
	}

	if ( tr.fraction < 1.0f ) {
		ent = gameLocal.GetTraceEntity( tr );
		if ( ent->IsType( idAI::Type ) ) {
			static_cast<idAI *>( ent )->TouchedByFlashlight( owner );
		} else if ( ent->IsType( idTrigger::Type ) ) {
			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, owner, &tr );
		}
	}

	// jitter the trace to try to catch cases where a trace down the center doesn't hit the monster
	end += muzzleFlash.axis * muzzleFlash.right * idMath::Sin16( MS2SEC( gameLocal.time ) * 31.34f );
	end += muzzleFlash.axis * muzzleFlash.up * idMath::Sin16( MS2SEC( gameLocal.time ) * 12.17f );
	gameLocal.clip.TracePoint( tr, muzzleFlash.origin, end, CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL | CONTENTS_FLASHLIGHT_TRIGGER, owner );
	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, muzzleFlash.origin, end, 0 );
		gameRenderWorld->DebugArrow( colorGreen, muzzleFlash.origin, tr.endpos, 2, 0 );
	}

	if ( tr.fraction < 1.0f ) {
		ent = gameLocal.GetTraceEntity( tr );
		if ( ent->IsType( idAI::Type ) ) {
			static_cast<idAI *>( ent )->TouchedByFlashlight( owner );
		} else if ( ent->IsType( idTrigger::Type ) ) {
			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, owner, &tr );
		}
	}
}

/*
================
idWeapon::CalculateHideRise
================
*/
void idWeapon::CalculateHideRise( idVec3& origin, idMat3& axis )
{
// hide offset is for dropping the gun when approaching a GUI or NPC
// This is simpler to manage than doing the weapon put-away animation
	if ( gameLocal.time - hideStartTime < hideTime )
	{
		float frac = (float)(gameLocal.time - hideStartTime) / (float)hideTime;
		if ( hideStart < hideEnd )
		{
			frac = 1.0f - frac;
			frac = 1.0f - frac * frac;
		}
		else
		{
			frac = frac * frac;
		}
		hideOffset = hideStart + (hideEnd - hideStart) * frac;
	}
	else
	{
		hideOffset = hideEnd;
		if ( hide && disabled )
		{
			Hide();
		}
	}
	//viewWeaponOrigin += hideOffset * viewWeaponAxis[2]; // Koz adjust the gui pointer by this offset later
	//viewWeaponOrigin.z += hideOffset;
	origin.z += hideOffset;
	// kick up based on repeat firing
	MuzzleRise( origin, axis );
}

/*
================
idWeapon::PresentWeapon
================
*/
void idWeapon::PresentWeapon( bool showViewModel, int hand ) {

    worldModel.GetEntity()->GetRenderEntity()->allowSurfaceInViewID = -1;

    playerViewOrigin = owner->firstPersonViewOrigin;
	playerViewAxis = owner->firstPersonViewAxis;
    weapon_t currentWeapon = WEAPON_NONE;
    currentWeapon = IdentifyWeapon();

    if ( isPlayerFlashlight || (hand != vr_weaponHand.GetInteger() && owner->hands[hand].idealWeapon != owner->weapon_pda))
    {
        viewWeaponOrigin = playerViewOrigin;
        viewWeaponAxis = playerViewAxis;
        owner->CalculateViewFlashlightPos( viewWeaponOrigin, viewWeaponAxis, flashlightOffsets[owner->hands[vr_weaponHand.GetInteger()].currentWeapon] );
    }
    else
    {
        // calculate weapon position based on player movement bobbing
        owner->CalculateViewWeaponPos( hand, viewWeaponOrigin, viewWeaponAxis );
        // Koz hide weapon and muzzlerise was here, now called as weapon::CalculateHideRise in player->CalculateViewWeaponPosition to allow hand animations
    }

	//HACKADOODLE-DOOO!
	idMat3 axis = viewWeaponAxis;
	if (currentWeapon == WEAPON_CHAINSAW)
	{
		axis = idAngles(45, 0, 0).ToMat3() * viewWeaponAxis;
	}

    // set the physics position and orientation
    GetPhysics()->SetOrigin( viewWeaponOrigin );
    GetPhysics()->SetAxis( axis );

    UpdateVisuals();

    // update the weapon script
    UpdateScript();

    UpdateGUI();

    // update animation
    UpdateAnimation();

    // in VR don't suppress drawing the player's body
    // also show the viewmodel
    if ( (hide && disabled) ) // hide the weapon if in a cinematic
    {
        renderEntity.allowSurfaceInViewID = -1;
        showViewModel = false;
    }    //show the viewmodel in all views - flashlight visibilty set in calcViewFlashPosition
    else if (!isPlayerFlashlight && !commonVr->handInGui  ) renderEntity.allowSurfaceInViewID = 0; // Koz fixme
    owner->GetRenderEntity()->suppressSurfaceInViewID = 0;

    // crunch the depth range so it never pokes into walls this breaks the machine gun gui
    renderEntity.weaponDepthHack = g_useWeaponDepthHack.GetBool();

    // present the model
    if ( showViewModel )
    {
        Present();
    }
    else
    {
        FreeModelDef();
    }

    if ( worldModel.GetEntity() && worldModel.GetEntity()->GetRenderEntity() )
    {
        // deal with the third-person visible world model
        // don't show shadows of the world model in first person
        if ( g_showPlayerShadow.GetBool() || pm_thirdPerson.GetBool() || game->isVR ) // Koz fixme only in vr
        {
            worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID = 0;
        }
        else
        {
            worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID = owner->entityNumber + 1;
            worldModel.GetEntity()->GetRenderEntity()->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
        }
    }

    if ( nozzleFx )
    {
        UpdateNozzleFx();
    }

    // muzzle smoke
    if ( showViewModel && !disabled && weaponSmoke && (weaponSmokeStartTime != 0) )
    {
        // use the barrel joint if available

        if ( smokeJointView != INVALID_JOINT )
        {
            GetGlobalJointTransform( true, smokeJointView, muzzleOrigin, muzzleAxis );
        }
        else if ( barrelJointView != INVALID_JOINT )
        {
            GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
        }
        else
        {
            // default to going straight out the view
            muzzleOrigin = playerViewOrigin;
            muzzleAxis = playerViewAxis;
        }
        // spit out a particle
        if ( !gameLocal.smokeParticles->EmitSmoke( weaponSmoke, weaponSmokeStartTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis ) )
        {
            weaponSmokeStartTime = (continuousSmoke) ? gameLocal.time : 0;
        }
    }

    if ( showViewModel && strikeSmoke && strikeSmokeStartTime != 0 )
    {
        // spit out a particle
        if ( !gameLocal.smokeParticles->EmitSmoke( strikeSmoke, strikeSmokeStartTime, gameLocal.random.RandomFloat(), strikePos, strikeAxis ) )
        {
            strikeSmokeStartTime = 0;
        }
    }

    if ( showViewModel && !hide )
    {
        for ( int i = 0; i < weaponParticles.Num(); i++ )
        {
            WeaponParticle_t* part = weaponParticles.GetIndex( i );

            if ( part->active )
            {
                if ( part->smoke )
                {
                    if ( part->joint != INVALID_JOINT )
                    {
                        GetGlobalJointTransform( true, part->joint, muzzleOrigin, muzzleAxis );
                    }
                    else
                    {
                        // default to going straight out the view
                        muzzleOrigin = playerViewOrigin;
                        muzzleAxis = playerViewAxis;
                    }
                    if ( !gameLocal.smokeParticles->EmitSmoke( part->particle, part->startTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis) )
                    {
                        part->active = false;	// all done
                        part->startTime = 0;
                    }
                }
                else
                {
                    if ( part->emitter != NULL )
                    {
                        //Manually update the position of the emitter so it follows the weapon
                        renderEntity_t* rendEnt = part->emitter->GetRenderEntity();
                        GetGlobalJointTransform( true, part->joint, rendEnt->origin, rendEnt->axis );

                        if ( part->emitter->GetModelDefHandle() != -1 )
                        {
                            gameRenderWorld->UpdateEntityDef( part->emitter->GetModelDefHandle(), rendEnt );
                        }
                    }
                }
            }
        }

        for ( int i = 0; i < weaponLights.Num(); i++ )
        {
            WeaponLight_t* light = weaponLights.GetIndex( i );

            if ( light->active )
            {

                GetGlobalJointTransform( true, light->joint, light->light.origin, light->light.axis );
                if ( (light->lightHandle != -1) )
                {
                    gameRenderWorld->UpdateLightDef( light->lightHandle, &light->light );
                }
                else
                {
                    light->lightHandle = gameRenderWorld->AddLightDef( &light->light );
                }
            }
        }
    }

	// Update the grabber effects
	if( grabberState != -1 )
	{
		grabberState = grabber.Update( owner, this, hide );
	}

    // remove the muzzle flash light when it's done
    if ( (!lightOn && (gameLocal.time >= muzzleFlashEnd)) || IsHidden() )
    {
        if ( muzzleFlashHandle != -1 )
        {
            gameRenderWorld->FreeLightDef( muzzleFlashHandle );
            muzzleFlashHandle = -1;
        }
        if ( worldMuzzleFlashHandle != -1 )
        {
            gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
            worldMuzzleFlashHandle = -1;
        }
    }

    // update the muzzle flash light, so it moves with the gun
    if ( muzzleFlashHandle != -1 )
    {
        UpdateFlashPosition();
        gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
        gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );

        // wake up monsters with the flashlight
        if ( lightOn && !owner->fl.notarget )
        {
            AlertMonsters();
        }
    }

    // update the gui light
    if ( guiLight.lightRadius[0] && guiLightJointView != INVALID_JOINT )
    {
        GetGlobalJointTransform( true, guiLightJointView, guiLight.origin, guiLight.axis );

        if ( (guiLightHandle != -1) )
        {
            gameRenderWorld->UpdateLightDef( guiLightHandle, &guiLight );
        }
        else
        {
            guiLightHandle = gameRenderWorld->AddLightDef( &guiLight );
        }
    }

    if ( status != WP_READY && sndHum )
    {
        StopSound( SND_CHANNEL_BODY, false );
    }

    UpdateSound();

    // constant rumble...
    float highMagnitude = weaponDef->dict.GetFloat( "controllerConstantShakeHighMag" );
    int highDuration = weaponDef->dict.GetInt( "controllerConstantShakeHighTime" );
    float lowMagnitude = weaponDef->dict.GetFloat( "controllerConstantShakeLowMag" );
    int lowDuration = weaponDef->dict.GetInt( "controllerConstantShakeLowTime" );

    if ( vr_rumbleChainsaw.GetBool() || !commonVr->VR_USE_MOTION_CONTROLS )
        owner->hands[hand].SetControllerShake( highMagnitude, highDuration, lowMagnitude, lowDuration );
}

/*
================
idWeapon::EnterCinematic
================
*/
void idWeapon::EnterCinematic( void ) {
	StopSound( SND_CHANNEL_ANY, false );

	if ( isLinked ) {
		SetState( "EnterCinematic", 0 );
		thread->Execute();

		WEAPON_ATTACK		= false;
		WEAPON_RELOAD		= false;
		WEAPON_NETRELOAD	= false;
		WEAPON_NETENDRELOAD	= false;
		if(WEAPON_NETFIRING.IsLinked())
			WEAPON_NETFIRING	= false;
		WEAPON_RAISEWEAPON	= false;
		WEAPON_LOWERWEAPON	= false;
	}

	disabled = true;

	LowerWeapon();
}

/*
================
idWeapon::ExitCinematic
================
*/
void idWeapon::ExitCinematic( void ) {
	disabled = false;

	if ( isLinked ) {
		SetState( "ExitCinematic", 0 );
		thread->Execute();
	}

	RaiseWeapon();
}

/*
================
idWeapon::NetCatchup
================
*/
void idWeapon::NetCatchup( void ) {
	if ( isLinked ) {
		SetState( "NetCatchup", 0 );
		thread->Execute();
	}
}

/*
================
idWeapon::GetZoomFov
================
*/
int	idWeapon::GetZoomFov( void ) {
	return zoomFov;
}

/*
================
idWeapon::GetWeaponAngleOffsets
================
*/
void idWeapon::GetWeaponAngleOffsets( int *average, float *scale, float *max ) {
	*average = weaponAngleOffsetAverages;
	*scale = weaponAngleOffsetScale;
	*max = weaponAngleOffsetMax;
}

/*
================
idWeapon::GetWeaponTimeOffsets
================
*/
void idWeapon::GetWeaponTimeOffsets( float *time, float *scale ) {
	*time = weaponOffsetTime;
	*scale = weaponOffsetScale;
}


/***********************************************************************

	Ammo

***********************************************************************/

/*
================
idWeapon::GetAmmoNumForName
================
*/
ammo_t idWeapon::GetAmmoNumForName( const char *ammoname ) {
	int num;
	const idDict *ammoDict;

	assert( ammoname );

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( !ammoDict ) {
		return 0;
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
	}

	if ( !ammoname[ 0 ] ) {
		return 0;
	}

	if ( !ammoDict->GetInt( ammoname, "-1", num ) ) {
		gameLocal.Error( "Unknown ammo type '%s'", ammoname );
	}

	if ( ( num < 0 ) || ( num >= AMMO_NUMTYPES ) ) {
		gameLocal.Error( "Ammo type '%s' value out of range.  Maximum ammo types is %d.\n", ammoname, AMMO_NUMTYPES );
	}

	return ( ammo_t )num;
}

/*
================
idWeapon::GetAmmoNameForNum
================
*/
const char *idWeapon::GetAmmoNameForNum( ammo_t ammonum ) {
	int i;
	int num;
	const idDict *ammoDict;
	const idKeyValue *kv;
	char text[ 32 ];

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
	}

	sprintf( text, "%d", ammonum );

	num = ammoDict->GetNumKeyVals();
	for( i = 0; i < num; i++ ) {
		kv = ammoDict->GetKeyVal( i );
		if ( kv->GetValue() == text ) {
			return kv->GetKey();
		}
	}

	return NULL;
}

/*
================
idWeapon::GetAmmoPickupNameForNum
================
*/
const char *idWeapon::GetAmmoPickupNameForNum( ammo_t ammonum ) {
	int i;
	int num;
	const idDict *ammoDict;
	const idKeyValue *kv;

	ammoDict = gameLocal.FindEntityDefDict( "ammo_names", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_names'\n" );
	}

	const char *name = GetAmmoNameForNum( ammonum );

	if ( name && *name ) {
		num = ammoDict->GetNumKeyVals();
		for( i = 0; i < num; i++ ) {
			kv = ammoDict->GetKeyVal( i );
			if ( idStr::Icmp( kv->GetKey(), name) == 0 ) {
				return kv->GetValue();
			}
		}
	}

	return "";
}

/*
================
idWeapon::AmmoAvailable
================
*/
int idWeapon::AmmoAvailable( void ) const {
	if ( owner ) {
		return owner->inventory.HasAmmo( ammoType, ammoRequired );
	} else {
		if( g_infiniteAmmo.GetBool() )
		{
			return 10;	// arbitrary number, just so whatever's calling thinks there's sufficient ammo...
		}
		else
		{
			return 0;
		}
	}
}
/*
================
idWeapon::AmmoCount

Returns the total number of rounds regardless of the required ammo
================
*/
int idWeapon::AmmoCount() const
{

	if( owner )
	{
		return owner->inventory.HasAmmo( ammoType, 1 );
	}
	else
	{
		return 0;
	}
}

/*
================
idWeapon::AmmoInClip
================
*/
int idWeapon::AmmoInClip( void ) const {
	return ammoClip;
}

/*
================
idWeapon::ResetAmmoClip
================
*/
void idWeapon::ResetAmmoClip( void ) {
	ammoClip = -1;
}

/*
================
idWeapon::GetAmmoType
================
*/
ammo_t idWeapon::GetAmmoType( void ) const {
	return ammoType;
}

/*
================
idWeapon::ClipSize
================
*/
int	idWeapon::ClipSize( void ) const {
	return clipSize;
}

/*
================
idWeapon::LowAmmo
================
*/
int	idWeapon::LowAmmo() const {
	return lowAmmo;
}

/*
================
idWeapon::AmmoRequired
================
*/
int	idWeapon::AmmoRequired( void ) const {
	return ammoRequired;
}

/*
================
idWeapon::GetGrabberState

Returns the current grabberState
================
*/
int idWeapon::GetGrabberState() const
{
	return grabberState;
}

/*
================
idWeapon::WriteToSnapshot
================
*/
void idWeapon::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( ammoClip, ASYNC_PLAYER_INV_CLIP_BITS );
	msg.WriteBits( worldModel.GetSpawnId(), 32 );
	msg.WriteBits( lightOn, 1 );
	msg.WriteBits( isFiring ? 1 : 0, 1 );
}

/*
================
idWeapon::ReadFromSnapshot
================
*/
void idWeapon::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	ammoClip = msg.ReadBits( ASYNC_PLAYER_INV_CLIP_BITS );
	worldModel.SetSpawnId( msg.ReadBits( 32 ) );
	bool snapLight = msg.ReadBits( 1 ) != 0;
	isFiring = msg.ReadBits( 1 ) != 0;

	// WEAPON_NETFIRING is only turned on for other clients we're predicting. not for local client
	if ( owner && gameLocal.localClientNum != owner->entityNumber && WEAPON_NETFIRING.IsLinked() ) {

		// immediately go to the firing state so we don't skip fire animations
		if ( !WEAPON_NETFIRING && isFiring ) {
			idealState = "Fire";
		}

		// immediately switch back to idle
		if ( WEAPON_NETFIRING && !isFiring ) {
			idealState = "Idle";
		}

		WEAPON_NETFIRING = isFiring;
	}

	if ( snapLight != lightOn ) {
		Reload();
	}
}

/*
================
idWeapon::RemoveMuzzleFlashlight
================
*/
void idWeapon::RemoveMuzzleFlashlight()
{
    if( muzzleFlashHandle != -1 )
    {
        gameRenderWorld->FreeLightDef( muzzleFlashHandle );
        muzzleFlashHandle = -1;
    }
    if( worldMuzzleFlashHandle != -1 )
    {
        gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
        worldMuzzleFlashHandle = -1;
    }
}

/*
================
idWeapon::ClientReceiveEvent
================
*/
bool idWeapon::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_RELOAD: {
			if ( gameLocal.time - time < 1000 ) {
				if ( WEAPON_NETRELOAD.IsLinked() ) {
					WEAPON_NETRELOAD = true;
					WEAPON_NETENDRELOAD = false;
				}
			}
			return true;
		}
		case EVENT_ENDRELOAD: {
			if ( WEAPON_NETENDRELOAD.IsLinked() ) {
				WEAPON_NETENDRELOAD = true;
			}
			return true;
		}
		case EVENT_CHANGESKIN: {
			int index = gameLocal.ClientRemapDecl( DECL_SKIN, msg.ReadInt() );
			renderEntity.customSkin = ( index != -1 ) ? static_cast<const idDeclSkin *>( declManager->DeclByIndex( DECL_SKIN, index ) ) : NULL;
			UpdateVisuals();
			if ( worldModel.GetEntity() ) {
				worldModel.GetEntity()->SetSkin( renderEntity.customSkin );
			}
			return true;
		}
		default:
			break;
	}

	return idEntity::ClientReceiveEvent( event, time, msg );
}

/***********************************************************************

	Script events

***********************************************************************/

/*
===============
idWeapon::Event_Clear
===============
*/
void idWeapon::Event_Clear( void ) {
	Clear();
}

/*
===============
idWeapon::Event_GetOwner
===============
*/
void idWeapon::Event_GetOwner( void ) {
	idThread::ReturnEntity( owner );
}

/*
===============
idWeapon::Event_WeaponState
===============
*/
void idWeapon::Event_WeaponState( const char *statename, int blendFrames ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	idealState = statename;

	weapon_t currentWeap = IdentifyWeapon();
	if ( !idealState.Icmp( "Fire" ) ) {
		isFiring = true;

	} else {
		isFiring = false;
	}

	animBlendFrames = blendFrames;
	thread->DoneProcessing();
}

/*
================
idWeapon::FlashlightOn
================
*/
void idWeapon::FlashlightOn()
{
    const function_t* func;

    if( !isLinked )
    {
        return;
    }

    func = scriptObject.GetFunction( "TurnOn" );
    if( !func )
    {
        common->Warning( "Can't find function 'TurnOn' in object '%s'", scriptObject.GetTypeName() );
        return;
    }

    // use the frameCommandThread since it's safe to use outside of framecommands
    gameLocal.frameCommandThread->CallFunction( this, func, true );
    gameLocal.frameCommandThread->Execute();

    return;
}


/*
================
idWeapon::FlashlightOff
================
*/
void idWeapon::FlashlightOff()
{
    const function_t* func;

    if( !isLinked )
    {
        return;
    }

    func = scriptObject.GetFunction( "TurnOff" );
    if( !func )
    {
        common->Warning( "Can't find function 'TurnOff' in object '%s'", scriptObject.GetTypeName() );
        return;
    }

    // use the frameCommandThread since it's safe to use outside of framecommands
    gameLocal.frameCommandThread->CallFunction( this, func, true );
    gameLocal.frameCommandThread->Execute();

    return;
}

/*
===============
idWeapon::Event_WeaponReady
===============
*/
void idWeapon::Event_WeaponReady( void ) {
    if (status == WP_RELOAD) {
        common->HapticEvent("weapon_reload_finish", vr_weaponHand.GetInteger() ? 1 : 2, 0, 100, 0,
                            0);
    }

    status = WP_READY;
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = false;
	}
	if ( sndHum ) {
		StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
	}
}

/*
===============
idWeapon::Event_WeaponOutOfAmmo
===============
*/
void idWeapon::Event_WeaponOutOfAmmo( void ) {
	status = WP_OUTOFAMMO;
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = false;
	}
}

/*
===============
idWeapon::Event_WeaponReloading
===============
*/
void idWeapon::Event_WeaponReloading( void ) {
	status = WP_RELOAD;

    common->HapticEvent("weapon_reload", vr_weaponHand.GetInteger() ? 1 : 2, 0, 100, 0,0);
}

/*
===============
idWeapon::Event_WeaponHolstered
===============
*/
void idWeapon::Event_WeaponHolstered( void ) {
	status = WP_HOLSTERED;
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = false;
	}
}

/*
===============
idWeapon::Event_WeaponRising
===============
*/
void idWeapon::Event_WeaponRising( void ) {
	status = WP_RISING;
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = false;
	}
	owner->WeaponRisingCallback();
}

/*
===============
idWeapon::Event_WeaponLowering
===============
*/
void idWeapon::Event_WeaponLowering( void ) {
	status = WP_LOWERING;
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = false;
	}
	owner->WeaponLoweringCallback();
}

/*
===============
idWeapon::Event_UseAmmo
===============
*/
void idWeapon::Event_UseAmmo( int amount ) {
	if ( gameLocal.isClient ) {
		return;
	}

	owner->inventory.UseAmmo( ammoType, ( powerAmmo ) ? amount : ( amount * ammoRequired ) );
	if ( clipSize && ammoRequired ) {
		ammoClip -= powerAmmo ? amount : ( amount * ammoRequired );
		if ( ammoClip < 0 ) {
			ammoClip = 0;
		}
	}
}

/*
===============
idWeapon::Event_AddToClip
===============
*/
void idWeapon::Event_AddToClip( int amount ) {
	int ammoAvail;

	if ( gameLocal.isClient ) {
		return;
	}

	ammoClip += amount;
	if ( ammoClip > clipSize ) {
		ammoClip = clipSize;
	}

	ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
	if ( ammoClip > ammoAvail ) {
		ammoClip = ammoAvail;
	}
}

/*
===============
idWeapon::Event_AmmoInClip
===============
*/
void idWeapon::Event_AmmoInClip( void ) {
	int ammo = AmmoInClip();
	idThread::ReturnFloat( ammo );
}

/*
===============
idWeapon::Event_AmmoAvailable
===============
*/
void idWeapon::Event_AmmoAvailable( void ) {
	int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_TotalAmmoCount
===============
*/
void idWeapon::Event_TotalAmmoCount( void ) {
	int ammoAvail = owner->inventory.HasAmmo( ammoType, 1 );
	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_ClipSize
===============
*/
void idWeapon::Event_ClipSize( void ) {
	idThread::ReturnFloat( clipSize );
}

/*
===============
idWeapon::Event_AutoReload
===============
*/
void idWeapon::Event_AutoReload( void ) {
	assert( owner );
	if ( gameLocal.isClient ) {
		idThread::ReturnFloat( 0.0f );
		return;
	}
	idThread::ReturnFloat( gameLocal.userInfo[ owner->entityNumber ].GetBool( "ui_autoReload" ) );
}

/*
===============
idWeapon::Event_NetReload
===============
*/
void idWeapon::Event_NetReload( void ) {
	assert( owner );
	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_RELOAD, NULL, false, -1 );
	}
}

/*
===============
idWeapon::Event_NetEndReload
===============
*/
void idWeapon::Event_NetEndReload( void ) {
	assert( owner );
	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_ENDRELOAD, NULL, false, -1 );
	}
}

/*
===============
idWeapon::Event_PlayAnim
===============
*/
void idWeapon::Event_PlayAnim( int channel, const char *animname ) {
	int anim;

	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		if ( !( owner && owner->GetInfluenceLevel() ) ) {
			Show();
		}
		animator.PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();
		if ( worldModel.GetEntity() ) {
			anim = worldModel.GetEntity()->GetAnimator()->GetAnim( animname );
			if ( anim ) {
				worldModel.GetEntity()->GetAnimator()->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
			}
		}
	}
	animBlendFrames = 0;
	idThread::ReturnInt( 0 );
}

/*
===============
idWeapon::Event_PlayCycle
===============
*/
void idWeapon::Event_PlayCycle( int channel, const char *animname ) {
	int anim;

	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		if ( !( owner && owner->GetInfluenceLevel() ) ) {
			Show();
		}
		animator.CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();
		if ( worldModel.GetEntity() ) {
			anim = worldModel.GetEntity()->GetAnimator()->GetAnim( animname );
			worldModel.GetEntity()->GetAnimator()->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		}
	}
	animBlendFrames = 0;
	idThread::ReturnInt( 0 );
}

/*
===============
idWeapon::Event_AnimDone
===============
*/
void idWeapon::Event_AnimDone( int channel, int blendFrames ) {
	if ( animDoneTime - FRAME2MS( blendFrames ) <= gameLocal.time ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
===============
idWeapon::Event_SetBlendFrames
===============
*/
void idWeapon::Event_SetBlendFrames( int channel, int blendFrames ) {
	animBlendFrames = blendFrames;
}

/*
===============
idWeapon::Event_GetBlendFrames
===============
*/
void idWeapon::Event_GetBlendFrames( int channel ) {
	idThread::ReturnInt( animBlendFrames );
}

/*
================
idWeapon::Event_Next
================
*/
void idWeapon::Event_Next( void ) {
	// change to another weapon if possible
	owner->NextBestWeapon();
}

/*
================
idWeapon::Event_SetSkin
================
*/
void idWeapon::Event_SetSkin( const char *skinname ) {
	const idDeclSkin *skinDecl;

	if ( !skinname || !skinname[ 0 ] ) {
		skinDecl = NULL;
	} else {
		skinDecl = declManager->FindSkin( skinname );
	}

    // Don't update if the skin hasn't changed.
    if( renderEntity.customSkin == skinDecl && worldModel.GetEntity() != NULL && worldModel.GetEntity()->GetSkin() == skinDecl )
    {
        return;
    }

	renderEntity.customSkin = skinDecl;
	UpdateVisuals();

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetSkin( skinDecl );
	}

	if ( gameLocal.isServer && !isPlayerFlashlight) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteInt( ( skinDecl != NULL ) ? gameLocal.ServerRemapDecl( -1, DECL_SKIN, skinDecl->Index() ) : -1 );
		ServerSendEvent( EVENT_CHANGESKIN, &msg, false, -1 );
	}
}

/*
================
idWeapon::Event_Flashlight
================
*/
void idWeapon::Event_Flashlight( int enable ) {
	if ( enable ) {
		lightOn = true;
		MuzzleFlashLight();
	} else {
		lightOn = false;
		muzzleFlashEnd = 0;
	}
}

/*
================
idWeapon::Event_GetLightParm
================
*/
void idWeapon::Event_GetLightParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	idThread::ReturnFloat( muzzleFlash.shaderParms[ parmnum ] );
}

/*
================
idWeapon::Event_SetLightParm
================
*/
void idWeapon::Event_SetLightParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	muzzleFlash.shaderParms[ parmnum ]		= value;
	worldMuzzleFlash.shaderParms[ parmnum ]	= value;
	UpdateVisuals();
}

/*
================
idWeapon::Event_SetLightParms
================
*/
void idWeapon::Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 ) {
	muzzleFlash.shaderParms[ SHADERPARM_RED ]			= parm0;
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]			= parm1;
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]			= parm2;
	muzzleFlash.shaderParms[ SHADERPARM_ALPHA ]			= parm3;

	worldMuzzleFlash.shaderParms[ SHADERPARM_RED ]		= parm0;
	worldMuzzleFlash.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	worldMuzzleFlash.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	worldMuzzleFlash.shaderParms[ SHADERPARM_ALPHA ]	= parm3;

	UpdateVisuals();
}

/*
================
idWeapon::Event_Grabber
================
*/
void idWeapon::Event_Grabber( int enable )
{
	if( enable )
	{
		grabberState = 0;
	}
	else
	{
		grabberState = -1;
	}
}

/*
================
idWeapon::Event_GrabberHasTarget
================
*/
void idWeapon::Event_GrabberHasTarget()
{
	idThread::ReturnInt( grabberState );
}

/*
================
idWeapon::Event_GrabberSetGrabDistance
================
*/
void idWeapon::Event_GrabberSetGrabDistance( float dist )
{

	grabber.SetDragDistance( dist );
}

/*
================
idWeapon::Event_CreateProjectile
================
*/
void idWeapon::Event_CreateProjectile( void ) {
	if ( !gameLocal.isClient ) {
		projectileEnt = NULL;
		gameLocal.SpawnEntityDef( projectileDict, &projectileEnt, false );
		if ( projectileEnt ) {
			projectileEnt->SetOrigin( GetPhysics()->GetOrigin() );
			projectileEnt->Bind( owner, false );
			projectileEnt->Hide();
		}
		idThread::ReturnEntity( projectileEnt );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

/*
================
idWeapon::Event_LaunchProjectiles
================
*/
void idWeapon::Event_LaunchProjectiles( int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower ) {
	idProjectile	*proj;
	idEntity		*ent;
	int				i;
	idVec3			dir;
	float			ang;
	float			spin;
	float			distance;
	trace_t			tr;
	idVec3			start;
	idVec3			muzzle_pos;
	idBounds		ownerBounds, projBounds;

    // Koz the shotgun is supposed to be a closeup weapon in the game, but the depth provided in VR really changes
    // how it feels. IMO, it seems underpowered unless you are on top of an enemy, due to the extreme pellet spread.
    // This enables an optional 'choke' for the shotgun, by reducing the spread.
    weapon_t currentWeap;

    currentWeap = IdentifyWeapon();
	if ( currentWeap == WEAPON_SHOTGUN || currentWeap == WEAPON_SHOTGUN_DOUBLE || currentWeap == WEAPON_SHOTGUN_DOUBLE_MP )
    {
        //idPlayer *player;
        //player = gameLocal.GetLocalPlayer();
        //if ( 1 || player == owner )
        //{
        common->Printf( "Event_LaunchProjectiles spread = %f , ", spread );
        spread -= 14 * vr_shotgunChoke.GetFloat() / 100.0f;
        spread = idMath::ClampFloat( 8.0f, 22.0f, spread ); // not too low, or the shotgun gets WAY too powerful at range.
        common->Printf( "choke spread = %f\n", spread );
        //}
    }

    assert( owner != NULL );

	if ( IsHidden() ) {
		return;
	}

	if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = weaponDef->dict.GetString( "classname" );
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}

	// avoid all ammo considerations on an MP client
	if ( !gameLocal.isClient ) {

		// check if we're out of ammo or the clip is empty
		int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		if ( !ammoAvail || ( ( clipSize != 0 ) && ( ammoClip <= 0 ) ) ) {
			return;
		}

		// if this is a power ammo weapon ( currently only the bfg ) then make sure
		// we only fire as much power as available in each clip
		if ( powerAmmo ) {
			// power comes in as a float from zero to max
			// if we use this on more than the bfg will need to define the max
			// in the .def as opposed to just in the script so proper calcs
			// can be done here.
			dmgPower = ( int )dmgPower + 1;
			if ( dmgPower > ammoClip ) {
				dmgPower = ammoClip;
			}
		}

		owner->inventory.UseAmmo( ammoType, ( powerAmmo ) ? dmgPower : ammoRequired );
		if ( clipSize && ammoRequired ) {
			ammoClip -= powerAmmo ? dmgPower : 1;
		}

	}

	if ( !silent_fire ) {
		// wake up nearby monsters
		gameLocal.AlertAI( owner );
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.realClientTime );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// calculate the muzzle position
	GetProjectileLaunchOriginAndAxis( muzzleOrigin, muzzleAxis );

	// calculate the muzzle position
	/*
	if ( barrelJointView != INVALID_JOINT && projectileDict.GetBool( "launchFromBarrel" ) ) {
		// there is an explicit joint for the muzzle
		GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
	} else {
		muzzleOrigin = viewWeaponOrigin;
		muzzleAxis = viewWeaponAxis;
	}*/

	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( kick_endtime < gameLocal.realClientTime ) {
		kick_endtime = gameLocal.realClientTime;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.realClientTime + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.realClientTime + muzzle_kick_maxtime;
	}

	// "Predict" damage effects on clients by just spawning a local projectile that deals no damage. Used only
	// for sound & visual effects. Damage will be handled through reliable messages to the host.
	const bool isHitscan = projectileDict.GetBool( "net_instanthit" );
	const bool attackerIsLocal = owner->IsLocallyControlled();
	const bool actuallySpawnProjectile = gameLocal.isServer || attackerIsLocal || isHitscan;

	if( actuallySpawnProjectile )
	{
		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRad = DEG2RAD( spread );
		for( i = 0; i < num_projectiles; i++ )
		{
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = ( float )DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			dir = muzzleAxis[ 0 ] + muzzleAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - muzzleAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
			dir.Normalize();

			if( projectileEnt )
			{
				ent = projectileEnt;
				ent->Show();
				ent->Unbind();
				projectileEnt = NULL;
			}
			else
			{
				if( gameLocal.isClient )
				{
					// This is predicted on a client, don't replicate.
					// Must be set before spawn, so that the entity can be spawned into the correct area of the entities array.
					projectileDict.SetBool( "net_skip_replication", true );
				}
				else
				{
					projectileDict.SetBool( "net_skip_replication", false );
				}
				gameLocal.SpawnEntityDef( projectileDict, &ent, false );
			}

			if( ent == NULL || !ent->IsType( idProjectile::Type ) )
			{
				const char* projectileName = weaponDef->dict.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
				return;
			}



			if( projectileDict.GetBool( "net_instanthit" ) )
			{
				// don't synchronize this on top of the already predicted effect
				ent->fl.networkSync = false;
			}
			else if( owner != NULL )
			{
				/*
				int predictedKey = idEntity::INVALID_PREDICTION_KEY;
				// Set the prediction key only for non-instanthit projectiles.
				if( gameLocal.isClient )
				{
					owner->IncrementFireCount();
				}
				predictedKey = gameLocal.GeneratePredictionKey( this, owner, -1 );
				ent->SetPredictedKey( predictedKey );*/
			}

			proj = static_cast<idProjectile*>( ent );
			proj->Create( owner, muzzleOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			// make sure the projectile starts inside the bounding box of the owner
			// Carl: unless it's a grenade, in which case we want to be able to drop it over a railing without inexplicably killing ourselves
			if( IdentifyWeapon() == WEAPON_HANDGRENADE && game->isVR && commonVr->VR_USE_MOTION_CONTROLS && i == 0 )
			{
				muzzle_pos = muzzleOrigin + muzzleAxis[ 0 ] * 2.0f;
				// Carl: check that there's a grenade-sized gap at hand height from the center of our chest to our grenade,
				// so we can drop grenades over a railing, but not through a solid wall or glass window.
				// Ideally start should be our right shoulder at grenade height, but I'm not sure how to get that.
				start = ownerBounds.GetCenter();
				start.z = muzzleOrigin.z;
				// MASK_SHOT_RENDERMODEL goes through chainlink fences, but a grenade shouldn't.
				gameLocal.clip.Translation(tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL | CONTENTS_PLAYERCLIP, owner);
				// Try reaching down from neck height if sticking our hand out straight doesn't work.
				// Ideally this should be our right shoulder at shoulder height, but I'm not sure how to get that.
				if ( tr.fraction < 1.0f )
				{
					start.z = commonVr->lastHMDViewOrigin.z - 6;
					gameLocal.clip.Translation(tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL | CONTENTS_PLAYERCLIP, owner);
				}
				muzzle_pos = tr.endpos;
			}
			else if( i == 0 )
			{
				muzzle_pos = muzzleOrigin + muzzleAxis[ 0 ] * 2.0f;
				if( ( ownerBounds - projBounds ).RayIntersection( muzzle_pos, muzzleAxis[0], distance ) )
				{
					start = muzzle_pos + distance * muzzleAxis[0];
				}
				else
				{
					start = ownerBounds.GetCenter();
				}
				gameLocal.clip.Translation( tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
				muzzle_pos = tr.endpos;
			}

			// Koz if throwing a grenade use the tracked hand velocity when using motion controls if the controller is not mounted
			// Carl: Dual Wielding
			float speed = 0;
			if( IdentifyWeapon() == WEAPON_HANDGRENADE && game->isVR && commonVr->VR_USE_MOTION_CONTROLS && !vr_mountedWeaponController.GetBool() )
			{
				int h = GetHand();
				if( h >= 0 )
					speed = owner->hands[ h ].throwVelocity * vr_throwPower.GetFloat();
			}
			// Koz end

			// Normal launch
			proj->Launch( muzzle_pos, dir, pushVelocity, fuseOffset, launchPower, dmgPower, speed );

			int position = vr_weaponHand.GetInteger() ? 1 : 2;

			if (currentWeap == WEAPON_PISTOL)
            {
				position = commonVr->GetWeaponStabilised() ? 4 : position;
			    common->HapticEvent("pistol_fire", position, 0, 100, 0, 0);
            }
			if (currentWeap == WEAPON_SHOTGUN)
            {
				position = commonVr->GetWeaponStabilised() ? 4 : position;
			    common->HapticEvent("shotgun_fire",  position, 0, 100, 0, 0);
            }
			if (currentWeap == WEAPON_PLASMAGUN)
            {
				position = commonVr->GetWeaponStabilised() ? 4 : position;
			    common->HapticEvent("plasmagun_fire",  position, 0, 100, 0, 0);
            }
			if (currentWeap == WEAPON_HANDGRENADE)
            {
			    common->HapticEvent("handgrenade_fire",  position, 0, 100, 0, 0);
            }
			if (currentWeap == WEAPON_MACHINEGUN)
            {
				position = commonVr->GetWeaponStabilised() ? 4 : position;
			    common->HapticEvent("machinegun_fire",  position, 0, 100, 0, 0);
            }
			if (currentWeap == WEAPON_CHAINGUN)
            {
				position = commonVr->GetWeaponStabilised() ? 4 : position;
			    common->HapticEvent("chaingun_fire",  position, 0, 100, 0, 0);
            }
			if (currentWeap == WEAPON_BFG)
            {
			    common->HapticEvent("bfg_fire",  position, 0, 100, 0, 0);
            }
			if (currentWeap == WEAPON_ROCKETLAUNCHER)
            {
				position = commonVr->GetWeaponStabilised() ? 4 : position;
			    common->HapticEvent("rocket_fire",  position, 0, 100, 0, 0);
            }

		}

		// toss the brass
		if( brassDelay >= 0 )
		{
			PostEventMS( &EV_Weapon_EjectBrass, brassDelay );
		}
	}

	/*if ( gameLocal.isClient ) {

		// predict instant hit projectiles
		if ( projectileDict.GetBool( "net_instanthit" ) ) {
			float spreadRad = DEG2RAD( spread );
			//muzzle_pos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;
			muzzle_pos = muzzleOrigin + viewWeaponAxis[ 0 ] * 2.0f;
			for( i = 0; i < num_projectiles; i++ ) {
				ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
				spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();

				dir = viewWeaponAxis[ 0 ] + viewWeaponAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - viewWeaponAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
				dir.Normalize();
				gameLocal.clip.Translation( tr, muzzle_pos, muzzle_pos + dir * 4096.0f, NULL, mat3_identity, MASK_SHOT_RENDERMODEL, owner );
				if ( tr.fraction < 1.0f ) {
					idProjectile::ClientPredictionCollide( this, projectileDict, tr, vec3_origin, true );
				}
			}
		}

	} else {

		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRad = DEG2RAD( spread );
		for( i = 0; i < num_projectiles; i++ ) {
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			dir = muzzleAxis[ 0 ] + muzzleAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - muzzleAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
			dir.Normalize();


			/* vrClientInfo *pVRClientInfo = owner->GetVRClientInfo();
			 * if (owner->GetCurrentWeaponId() == WEAPON_HANDGRENADE &&
				pVRClientInfo != nullptr &&
				vr_throwables.GetBool())
			{
				idVec3	releaseOffset( -pVRClientInfo->throw_origin[2],
										 -pVRClientInfo->throw_origin[0],
										 pVRClientInfo->throw_origin[1]);
				idAngles a(0, owner->viewAngles.yaw - pVRClientInfo->hmdorientation[YAW], 0);
				releaseOffset *= a.ToMat3();
				releaseOffset *= ((100.0f / 2.54f) * vr_scale.GetFloat());
				muzzle_pos = owner->firstPersonViewOrigin + releaseOffset;

				idVec3	throw_direction( -pVRClientInfo->throw_trajectory[2],
										   -pVRClientInfo->throw_trajectory[0],
										   pVRClientInfo->throw_trajectory[1]);
				throw_direction *= a.ToMat3();
				throw_direction.Normalize();
				dir = throw_direction;

				launchPower = pVRClientInfo->throw_power * 0.5f;
			} else {
				dir = viewWeaponAxis[0] + viewWeaponAxis[2] * (ang * idMath::Sin(spin)) -
					  viewWeaponAxis[1] * (ang * idMath::Cos(spin));

				dir.Normalize();
			}***

			if ( projectileEnt ) {
				ent = projectileEnt;
				ent->Show();
				ent->Unbind();
				projectileEnt = NULL;
			} else {
				gameLocal.SpawnEntityDef( projectileDict, &ent, false );
			}

			if ( !ent || !ent->IsType( idProjectile::Type ) ) {
				const char *projectileName = weaponDef->dict.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
			}

			if ( projectileDict.GetBool( "net_instanthit" ) ) {
				// don't synchronize this on top of the already predicted effect
				ent->fl.networkSync = false;
			}

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, muzzleOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			if (owner->GetCurrentWeaponId() != WEAPON_HANDGRENADE ||
				!vr_throwables.GetBool()) {
				// make sure the projectile starts inside the bounding box of the owner
				if (i == 0) {
					//muzzle_pos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;
					muzzle_pos = muzzleOrigin + viewWeaponAxis[0] * 2.0f;
					// DG: sometimes the assertion in idBounds::operator-(const idBounds&) triggers
					//     (would get bounding box with negative volume)
					//     => check that before doing ownerBounds - projBounds (equivalent to the check in the assertion)
					idVec3 obDiff = ownerBounds[1] - ownerBounds[0];
					idVec3 pbDiff = projBounds[1] - projBounds[0];
					bool boundsSubLegal =
							obDiff.x > pbDiff.x && obDiff.y > pbDiff.y && obDiff.z > pbDiff.z;
					if (boundsSubLegal &&
						(ownerBounds - projBounds).RayIntersection(muzzle_pos, viewWeaponAxis[0],
																   distance)) {
						//start = muzzle_pos + distance * playerViewAxis[0];
						start = muzzle_pos + distance * viewWeaponAxis[0];
					} else {
						start = ownerBounds.GetCenter();
					}
					gameLocal.clip.Translation(tr, start, muzzle_pos,
											   proj->GetPhysics()->GetClipModel(),
											   proj->GetPhysics()->GetClipModel()->GetAxis(),
											   MASK_SHOT_RENDERMODEL, owner);
					muzzle_pos = tr.endpos;
				}
			}

            proj->Launch(muzzle_pos, dir, pushVelocity, fuseOffset, launchPower, dmgPower);
		}

		// toss the brass
		PostEventMS( &EV_Weapon_EjectBrass, brassDelay );
	}*/

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->WeaponFireFeedback( GetHand(), &weaponDef->dict );

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.realClientTime;
}



/*
=====================
idWeapon::Event_Melee
=====================
*/
void idWeapon::Event_Melee( void ) {
	idEntity	*ent;
	trace_t		tr;

	if ( !meleeDef ) {
		gameLocal.Error( "No meleeDef on '%s'", weaponDef->dict.GetString( "classname" ) );
	}

	weapon_t currentWeapon = IdentifyWeapon();
	if (currentWeapon == WEAPON_FISTS)
	{
		common->HapticEvent("punch", 2 - GetHand(), 0, 100, 0, 0);
	}
	if (currentWeapon == WEAPON_CHAINSAW)
	{
		common->HapticStopEvent("chainsaw_idle");
		common->HapticEvent("chainsaw_fire", vr_weaponHand.GetInteger() ? 1 : 2, 0, 100, 0, 0);
	}

	if ( !gameLocal.isClient ) {
		idVec3 start = viewWeaponOrigin;
		//idVec3 end = start + playerViewAxis[0] * ( meleeDistance * owner->PowerUpModifier( MELEE_DISTANCE ) );
		idVec3 end = start + viewWeaponAxis[0] * ( meleeDistance * owner->PowerUpModifier( MELEE_DISTANCE ) );
		gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, owner );
		if ( tr.fraction < 1.0f ) {
			ent = gameLocal.GetTraceEntity( tr );
		} else {
			ent = NULL;
		}

		if ( g_debugWeapon.GetBool() ) {
			gameRenderWorld->DebugLine( colorYellow, start, end, 100 );
			if ( ent ) {
				gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds(), ent->GetPhysics()->GetOrigin(), 100 );
			}
		}

		bool hit = false;
		const char *hitSound = meleeDef->dict.GetString( "snd_miss" );

		if ( ent ) {

			float push = meleeDef->dict.GetFloat( "push" );
			idVec3 impulse = -push * owner->PowerUpModifier( SPEED ) * tr.c.normal;

			if ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) && ( ent->IsType( idActor::Type ) || ent->IsType( idAFAttachment::Type) ) ) {
				idThread::ReturnInt( 0 );
				return;
			}

			ent->ApplyImpulse( this, tr.c.id, tr.c.point, impulse );

			// weapon stealing - do this before damaging so weapons are not dropped twice
			if ( gameLocal.isMultiplayer
				&& weaponDef && weaponDef->dict.GetBool( "stealing" )
				&& ent->IsType( idPlayer::Type )
				&& !owner->PowerUpActive( BERSERK )
				&& ( gameLocal.gameType != GAME_TDM || gameLocal.serverInfo.GetBool( "si_teamDamage" ) || ( owner->team != static_cast< idPlayer * >( ent )->team ) )
				) {
				owner->StealWeapon( static_cast< idPlayer * >( ent ) );
			}

			if ( ent->fl.takedamage ) {
				idVec3 kickDir, globalKickDir;
				meleeDef->dict.GetVector( "kickDir", "0 0 0", kickDir );
				globalKickDir = muzzleAxis * kickDir;
				ent->Damage( owner, owner, globalKickDir, meleeDefName, owner->PowerUpModifier( MELEE_DAMAGE ), tr.c.id );
				hit = true;
			}

			if ( weaponDef->dict.GetBool( "impact_damage_effect" ) ) {

				if ( ent->spawnArgs.GetBool( "bleed" ) ) {

					hitSound = meleeDef->dict.GetString( owner->PowerUpActive( BERSERK ) ? "snd_hit_berserk" : "snd_hit" );

					ent->AddDamageEffect( tr, impulse, meleeDef->dict.GetString( "classname" ) );

				} else {

					int type = tr.c.material->GetSurfaceType();
					if ( type == SURFTYPE_NONE ) {
						type = GetDefaultSurfaceType();
					}

					const char *materialType = gameLocal.sufaceTypeNames[ type ];

					// start impact sound based on material type
					hitSound = meleeDef->dict.GetString( va( "snd_%s", materialType ) );
					if ( *hitSound == '\0' ) {
						hitSound = meleeDef->dict.GetString( "snd_metal" );
					}

					if ( gameLocal.time > nextStrikeFx ) {
						const char *decal;
						// project decal
						decal = weaponDef->dict.GetString( "mtr_strike" );
						if ( decal && *decal ) {
							gameLocal.ProjectDecal( tr.c.point, -tr.c.normal, 8.0f, true, 6.0, decal );
						}
						nextStrikeFx = gameLocal.time + 200;
					} else {
						hitSound = "";
					}

					strikeSmokeStartTime = gameLocal.time;
					strikePos = tr.c.point;
					strikeAxis = -tr.endAxis;
				}
			}
		}

		if ( *hitSound != '\0' ) {
			const idSoundShader *snd = declManager->FindSound( hitSound );
			StartSoundShader( snd, SND_CHANNEL_BODY2, 0, true, NULL );
		}

		idThread::ReturnInt( hit );

		owner->WeaponFireFeedback( GetHand(), &weaponDef->dict );
		return;
	}

	idThread::ReturnInt( 0 );
	owner->WeaponFireFeedback( GetHand(), &weaponDef->dict );
}

/*
=====================
idWeapon::Event_GetWorldModel
=====================
*/
void idWeapon::Event_GetWorldModel( void ) {
	idThread::ReturnEntity( worldModel.GetEntity() );
}

/*
=====================
idWeapon::Event_AllowDrop
=====================
*/
void idWeapon::Event_AllowDrop( int allow ) {
	if ( allow ) {
		allowDrop = true;
	} else {
		allowDrop = false;
	}
}

/*
================
idWeapon::Event_EjectBrass

Toss a shell model out from the breach if the bone is present
================
*/
void idWeapon::Event_EjectBrass( void ) {
	if ( !g_showBrass.GetBool() || !owner->CanShowWeaponViewmodel() ) {
		return;
	}

	if ( ejectJointView == INVALID_JOINT || !brassDict.GetNumKeyVals() ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	idMat3 axis;
	idVec3 origin, linear_velocity, angular_velocity;
	idEntity *ent;

	if ( !GetGlobalJointTransform( true, ejectJointView, origin, axis ) ) {
		return;
	}

	gameLocal.SpawnEntityDef( brassDict, &ent, false );
	if ( !ent || !ent->IsType( idDebris::Type ) ) {
		gameLocal.Error( "'%s' is not an idDebris", weaponDef ? weaponDef->dict.GetString( "def_ejectBrass" ) : "def_ejectBrass" );
	}
	idDebris *debris = static_cast<idDebris *>(ent);
	debris->Create( owner, origin, axis );
	debris->Launch();

    // Koz begin
    idAngles vwa = viewWeaponAxis.ToAngles();
    vwa.yaw += 30.0f;
    idMat3 ba = vwa.Normalize180().ToMat3();

    //linear_velocity = 50 * (viewWeaponAxis[0] + viewWeaponAxis[1] + viewWeaponAxis[2]);
    linear_velocity = 50 * ( ba[0] + ba[1] + ba[2] );

    // Koz end
	//linear_velocity = 40 * ( playerViewAxis[0] + playerViewAxis[1] + playerViewAxis[2] );
	//linear_velocity = 40 * ( viewWeaponAxis[0] + viewWeaponAxis[1] + viewWeaponAxis[2] );
	angular_velocity.Set( 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat() );

	debris->GetPhysics()->SetLinearVelocity( linear_velocity );
	debris->GetPhysics()->SetAngularVelocity( angular_velocity );
}

/*Koz - what a mess
===============
idWeapon::Event_GetWeaponSkin
===============
*/
void idWeapon::Event_GetWeaponSkin()
{
	if ( !owner || !game->isVR )
	{
		idThread::ReturnString( "" );
		return;
	}

	// game is vr, return a string name for the skin matching the hand/statwatch config.

	static idStr vrSkinName;

	// Koz fixme - need to go through all the weapon models and delete the meshes for the hands and statwatch.  Also need to delete all the skin definitions, no longer necessary.

	// this has all changed.  Originally, hands were part of the weapon models, and hands and statwatch were turned on and off
	// by changing the skin for model.  Now the hands are always drawn as part of the player body, so all the skins
	// are no longer needed.  The only skin used now is for the mini flashlight when mounted to the weapon.


	if ( isPlayerFlashlight )
	{
		vrSkinName = ( commonVr->GetCurrentFlashlightMode() == FLASHLIGHT_GUN || commonVr->GetCurrentFlashlightMode() == FLASHLIGHT_PISTOL ) ? "minivr/flashhands/0h" : "vr/flashhands/0h"; // mini flashlight skin for gun mount : normal flashlight skin
	}
	else
	{
		//vrSkinName = "vr/weaponhands/0h";
		vrSkinName = "";
	}

	//common->Printf( "idWeapon::Event_GetWeaponSkin() returning %s\n", vrSkinName.c_str() );
	idThread::ReturnString( vrSkinName.c_str() );

}

/*
==================
idPlayer::Event_IsMotionControlled
==================
*/
void idWeapon::Event_IsMotionControlled()
{
	static int isMC;

	//isMC = (game->isVR && commonVr->VR_USE_MOTION_CONTROLS) ? 1 : 0;
	isMC = 1;

	// Koz debug common->Printf( "Event_IsMotionControlled returning %d\n", isMC );
	//	idThread::ReturnInt( ( game->isVR && commonVr->VR_USE_MOTION_CONTROLS ) ? 1 : 0 );

	idThread::ReturnInt( isMC );
}

/*
===============
idWeapon::Event_IsInvisible
===============
*/
void idWeapon::Event_IsInvisible( void ) {
	if ( !owner ) {
		idThread::ReturnFloat( 0 );
		return;
	}
	idThread::ReturnFloat( owner->PowerUpActive( INVISIBILITY ) ? 1 : 0 );
}

/*
===============
idWeapon::ClientPredictionThink
===============
*/
void idWeapon::ClientPredictionThink( void ) {
	UpdateAnimation();
}
