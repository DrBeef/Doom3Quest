/************************************************************************************

Filename	:	VrInputDefault.c
Content		:	Handles default controller input
Created		:	August 2019
Authors		:	Simon Brown

*************************************************************************************/

#include "../../../../../../VrApi/Include/VrApi.h"
#include "../../../../../../VrApi/Include/VrApi_Helpers.h"
#include "../../../../../../VrApi/Include/VrApi_SystemUtils.h"
#include "../../../../../../VrApi/Include/VrApi_Input.h"
#include "../../../../../../VrApi/Include/VrApi_Types.h"
#include <android/keycodes.h>
#include <sys/time.h>

#include "VrInput.h"

#include "doomkeys.h"

float	vr_reloadtimeoutms = 200.0f;
float	vr_walkdirection = 0;
float	vr_weapon_pitchadjust = -30.0f;
float	vr_teleport;
float	vr_switch_sticks = 0;

extern bool forceVirtualScreen;

/*
================
Sys_Milliseconds
================
*/
int curtime;
int sys_timeBase;
int Sys_Milliseconds( void ) {
    struct timeval tp;
    struct timezone tzp;

    gettimeofday( &tp, &tzp );

    if ( !sys_timeBase ) {
        sys_timeBase = tp.tv_sec;
        return tp.tv_usec / 1000;
    }

    curtime = ( tp.tv_sec - sys_timeBase ) * 1000 + tp.tv_usec / 1000;

    return curtime;
}

void Android_SetImpuse(int impulse);
void Android_SetCommand(const char * cmd);
void Android_ButtonChange(int key, int state);
int Android_GetCVarInteger(const char* cvar);

extern bool inMenu;
extern bool inGameGuiActive;
extern bool objectiveSystemActive;
extern bool inCinematic;
const int USERCMD_HZ = 60;


void HandleInput_Default( int controlscheme, ovrInputStateTrackedRemote *pDominantTrackedRemoteNew, ovrInputStateTrackedRemote *pDominantTrackedRemoteOld, ovrTracking* pDominantTracking,
                          ovrInputStateTrackedRemote *pOffTrackedRemoteNew, ovrInputStateTrackedRemote *pOffTrackedRemoteOld, ovrTracking* pOffTracking,
                          int domButton1, int domButton2, int offButton1, int offButton2 )

