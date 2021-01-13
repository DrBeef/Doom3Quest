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

#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"
#include "Player.h"

#include "Camera.h"

/*
===============================================================================

  idCamera

  Base class for cameras

===============================================================================
*/

ABSTRACT_DECLARATION( idEntity, idCamera )
END_CLASS

/*
=====================
idCamera::Spawn
=====================
*/
void idCamera::Spawn( void ) {
}

/*
=====================
idCamera::GetRenderView
=====================
*/
renderView_t *idCamera::GetRenderView() {
	renderView_t *rv = idEntity::GetRenderView();
	GetViewParms( rv );
	return rv;
}

/***********************************************************************

  idCameraView

***********************************************************************/
const idEventDef EV_Camera_SetAttachments( "<getattachments>", NULL );

CLASS_DECLARATION( idCamera, idCameraView )
	EVENT( EV_Activate,				idCameraView::Event_Activate )
	EVENT( EV_Camera_SetAttachments, idCameraView::Event_SetAttachments )
END_CLASS


/*
===============
idCameraView::idCameraView
================
*/
idCameraView::idCameraView() {
	fov = 90.0f;
	attachedTo = NULL;
	attachedView = NULL;
}

/*
===============
idCameraView::Save
================
*/
void idCameraView::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( fov );
	savefile->WriteObject( attachedTo );
	savefile->WriteObject( attachedView );
}

/*
===============
idCameraView::Restore
================
*/
void idCameraView::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( fov );
	savefile->ReadObject( reinterpret_cast<idClass *&>( attachedTo ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( attachedView ) );
}

/*
===============
idCameraView::Event_SetAttachments
================
*/
void idCameraView::Event_SetAttachments(  ) {
	SetAttachment( &attachedTo, "attachedTo" );
	SetAttachment( &attachedView, "attachedView" );
}

/*
===============
idCameraView::Event_Activate
================
*/
void idCameraView::Event_Activate( idEntity *activator ) {
	if (spawnArgs.GetBool("trigger")) {
		if (gameLocal.GetCamera() != this) {
			if ( g_debugCinematic.GetBool() ) {
				gameLocal.Printf( "%d: '%s' start\n", gameLocal.framenum, GetName() );
			}

			gameLocal.SetCamera(this);
		} else {
			if ( g_debugCinematic.GetBool() ) {
				gameLocal.Printf( "%d: '%s' stop\n", gameLocal.framenum, GetName() );
			}
			gameLocal.SetCamera(NULL);
		}
	}
}

/*
=====================
idCameraView::Stop
=====================
*/
void idCameraView::Stop( void ) {
	if ( g_debugCinematic.GetBool() ) {
		gameLocal.Printf( "%d: '%s' stop\n", gameLocal.framenum, GetName() );
	}
	gameLocal.SetCamera(NULL);
	ActivateTargets( gameLocal.GetLocalPlayer() );
}


/*
=====================
idCameraView::Spawn
=====================
*/
void idCameraView::SetAttachment( idEntity **e, const char *p  ) {
	const char *cam = spawnArgs.GetString( p );
	if ( strlen ( cam ) ) {
		*e = gameLocal.FindEntity( cam );
	}
}


/*
=====================
idCameraView::Spawn
=====================
*/
void idCameraView::Spawn( void ) {
	// if no target specified use ourself
	const char *cam = spawnArgs.GetString("cameraTarget");
	if ( strlen ( cam ) == 0) {
		spawnArgs.Set("cameraTarget", spawnArgs.GetString("name"));
	}
	fov = spawnArgs.GetFloat("fov", "90");

	PostEventMS( &EV_Camera_SetAttachments, 0 );

	UpdateChangeableSpawnArgs(NULL);
}

/*
=====================
idCameraView::GetViewParms
=====================
*/
void idCameraView::GetViewParms( renderView_t *view ) {
	assert( view );

	if (view == NULL) {
		return;
	}

	idVec3 dir;
	idEntity *ent;

	if ( attachedTo ) {
		ent = attachedTo;
	} else {
		ent = this;
	}

	view->vieworg = ent->GetPhysics()->GetOrigin();
	if ( attachedView ) {
		dir = attachedView->GetPhysics()->GetOrigin() - view->vieworg;
		dir.Normalize();
		view->viewaxis = dir.ToMat3();
	} else {
		view->viewaxis = ent->GetPhysics()->GetAxis();
	}

	gameLocal.CalcFov( fov, view->fov_x, view->fov_y );
}

/*
===============================================================================

  idCameraAnim

===============================================================================
*/

const idEventDef EV_Camera_Start( "start", NULL );
const idEventDef EV_Camera_Stop( "stop", NULL );

CLASS_DECLARATION( idCamera, idCameraAnim )
	EVENT( EV_Thread_SetCallback,	idCameraAnim::Event_SetCallback )
	EVENT( EV_Camera_Stop,			idCameraAnim::Event_Stop )
	EVENT( EV_Camera_Start,			idCameraAnim::Event_Start )
	EVENT( EV_Activate,				idCameraAnim::Event_Activate )
END_CLASS


/*
=====================
idCameraAnim::idCameraAnim
=====================
*/
idCameraAnim::idCameraAnim() {
	threadNum = 0;
	offset.Zero();
	frameRate = 0;
	cycle = 1;
	starttime = 0;
	activator = NULL;

}

/*
=====================
idCameraAnim::~idCameraAnim
=====================
*/
idCameraAnim::~idCameraAnim() {
	if ( gameLocal.GetCamera() == this ) {
		gameLocal.SetCamera( NULL );
	}
}

/*
===============
idCameraAnim::Save
================
*/
void idCameraAnim::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( threadNum );
	savefile->WriteVec3( offset );
	savefile->WriteInt( frameRate );
	savefile->WriteInt( starttime );
	savefile->WriteInt( cycle );
	activator.Save( savefile );
}

/*
===============
idCameraAnim::Restore
================
*/
void idCameraAnim::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( threadNum );
	savefile->ReadVec3( offset );
	savefile->ReadInt( frameRate );
	savefile->ReadInt( starttime );
	savefile->ReadInt( cycle );
	activator.Restore( savefile );

	LoadAnim();
}

/*
=====================
idCameraAnim::Spawn
=====================
*/
void idCameraAnim::Spawn( void ) {
	if ( spawnArgs.GetVector( "old_origin", "0 0 0", offset ) ) {
		offset = GetPhysics()->GetOrigin() - offset;
	} else {
		offset.Zero();
	}

	// always think during cinematics
	cinematic = true;

	LoadAnim();
}

/*
================
idCameraAnim::Load
================
*/
void idCameraAnim::LoadAnim( void ) {
	int			version;
	idLexer		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	idToken		token;
	int			numFrames;
	int			numCuts;
	int			i;
	idStr		filename;
	const char	*key;

	key = spawnArgs.GetString( "anim" );
	if ( !key ) {
		gameLocal.Error( "Missing 'anim' key on '%s'", name.c_str() );
	}

	filename = spawnArgs.GetString( va( "anim %s", key ) );
	if ( !filename.Length() ) {
		gameLocal.Error( "Missing 'anim %s' key on '%s'", key, name.c_str() );
	}

	filename.SetFileExtension( MD5_CAMERA_EXT );
	if ( !parser.LoadFile( filename ) ) {
		gameLocal.Error( "Unable to load '%s' on '%s'", filename.c_str(), name.c_str() );
	}

	cameraCuts.Clear();
	cameraCuts.SetGranularity( 1 );
	camera.Clear();
	camera.SetGranularity( 1 );

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();
	if ( version != MD5_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}

	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if ( numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", numFrames );
	}

	// parse framerate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if ( frameRate <= 0 ) {
		parser.Error( "Invalid framerate: %d", frameRate );
	}

	// parse num cuts
	parser.ExpectTokenString( "numCuts" );
	numCuts = parser.ParseInt();
	if ( ( numCuts < 0 ) || ( numCuts > numFrames ) ) {
		parser.Error( "Invalid number of camera cuts: %d", numCuts );
	}

	// parse the camera cuts
	parser.ExpectTokenString( "cuts" );
	parser.ExpectTokenString( "{" );
	cameraCuts.SetNum( numCuts );

	idToken cutToken; // Koz

	for( i = 0; i < numCuts; i++ ) {
		cameraCuts[i].posOverride = false;
		cameraCuts[i].rotOverride = false;
		cameraCuts[i].posNew = vec3_zero;
		cameraCuts[i].rotNew.Set( 0.0f, 0.0f, 0.0f );

		cameraCuts[ i ].cutFrame = parser.ParseInt();
		if( ( cameraCuts[ i ].cutFrame < 0 ) || ( cameraCuts[ i ].cutFrame >= numFrames ) ) // Koz - changed to allow camera cut on first frame.
		{
			parser.Error( "Invalid camera cut" );
		}

		// Koz read pos and rot override values if present.
		if ( parser.PeekTokenString( "pos" ) )
		{
			parser.ReadToken( &cutToken );
			parser.Parse1DMatrix( 3, cameraCuts[i].posNew.ToFloatPtr() );
			cameraCuts[i].posOverride = true;
		}

		if ( parser.PeekTokenString( "rotA" ) ) // read rotation in angles - easier for manual editing.
		{
			parser.ReadToken( &cutToken );

			idAngles ta;
			parser.Parse1DMatrix( 3, &ta[0] );
			cameraCuts[i].rotNew = ta.ToQuat().ToCQuat();
			cameraCuts[i].rotOverride = true;
		}

		if ( parser.PeekTokenString( "rot" ) ) // read rotation quat - easier to copy from exitsing frame.
		{
			parser.ReadToken( &cutToken );
			parser.Parse1DMatrix( 3, cameraCuts[i].rotNew.ToFloatPtr() );
			cameraCuts[i].rotOverride = true;
		}
	}

	// Koz End

	parser.ExpectTokenString( "}" );

	// parse the camera frames
	parser.ExpectTokenString( "camera" );
	parser.ExpectTokenString( "{" );
	camera.SetNum( numFrames );
	for( i = 0; i < numFrames; i++ ) {
		parser.Parse1DMatrix( 3, camera[ i ].t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, camera[ i ].q.ToFloatPtr() );
		camera[ i ].fov = parser.ParseFloat();
	}
	parser.ExpectTokenString( "}" );