{
	//Ensure handedness is set correctly
	pVRClientInfo->right_handed = controlscheme < 10 ||
            controlscheme == 99; // Always right-handed for weapon calibration

	pVRClientInfo->teleportenabled = vr_teleport != 0;

    static bool dominantGripPushed = false;
	static float dominantGripPushTime = 0.0f;
    static bool canGrabFlashlight = false;


    //Need this for the touch screen
    ovrTracking * pWeapon = pDominantTracking;
    ovrTracking * pOff = pOffTracking;


    //All this to allow stick and button switching!
    ovrVector2f *pPrimaryJoystick;
    ovrVector2f *pSecondaryJoystick;
    uint32_t primaryButtonsNew;
    uint32_t primaryButtonsOld;
    uint32_t secondaryButtonsNew;
    uint32_t secondaryButtonsOld;
    int primaryButton1;
    int primaryButton2;
    int secondaryButton1;
    int secondaryButton2;
    if (vr_switch_sticks)
    {
        //
        // This will switch the joystick and A/B/X/Y button functions only
        // Move, Strafe, Turn, Jump, Crouch, Notepad, HUD mode, Weapon Switch
        pSecondaryJoystick = &pDominantTrackedRemoteNew->Joystick;
        pPrimaryJoystick = &pOffTrackedRemoteNew->Joystick;
        secondaryButtonsNew = pDominantTrackedRemoteNew->Buttons;
        secondaryButtonsOld = pDominantTrackedRemoteOld->Buttons;
        primaryButtonsNew = pOffTrackedRemoteNew->Buttons;
        primaryButtonsOld = pOffTrackedRemoteOld->Buttons;
        primaryButton1 = offButton1;
        primaryButton2 = offButton2;
        secondaryButton1 = domButton1;
        secondaryButton2 = domButton2;
    }
    else
    {
        pPrimaryJoystick = &pDominantTrackedRemoteNew->Joystick;
        pSecondaryJoystick = &pOffTrackedRemoteNew->Joystick;
        primaryButtonsNew = pDominantTrackedRemoteNew->Buttons;
        primaryButtonsOld = pDominantTrackedRemoteOld->Buttons;
        secondaryButtonsNew = pOffTrackedRemoteNew->Buttons;
        secondaryButtonsOld = pOffTrackedRemoteOld->Buttons;
        primaryButton1 = domButton1;
        primaryButton2 = domButton2;
        secondaryButton1 = offButton1;
        secondaryButton2 = offButton2;
    }



    {
        //Set gun angles - We need to calculate all those we might need (including adjustments) for the client to then take its pick
        vec3_t rotation = {0};
        rotation[PITCH] = 30;
        QuatToYawPitchRoll(pWeapon->HeadPose.Pose.Orientation, rotation, pVRClientInfo->weaponangles_unadjusted);

        rotation[PITCH] = vr_weapon_pitchadjust;
        QuatToYawPitchRoll(pWeapon->HeadPose.Pose.Orientation, rotation, pVRClientInfo->weaponangles);

        VectorSubtract(pVRClientInfo->weaponangles_last, pVRClientInfo->weaponangles, pVRClientInfo->weaponangles_delta);
        VectorCopy(pVRClientInfo->weaponangles, pVRClientInfo->weaponangles_last);
    }

    //Menu button - can be used in all modes
    handleTrackedControllerButton_AsKey(&leftTrackedRemoteState_new, &leftTrackedRemoteState_old, ovrButton_Enter, K_ESCAPE);

    if ( inMenu || inGameGuiActive || inCinematic ) // Specific cases where we need to interact using mouse etc
    {
        controlMouse(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld);
        handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, true, ovrButton_Trigger, 1);
    }

    if ( objectiveSystemActive )
    {
        controlMouse(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld);
        handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, true, ovrButton_Trigger, 1);
        if (((pOffTrackedRemoteNew->Buttons & offButton1) != (pOffTrackedRemoteOld->Buttons & offButton1)) && (pOffTrackedRemoteNew->Buttons & offButton1))
        {
            Android_SetImpuse(UB_IMPULSE19);
        }
    }

    if ( !inCinematic && !inMenu )
    {
        static bool canUseQuickSave = false;
        if (pOffTracking->Status & (VRAPI_TRACKING_STATUS_POSITION_TRACKED | VRAPI_TRACKING_STATUS_POSITION_VALID)) {
            canUseQuickSave = false;
        }
        else if (!canUseQuickSave) {
            int channel = (controlscheme >= 10) ? 1 : 0;
            Doom3Quest_Vibrate(40, channel, 0.5); // vibrate to let user know they can switch
            canUseQuickSave = true;
        }

        if (canUseQuickSave)
        {
            if (((pOffTrackedRemoteNew->Buttons & offButton1) !=
                 (pOffTrackedRemoteOld->Buttons & offButton1)) &&
                (pOffTrackedRemoteNew->Buttons & offButton1)) {
                Android_SetCommand("savegame quick");
            }

            if (((pOffTrackedRemoteNew->Buttons & offButton2) !=
                 (pOffTrackedRemoteOld->Buttons & offButton2)) &&
                (pOffTrackedRemoteNew->Buttons & offButton2)) {
                Android_SetCommand("loadgame quick");
            }
        } else
        {
            //PDA
            if (((pOffTrackedRemoteNew->Buttons & offButton1) !=
                 (pOffTrackedRemoteOld->Buttons & offButton1)) &&
                (pOffTrackedRemoteNew->Buttons & offButton1)) {
                Android_SetImpuse(UB_IMPULSE19);
            }

            if (((pOffTrackedRemoteNew->Buttons & offButton2) !=
                 (pOffTrackedRemoteOld->Buttons & offButton2)) &&
                (pOffTrackedRemoteNew->Buttons & offButton2)) {
                forceVirtualScreen = !forceVirtualScreen;
            }
        }

        float distance = sqrtf(powf(pOff->HeadPose.Pose.Position.x - pWeapon->HeadPose.Pose.Position.x, 2) +
                               powf(pOff->HeadPose.Pose.Position.y - pWeapon->HeadPose.Pose.Position.y, 2) +
                               powf(pOff->HeadPose.Pose.Position.z - pWeapon->HeadPose.Pose.Position.z, 2));

        //Turn on weapon stabilisation?
        bool stabilised = false;
        if (!pVRClientInfo->pistol && // Don't stabilise pistols
            (pOffTrackedRemoteNew->Buttons & ovrButton_GripTrigger) && (distance < STABILISATION_DISTANCE))
        {
            stabilised = true;
        }

        pVRClientInfo->weapon_stabilised = stabilised;

        //dominant hand stuff first
        {
            //Record recent weapon position for trajectory based stuff
            for (int i = (NUM_WEAPON_SAMPLES-1); i != 0; --i)
            {
                VectorCopy(pVRClientInfo->weaponoffset_history[i-1], pVRClientInfo->weaponoffset_history[i]);
                pVRClientInfo->weaponoffset_history_timestamp[i] = pVRClientInfo->weaponoffset_history_timestamp[i-1];
            }
            VectorCopy(pVRClientInfo->current_weaponoffset, pVRClientInfo->weaponoffset_history[0]);
            pVRClientInfo->weaponoffset_history_timestamp[0] = pVRClientInfo->current_weaponoffset_timestamp;

			///Weapon location relative to view
            pVRClientInfo->current_weaponoffset[0] = pWeapon->HeadPose.Pose.Position.x - pVRClientInfo->hmdposition[0];
            pVRClientInfo->current_weaponoffset[1] = pWeapon->HeadPose.Pose.Position.y - pVRClientInfo->hmdposition[1];
            pVRClientInfo->current_weaponoffset[2] = pWeapon->HeadPose.Pose.Position.z - pVRClientInfo->hmdposition[2];
            pVRClientInfo->current_weaponoffset_timestamp = Sys_Milliseconds( );

            {
                //Caclulate speed between two historic controller position readings
                float distance = VectorDistance(pVRClientInfo->weaponoffset_history[NEWER_READING], pVRClientInfo->weaponoffset_history[OLDER_READING]);
                float t = pVRClientInfo->weaponoffset_history_timestamp[NEWER_READING] - pVRClientInfo->weaponoffset_history_timestamp[OLDER_READING];
                pVRClientInfo->throw_power = distance / (t/(float)1000.0);

                //Calculate trajectory
                VectorSubtract(pVRClientInfo->weaponoffset_history[NEWER_READING], pVRClientInfo->weaponoffset_history[OLDER_READING], pVRClientInfo->throw_trajectory);
                VectorNormalize( pVRClientInfo->throw_trajectory );

                //Set origin to the newer reading offset
                VectorCopy(pVRClientInfo->weaponoffset_history[NEWER_READING], pVRClientInfo->throw_origin);
            }

            //Does weapon velocity trigger attack (knife) and is it fast enough
            static bool velocityTriggeredAttack = false;
            if (pVRClientInfo->velocitytriggered)
            {
                static bool fired = false;
                float velocity = sqrtf(powf(pWeapon->HeadPose.LinearVelocity.x, 2) +
                                       powf(pWeapon->HeadPose.LinearVelocity.y, 2) +
                                       powf(pWeapon->HeadPose.LinearVelocity.z, 2));

                velocityTriggeredAttack = (velocity > VELOCITY_TRIGGER);

                if (fired != velocityTriggeredAttack) {
                    ALOGV("**WEAPON EVENT**  veocity triggered %s", velocityTriggeredAttack ? "+attack" : "-attack");
                    Android_ButtonChange(UB_ATTACK, velocityTriggeredAttack ? 1 : 0);
                    fired = velocityTriggeredAttack;
                }
            }
            else if (velocityTriggeredAttack)
            {
                //send a stop attack as we have an unfinished velocity attack
                velocityTriggeredAttack = false;
                ALOGV("**WEAPON EVENT**  veocity triggered -attack");
                Android_ButtonChange(UB_ATTACK, velocityTriggeredAttack ? 1 : 0);
            }

            if (pVRClientInfo->weapon_stabilised)
            {
                {
                    float x = pOff->HeadPose.Pose.Position.x - pWeapon->HeadPose.Pose.Position.x;
                    float y = pOff->HeadPose.Pose.Position.y - pWeapon->HeadPose.Pose.Position.y;
                    float z = pOff->HeadPose.Pose.Position.z - pWeapon->HeadPose.Pose.Position.z;
                    float zxDist = length(x, z);

                    if (zxDist != 0.0f && z != 0.0f) {
                        {
                            VectorSet(pVRClientInfo->weaponangles, -degrees(atanf(y / zxDist)),
                                      -degrees(atan2f(x, -z)), pVRClientInfo->weaponangles[ROLL] / 2.0f); //Dampen roll on stabilised weapon
                        }
                    }
                }
            }

            if (pVRClientInfo->weaponid != -1 &&
                    pVRClientInfo->weaponid != 11)
            {
                vec2_t v;
                rotateAboutOrigin(pVRClientInfo->right_handed ? -0.2f : 0.2f, 0.0f,
                                  -pVRClientInfo->hmdorientation[YAW], v);
                pVRClientInfo->flashlightHolsterOffset[0] = -v[0];
                pVRClientInfo->flashlightHolsterOffset[1] = -pVRClientInfo->hmdposition[1] * 0.45f; // almost half way down body "waist"
                pVRClientInfo->flashlightHolsterOffset[2] = -v[1];

                float distance = sqrtf(
                        powf((pVRClientInfo->hmdposition[0] + pVRClientInfo->flashlightHolsterOffset[0]) - pWeapon->HeadPose.Pose.Position.x, 2) +
                        powf((pVRClientInfo->hmdposition[1] + pVRClientInfo->flashlightHolsterOffset[1]) - pWeapon->HeadPose.Pose.Position.y, 2) +
                        powf((pVRClientInfo->hmdposition[2] + pVRClientInfo->flashlightHolsterOffset[2]) - pWeapon->HeadPose.Pose.Position.z, 2));

                if (distance > FLASHLIGHT_HOLSTER_DISTANCE) {
                    canGrabFlashlight = false;
                }
                else if (!canGrabFlashlight && pVRClientInfo->holsteritemactive == 0) {
                    int channel = (controlscheme >= 10) ? 0 : 1;
                    Doom3Quest_Vibrate(40, channel, 0.4); // vibrate to let user know they can switch

                    canGrabFlashlight = true;
                }
            }

            dominantGripPushed = (pDominantTrackedRemoteNew->Buttons &
                                  ovrButton_GripTrigger) != 0;

            if (dominantGripPushed) {
                if (!canGrabFlashlight) {
                    if (dominantGripPushTime == 0) {
                        dominantGripPushTime = GetTimeInMilliSeconds();
                    }
                }
                else
                {
                    if (pVRClientInfo->holsteritemactive == 0) {
                        pVRClientInfo->lastweaponid = pVRClientInfo->weaponid;

                        //Initiate flashlight from backpack mode
                        Android_SetImpuse(UB_IMPULSE11);
                        int channel = (controlscheme >= 10) ? 0 : 1;
                        Doom3Quest_Vibrate(80, channel, 0.8); // vibrate to let user know they switched

                        pVRClientInfo->holsteritemactive = 1;
                    }
                }
            }
            else
            {
                if (pVRClientInfo->holsteritemactive == 1) {
                    //Restores last used weapon if possible
                    if (pVRClientInfo->weaponid != -1) {
                        Android_SetImpuse(UB_IMPULSE0 + pVRClientInfo->weaponid);
                    }
                    pVRClientInfo->holsteritemactive = 0;
                }
                else if ((GetTimeInMilliSeconds() - dominantGripPushTime) < vr_reloadtimeoutms) {
                    //Reload
                    Android_SetImpuse(UB_IMPULSE13);
                }

                dominantGripPushTime = 0;
            }
        }

        float controllerYawHeading = 0.0f;
        //off-hand stuff
        {
            pVRClientInfo->offhandoffset[0] = pOff->HeadPose.Pose.Position.x - pVRClientInfo->hmdposition[0];
            pVRClientInfo->offhandoffset[1] = pOff->HeadPose.Pose.Position.y - pVRClientInfo->hmdposition[1];
            pVRClientInfo->offhandoffset[2] = pOff->HeadPose.Pose.Position.z - pVRClientInfo->hmdposition[2];

            vec3_t rotation = {0};
            rotation[PITCH] = -45;
            QuatToYawPitchRoll(pOff->HeadPose.Pose.Orientation, rotation, pVRClientInfo->offhandangles);

			if (vr_walkdirection == 0) {
				controllerYawHeading = pVRClientInfo->offhandangles[YAW] - pVRClientInfo->hmdorientation[YAW];
			}
			else
			{
				controllerYawHeading = 0.0f;
			}
        }

        //Right-hand specific stuff
        {
            //Adjust positional factor for this sample based on how long the last frame took, it should
            //approximately even out the positional movement on a per frame basis (especially when fps is much lower than 60)
            static float lastSampleTime = 0;
            float sampleTime = Sys_Milliseconds();
            float vr_positional_factor = 2400.0f * ((1000.0f / USERCMD_HZ) / (sampleTime-lastSampleTime));
            lastSampleTime = sampleTime;

            //This section corrects for the fact that the controller actually controls direction of movement, but we want to move relative to the direction the
            //player is facing for positional tracking
            vec2_t v;
            rotateAboutOrigin(-pVRClientInfo->hmdposition_delta[0] * vr_positional_factor,
                              pVRClientInfo->hmdposition_delta[2] * vr_positional_factor, - pVRClientInfo->hmdorientation[YAW], v);
            positional_movementSideways = v[0];
            positional_movementForward = v[1];

            //Jump (B Button)
            if (pVRClientInfo->holsteritemactive != 2 && !canGrabFlashlight) {

                if ((primaryButtonsNew & primaryButton2) != (primaryButtonsOld & primaryButton2))
                {
                    handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, false, primaryButton2, UB_UP);
                }
            }


            //Fire Primary
            if (pVRClientInfo->holsteritemactive != 3 && // Can't fire while holding binoculars
                !pVRClientInfo->velocitytriggered && // Don't fire velocity triggered weapons
                (pDominantTrackedRemoteNew->Buttons & ovrButton_Trigger) !=
                (pDominantTrackedRemoteOld->Buttons & ovrButton_Trigger)) {

                ALOGV("**WEAPON EVENT**  Not Grip Pushed %sattack", (pDominantTrackedRemoteNew->Buttons & ovrButton_Trigger) ? "+" : "-");

                handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, false, ovrButton_Trigger, UB_ATTACK);
            }

            //Duck
            if (pVRClientInfo->holsteritemactive != 2 &&
                !canGrabFlashlight &&
                (primaryButtonsNew & primaryButton1) !=
                (primaryButtonsOld & primaryButton1)) {

                handleTrackedControllerButton_AsToggleButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, primaryButton1, UB_DOWN);
            }

			//Weapon Chooser
			static bool itemSwitched = false;
			if (between(-0.2f, pPrimaryJoystick->x, 0.2f) &&
				(between(0.8f, pPrimaryJoystick->y, 1.0f) ||
				 between(-1.0f, pPrimaryJoystick->y, -0.8f)))
			{
				if (!itemSwitched) {
					if (between(0.8f, pPrimaryJoystick->y, 1.0f))
					{
					    //Previous Weapon
                        Android_SetImpuse(UB_IMPULSE15);
					}
					else
					{
					    //Next Weapon
                        Android_SetImpuse(UB_IMPULSE14);
					}
					itemSwitched = true;
				}
			} else {
				itemSwitched = false;
			}
        }

        {
            //Apply a filter and quadratic scaler so small movements are easier to make
            float dist = length(pSecondaryJoystick->x, pSecondaryJoystick->y);
            float nlf = nonLinearFilter(dist);
            float x = nlf * pSecondaryJoystick->x;
            float y = nlf * pSecondaryJoystick->y;

            pVRClientInfo->player_moving = (fabs(x) + fabs(y)) > 0.05f;

            //Adjust to be off-hand controller oriented
            vec2_t v;
            rotateAboutOrigin(x, y, controllerYawHeading, v);

            //Move a lot slower if scope is engaged
            float vr_movement_multiplier = 127;
            remote_movementSideways = v[0] *  vr_movement_multiplier;
            remote_movementForward = v[1] *  vr_movement_multiplier;

            if (!canUseQuickSave) {
                if (((secondaryButtonsNew & secondaryButton1) !=
                     (secondaryButtonsOld & secondaryButton1)) &&
                    (secondaryButtonsNew & secondaryButton1)) {

                    if (dominantGripPushed) {
                        Android_SetCommand("give all");
                    }
                }
            }

            if (((pOffTrackedRemoteNew->Buttons & ovrButton_Joystick) !=
                (pOffTrackedRemoteOld->Buttons & ovrButton_Joystick)) &&
                    (pOffTrackedRemoteOld->Buttons & ovrButton_Joystick)) {

                pVRClientInfo->visible_hud = !pVRClientInfo->visible_hud;

            }

            //We need to record if we have started firing primary so that releasing trigger will stop definitely firing, if user has pushed grip
            //in meantime, then it wouldn't stop the gun firing and it would get stuck
            if (!pVRClientInfo->teleportenabled)
            {
                //Run
                handleTrackedControllerButton_AsButton(pOffTrackedRemoteNew, pOffTrackedRemoteOld, false, ovrButton_Trigger, UB_SPEED);

            } else {
                if (pOffTrackedRemoteNew->Buttons & ovrButton_Trigger)
                {
                    pVRClientInfo->teleportseek = true;
                }
                else if (pVRClientInfo->teleportseek)
                {
                    pVRClientInfo->teleportseek = false;
                    pVRClientInfo->teleportexecute = pVRClientInfo->teleportready;
                    pVRClientInfo->teleportready = false;
                }
            }

            int vr_turn_mode = Android_GetCVarInteger("vr_turnmode");
            float vr_turn_angle = Android_GetCVarInteger("vr_turnangle");

            //No snap turn when using mounted gun
            static int increaseSnap = true;
            {
                if (pPrimaryJoystick->x > 0.7f) {
                    if (increaseSnap) {
                        float turnAngle = vr_turn_mode ? (vr_turn_angle / 9.0f) : vr_turn_angle;
                        snapTurn -= turnAngle;

                        if (vr_turn_mode == 0) {
                            increaseSnap = false;
                        }

                        if (snapTurn < -180.0f) {
                            snapTurn += 360.f;
                        }
                    }
                } else if (pPrimaryJoystick->x < 0.3f) {
                    increaseSnap = true;
                }

                static int decreaseSnap = true;
                if (pPrimaryJoystick->x < -0.7f) {
                    if (decreaseSnap) {

                        float turnAngle = vr_turn_mode ? (vr_turn_angle / 9.0f) : vr_turn_angle;
                        snapTurn += turnAngle;

                        //If snap turn configured for less than 10 degrees
                        if (vr_turn_mode == 0) {
                            decreaseSnap = false;
                        }

                        if (snapTurn > 180.0f) {
                            snapTurn -= 360.f;
                        }

                    }
                } else if (pPrimaryJoystick->x > -0.3f) {
                    decreaseSnap = true;
                }
            }
        }
    }

    //Save state
    rightTrackedRemoteState_old = rightTrackedRemoteState_new;
    leftTrackedRemoteState_old = leftTrackedRemoteState_new;
}