#if 0
	if ( !gameLocal.GetLocalPlayer() ) {
		return;
	}

	idDebugGraph gGraph;
	idDebugGraph tGraph;
	idDebugGraph qGraph;
	idDebugGraph dtGraph;
	idDebugGraph dqGraph;
	gGraph.SetNumSamples( numFrames );
	tGraph.SetNumSamples( numFrames );
	qGraph.SetNumSamples( numFrames );
	dtGraph.SetNumSamples( numFrames );
	dqGraph.SetNumSamples( numFrames );

	gameLocal.Printf( "\n\ndelta vec:\n" );
	float diff_t, last_t, t;
	float diff_q, last_q, q;
	diff_t = last_t = 0.0f;
	diff_q = last_q = 0.0f;
	for( i = 1; i < numFrames; i++ ) {
		t = ( camera[ i ].t - camera[ i - 1 ].t ).Length();
		q = ( camera[ i ].q.ToQuat() - camera[ i - 1 ].q.ToQuat() ).Length();
		diff_t = t - last_t;
		diff_q = q - last_q;
		gGraph.AddValue( ( i % 10 ) == 0 );
		tGraph.AddValue( t );
		qGraph.AddValue( q );
		dtGraph.AddValue( diff_t );
		dqGraph.AddValue( diff_q );

		gameLocal.Printf( "%d: %.8f  :  %.8f,     %.8f  :  %.8f\n", i, t, diff_t, q, diff_q  );
		last_t = t;
		last_q = q;
	}

	gGraph.Draw( colorBlue, 300.0f );
	tGraph.Draw( colorOrange, 60.0f );
	dtGraph.Draw( colorYellow, 6000.0f );
	qGraph.Draw( colorGreen, 60.0f );
	dqGraph.Draw( colorCyan, 6000.0f );
#endif
}

/*
===============
idCameraAnim::Start
================
*/
void idCameraAnim::Start( void ) {
	cycle = spawnArgs.GetInt( "cycle" );
	if ( !cycle ) {
		cycle = 1;
	}

	if ( g_debugCinematic.GetBool() ) {
		gameLocal.Printf( "%d: '%s' start\n", gameLocal.framenum, GetName() );
	}

	starttime = gameLocal.time;
	gameLocal.SetCamera( this );
	BecomeActive( TH_THINK );

	// if the player has already created the renderview for this frame, have him update it again so that the camera starts this frame
	if ( gameLocal.GetLocalPlayer()->GetRenderView()->time == gameLocal.time ) {
		gameLocal.GetLocalPlayer()->CalculateRenderView();
	}
}

/*
=====================
idCameraAnim::Stop
=====================
*/
void idCameraAnim::Stop( void ) {
	if ( gameLocal.GetCamera() == this ) {
		if ( g_debugCinematic.GetBool() ) {
			gameLocal.Printf( "%d: '%s' stop\n", gameLocal.framenum, GetName() );
		}

		BecomeInactive( TH_THINK );
		gameLocal.SetCamera( NULL );
		if ( threadNum ) {
			idThread::ObjectMoveDone( threadNum, this );
			threadNum = 0;
		}
		ActivateTargets( activator.GetEntity() );
	}
}

/*
=====================
idCameraAnim::Think
=====================
*/
void idCameraAnim::Think( void ) {
	int frame;
	int frameTime;

	if ( thinkFlags & TH_THINK ) {
		// check if we're done in the Think function when the cinematic is being skipped (idCameraAnim::GetViewParms isn't called when skipping cinematics).
		if ( !gameLocal.skipCinematic ) {
			return;
		}

		if ( camera.Num() < 2 ) {
			// 1 frame anims never end
			return;
		}

		if ( frameRate == renderSystem->GetRefresh() ) {
			frameTime	= gameLocal.time - starttime;
			frame		= frameTime / USERCMD_MSEC;
		} else {
			frameTime	= ( gameLocal.time - starttime ) * frameRate;
			frame		= frameTime / 1000;
		}

		if ( frame > camera.Num() + cameraCuts.Num() - 2 ) {
			if ( cycle > 0 ) {
				cycle--;
			}

			if ( cycle != 0 ) {
				// advance start time so that we loop
				starttime += ( ( camera.Num() - cameraCuts.Num() ) * 1000 ) / frameRate;
			} else {
				Stop();
			}
		}
	}
}

/*
=====================
idCameraAnim::GetViewParms
=====================
*/
void idCameraAnim::GetViewParms( renderView_t *view ) {
	int				realFrame;
	int				frame;
	int				frameTime;
	float			lerp;
	float			invlerp;
	cameraFrame_t	*camFrame;
	cameraFrame_t*	camFrame2; // Koz for clamping camera positions during cinematics to eliminate uncomfortable panning in VR.

	cameraFrame_t	cutFrame;
	bool			cutRotOverride;

	int				i;
	int				cut;
	idQuat			q1, q2, q3;

	assert( view );
	if ( !view ) {
		return;
	}

	if ( camera.Num() == 0 ) {
		// we most likely are in the middle of a restore
		// FIXME: it would be better to fix it so this doesn't get called during a restore
		return;
	}

	if ( frameRate == renderSystem->GetRefresh() ) {
		frameTime	= gameLocal.time - starttime;
		frame		= frameTime / USERCMD_MSEC;
		lerp		= 0.0f;
	} else {
		frameTime	= ( gameLocal.time - starttime ) * frameRate;
		frame		= frameTime / 1000;
		lerp		= ( frameTime % 1000 ) * 0.001f;
	}

	if(vr_cinematics.GetInteger() == 2)
	{
		// skip any frames where camera cuts occur
		realFrame = frame;
		cut = 0;
		for( i = 0; i < cameraCuts.Num(); i++ ) {
			if ( frame < cameraCuts[ i ].cutFrame ) {
				break;
			}
			frame++;
			cut++;
		}

		if ( g_debugCinematic.GetBool() ) {
			int prevFrameTime	= ( gameLocal.time - starttime - USERCMD_MSEC ) * frameRate;
			int prevFrame		= prevFrameTime / 1000;
			int prevCut;

			prevCut = 0;
			for( i = 0; i < cameraCuts.Num(); i++ ) {
				if ( prevFrame < cameraCuts[ i ].cutFrame ) {
					break;
				}
				prevFrame++;
				prevCut++;
			}

			if ( prevCut != cut ) {
				gameLocal.Printf( "%d: '%s' cut %d\n", gameLocal.framenum, GetName(), cut );
			}
		}

		// clamp to the first frame.  also check if this is a one frame anim.  one frame anims would end immediately,
		// but since they're mainly used for static cams anyway, just stay on it infinitely.
		if ( ( frame < 0 ) || ( camera.Num() < 2 ) ) {
			view->viewaxis = camera[ 0 ].q.ToQuat().ToMat3();
			view->vieworg = camera[ 0 ].t + offset;
			view->fov_x = camera[ 0 ].fov;
		} else if ( frame > camera.Num() - 2 ) {
			if ( cycle > 0 ) {
				cycle--;
			}

			if ( cycle != 0 ) {
				// advance start time so that we loop
				starttime += ( ( camera.Num() - cameraCuts.Num() ) * 1000 ) / frameRate;
				GetViewParms( view );
				return;
			}

			Stop();
			if ( gameLocal.GetCamera() != NULL ) {
				// we activated another camera when we stopped, so get it's viewparms instead
				gameLocal.GetCamera()->GetViewParms( view );
				return;
			} else {
				// just use our last frame
				camFrame = &camera[ camera.Num() - 1 ];
				view->viewaxis = camFrame->q.ToQuat().ToMat3();
				view->vieworg = camFrame->t + offset;
				view->fov_x = camFrame->fov;
			}
		} else if ( lerp == 0.0f ) {
			camFrame = &camera[ frame ];
			view->viewaxis = camFrame[ 0 ].q.ToMat3();
			view->vieworg = camFrame[ 0 ].t + offset;
			view->fov_x = camFrame[ 0 ].fov;
		} else {
			camFrame = &camera[ frame ];
			invlerp = 1.0f - lerp;
			q1 = camFrame[ 0 ].q.ToQuat();
			q2 = camFrame[ 1 ].q.ToQuat();
			q3.Slerp( q1, q2, lerp );
			view->viewaxis = q3.ToMat3();
			view->vieworg = camFrame[ 0 ].t * invlerp + camFrame[ 1 ].t * lerp + offset;
			view->fov_x = camFrame[ 0 ].fov * invlerp + camFrame[ 1 ].fov * lerp;
		}

		gameLocal.CalcFov( view->fov_x, view->fov_x, view->fov_y );

		// setup the pvs for this frame
		UpdatePVSAreas( view->vieworg );
	} else {
		// skip any frames where camera cuts occur
		realFrame = frame;
		cut = 0;
		camFrame2 = &camera[0]; // Koz

		cutFrame.fov = camFrame2[ 0 ].fov;
		cutFrame.q = camFrame2[ 0 ].q;
		cutFrame.t = camFrame2[ 0 ].t;

		cutRotOverride = false;
		for( i = 0; i < cameraCuts.Num(); i++ ) {
			if ( frame < cameraCuts[i].cutFrame )
			{
				break;
			}
			frame++;
			cut++;
			int cf = idMath::ClampInt( 0, camera.Num() - 2, cameraCuts[i].cutFrame + 1 );
			camFrame2 = &camera[ cf /*cameraCuts[i].cutFrame*/ ];

			cutFrame.fov = camFrame2[0].fov;
			cutFrame.q = camFrame2[0].q;
			cutFrame.t = camFrame2[0].t;

			if ( vr_cinematics.GetInteger() == 0 ) // only use replacement positions/rotations if using immersive cinematics.
			{
				cutRotOverride = false;
				if ( cameraCuts[i].posOverride )
				{
					cutFrame.t = cameraCuts[i].posNew;
				}
				if ( cameraCuts[i].rotOverride )
				{
					cutFrame.q = cameraCuts[i].rotNew;
					cutRotOverride = true;
				}
			}
		}

		if ( g_debugCinematic.GetBool() ) {
			if ( gameLocal.GetCamera() )
			{
				common->Printf( "Time %d - Camera %s cut %d rot Quat %s : rot Angles %s : pos %s\n", commonVr->Sys_Milliseconds(), gameLocal.GetCamera()->GetName(), cut, cutFrame.q.ToString(), cutFrame.q.ToAngles().ToString(), cutFrame.t.ToString() );
			}

			int prevFrameTime	= ( gameLocal.time - starttime - USERCMD_MSEC ) * frameRate;
			int prevFrame		= prevFrameTime / 1000;
			int prevCut;

			prevCut = 0;
			for( i = 0; i < cameraCuts.Num(); i++ ) {
				if( prevFrame < cameraCuts[ i ].cutFrame ) {
					break;
				}
				prevFrame++;
				prevCut++;
			}

			if ( prevCut != cut ) {
				gameLocal.Printf( "%d: '%s' cut %d\n", gameLocal.framenum, GetName(), cut );
			}
		}

		// clamp to the first frame.  also check if this is a one frame anim.  one frame anims would end immediately,
		// but since they're mainly used for static cams anyway, just stay on it infinitely.
		if ( ( frame < 0 ) || ( camera.Num() < 2 ) ) {
			view->viewaxis = camera[ 0 ].q.ToQuat().ToMat3();
			view->vieworg = camera[ 0 ].t + offset;
			view->fov_x = camera[ 0 ].fov;
		} else if ( frame > camera.Num() - 2 ) {
			if ( cycle > 0 ) {
				cycle--;
			}

			if ( cycle != 0 ) {
				// advance start time so that we loop
				starttime += ( ( camera.Num() - cameraCuts.Num() ) * 1000 ) / frameRate;
				GetViewParms( view );
				return;
			}

			Stop();
			if ( gameLocal.GetCamera() != NULL ) {
				// we activated another camera when we stopped, so get it's viewparms instead
				gameLocal.GetCamera()->GetViewParms( view );
				return;
			} else {
				// just use our last frame
				camFrame = &camera[ camera.Num() - 1 ];
				view->viewaxis = camFrame->q.ToQuat().ToMat3();
				view->vieworg = camFrame->t + offset;
				view->fov_x = camFrame->fov;
			}
		} else if ( lerp == 0.0f ) {
			camFrame = &camera[ frame ];
			view->viewaxis = camFrame[ 0 ].q.ToMat3();
			view->vieworg = camFrame[ 0 ].t + offset;
			view->fov_x = camFrame[ 0 ].fov;
		} else {
			camFrame = &camera[ frame ];
			invlerp = 1.0f - lerp;
			q1 = camFrame[ 0 ].q.ToQuat();
			q2 = camFrame[ 1 ].q.ToQuat();
			q3.Slerp( q1, q2, lerp );
			view->viewaxis = q3.ToMat3();
			view->vieworg = camFrame[ 0 ].t * invlerp + camFrame[ 1 ].t * lerp + offset;
			view->fov_x = camFrame[ 0 ].fov * invlerp + camFrame[ 1 ].fov * lerp;
		}

		// Koz begin
		if ( game->isVR && (vr_cinematics.GetInteger() == 0) )
		{
			// Clamp the camera origin to camera cut locations.
			// This eliminates camera panning and smooth movements in cutscenes,
			// while allowing the player to look around from
			// the camera origin. Not perfect but less vomitous.

			// Update: camera files have been updated with additional cuts, and pos/rot overrides at cut locations
			// for better 'immersive' cutscenes. Player also has option to use cropped or projected cutscenes
			// where the pos/rot overrides are ignored and cameras are not clamped to cuts.

			// Flicksync camera
			idEntity* ent = NULL;
			static idEntity* hiddenEnt = NULL;

			static idEntity *last_ent = NULL;
			// only use character if it's not hidden, and it's within range of current camera

			if ( g_debugCinematic.GetBool() && ent != last_ent )
			{
				gameLocal.Printf( "%d: Flicksync using camera %s\n", gameLocal.framenum, this->name.c_str() );

			}

			last_ent = ent;

			view->viewaxis = cutFrame.q.ToMat3();
			view->vieworg = cutFrame.t + offset;

			// if the rotation wasn't overridden, remove camera pitch & roll, this is uncomfortable in VR.

			if ( !cutRotOverride )
			{
				idAngles angles = view->viewaxis.ToAngles();
				angles.pitch = 0;
				angles.roll = 0;
				view->viewaxis = angles.ToMat3();
			}
		}

		if ( game->isVR && gameLocal.inCinematic )
		{
			// override any camera fov changes. Unless you *like* the taste of hurl.

			const idPlayer	*player = gameLocal.GetLocalPlayer();
			view->fov_x = player->DefaultFov();
		}
		// Koz end

		gameLocal.CalcFov( view->fov_x, view->fov_x, view->fov_y );

		// setup the pvs for this frame
		UpdatePVSAreas( view->vieworg );

		if ( g_showcamerainfo.GetBool() ) {
			gameLocal.Printf( "^5Frame: ^7%d/%d\n\n\n", realFrame + 1, camera.Num() - cameraCuts.Num() );
		}
	}

}

/*
===============
idCameraAnim::Event_Activate
================
*/
void idCameraAnim::Event_Activate( idEntity *_activator ) {
	activator = _activator;
	if ( thinkFlags & TH_THINK ) {
		Stop();
	} else {
		Start();
	}
}

/*
===============
idCameraAnim::Event_Start
================
*/
void idCameraAnim::Event_Start( void ) {
	Start();
}

/*
===============
idCameraAnim::Event_Stop
================
*/
void idCameraAnim::Event_Stop( void ) {
	Stop();
}

/*
================
idCameraAnim::Event_SetCallback
================
*/
void idCameraAnim::Event_SetCallback( void ) {
	if ( ( gameLocal.GetCamera() == this ) && !threadNum ) {
		threadNum = idThread::CurrentThreadNum();
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
