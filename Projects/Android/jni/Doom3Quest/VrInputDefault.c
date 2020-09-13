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

float	vr_turn_mode = 0.0f;
float	vr_turn_angle = 45.0f;
float	vr_reloadtimeoutms;
float	vr_walkdirection;
float	vr_movement_multiplier;
float	vr_weapon_pitchadjust;
float	vr_lasersight;
float	vr_control_scheme;
float	vr_teleport;
float	vr_virtual_stock;
float	vr_switch_sticks = 0;
float	vr_cinematic_stereo;
float	vr_screen_dist;


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

void HandleInput_Default( ovrInputStateTrackedRemote *pDominantTrackedRemoteNew, ovrInputStateTrackedRemote *pDominantTrackedRemoteOld, ovrTracking* pDominantTracking,
                          ovrInputStateTrackedRemote *pOffTrackedRemoteNew, ovrInputStateTrackedRemote *pOffTrackedRemoteOld, ovrTracking* pOffTracking,
                          int domButton1, int domButton2, int offButton1, int offButton2 )

{
	//Ensure handedness is set correctly
	pVRClientInfo->right_handed = vr_control_scheme < 10 ||
            vr_control_scheme == 99; // Always right-handed for weapon calibration

	pVRClientInfo->teleportenabled = vr_teleport != 0;

    static bool dominantGripPushed = false;
	static float dominantGripPushTime = 0.0f;
    static bool canUseBackpack = false;


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
        QuatToYawPitchRoll(pWeapon->HeadPose.Pose.Orientation, rotation, pVRClientInfo->weaponangles_flashlight);

        rotation[PITCH] = vr_weapon_pitchadjust;
        QuatToYawPitchRoll(pWeapon->HeadPose.Pose.Orientation, rotation, pVRClientInfo->weaponangles);

        VectorSubtract(pVRClientInfo->weaponangles_last, pVRClientInfo->weaponangles, pVRClientInfo->weaponangles_delta);
        VectorCopy(pVRClientInfo->weaponangles, pVRClientInfo->weaponangles_last);
    }

    //Menu button
    handleTrackedControllerButton_AsKey(&leftTrackedRemoteState_new, &leftTrackedRemoteState_old, ovrButton_Enter, K_ESCAPE);

    static bool resetCursor = true;
    if ( Doom3Quest_useScreenLayer() )
    {
        interactWithTouchScreen(resetCursor, pDominantTrackedRemoteNew, pDominantTrackedRemoteOld);

        handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, true, ovrButton_Trigger, 1);
    }
    else
    {
        resetCursor = true;

        static bool canUseQuickSave = false;
        if (pOffTracking->Status & (VRAPI_TRACKING_STATUS_POSITION_TRACKED | VRAPI_TRACKING_STATUS_POSITION_VALID)) {
            canUseQuickSave = false;
        }
        else if (!canUseQuickSave) {
            int channel = (vr_control_scheme >= 10) ? 1 : 0;
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
        } else {
            if (((pOffTrackedRemoteNew->Buttons & offButton1) !=
                 (pOffTrackedRemoteOld->Buttons & offButton1))) {
                handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, false, offButton1, UB_IMPULSE19);
            }
        }



        float distance = sqrtf(powf(pOff->HeadPose.Pose.Position.x - pWeapon->HeadPose.Pose.Position.x, 2) +
                               powf(pOff->HeadPose.Pose.Position.y - pWeapon->HeadPose.Pose.Position.y, 2) +
                               powf(pOff->HeadPose.Pose.Position.z - pWeapon->HeadPose.Pose.Position.z, 2));

        float distanceToHMD = sqrtf(powf(pVRClientInfo->hmdposition[0] - pWeapon->HeadPose.Pose.Position.x, 2) +
                                    powf(pVRClientInfo->hmdposition[1] - pWeapon->HeadPose.Pose.Position.y, 2) +
                                    powf(pVRClientInfo->hmdposition[2] - pWeapon->HeadPose.Pose.Position.z, 2));

        //Turn on weapon stabilisation?
        bool stabilised = false;
        if (!pVRClientInfo->pistol && // Don't stabilise pistols
            (pOffTrackedRemoteNew->Buttons & ovrButton_GripTrigger) && (distance < STABILISATION_DISTANCE))
        {
            stabilised = true;
        }

        pVRClientInfo->weapon_stabilised = stabilised;

        //Engage scope / virtual stock if conditions are right
        bool scopeready = pVRClientInfo->weapon_stabilised && (distanceToHMD < SCOPE_ENGAGE_DISTANCE);
        static bool lastScopeReady = false;
        if (scopeready != lastScopeReady) {
            if (pVRClientInfo->scopedweapon && !pVRClientInfo->scopedetached) {
                if (!pVRClientInfo->scopeengaged && scopeready) {
                    ALOGV("**WEAPON EVENT**  trigger scope mode");
                    sendButtonActionSimple("weapalt");
                }
                else if (pVRClientInfo->scopeengaged && !scopeready) {
                    ALOGV("**WEAPON EVENT**  disable scope mode");
                    sendButtonActionSimple("weapalt");
                }
                lastScopeReady = scopeready;
            }
        }

        //Engage scope / virtual stock (iron sight lock) if conditions are right
        static bool scopeEngaged = false;
        if (scopeEngaged != pVRClientInfo->scopeengaged)
        {
            scopeEngaged = pVRClientInfo->scopeengaged;
        }

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

            //Just copy to calculated offset, used to use this in case we wanted to apply any modifiers, but don't any more
            VectorCopy(pVRClientInfo->current_weaponoffset, pVRClientInfo->calculated_weaponoffset);

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
                    sendButtonAction("+attack", velocityTriggeredAttack);
                    fired = velocityTriggeredAttack;
                }
            }
            else if (velocityTriggeredAttack)
            {
                //send a stop attack as we have an unfinished velocity attack
                velocityTriggeredAttack = false;
                ALOGV("**WEAPON EVENT**  veocity triggered -attack");
                sendButtonAction("+attack", velocityTriggeredAttack);
            }

            if (pVRClientInfo->weapon_stabilised)
            {
                if (pVRClientInfo->scopeengaged)
                {
                    //offset to the appropriate eye a little bit
                    vec2_t xy;
                    rotateAboutOrigin(0.065f / 2.0f, 0.0f, -pVRClientInfo->hmdorientation[YAW], xy);
                    float x = pOff->HeadPose.Pose.Position.x - (pVRClientInfo->hmdposition[0] + xy[0]);
                    float y = pOff->HeadPose.Pose.Position.y - (pVRClientInfo->hmdposition[1] - 0.1f); // Use a point lower
                    float z = pOff->HeadPose.Pose.Position.z - (pVRClientInfo->hmdposition[2] + xy[1]);
                    float zxDist = length(x, z);

                    if (zxDist != 0.0f && z != 0.0f) {
                        VectorSet(pVRClientInfo->weaponangles, -degrees(atanf(y / zxDist)),
                                  -degrees(atan2f(x, -z)), 0);
                    }
                }
                else
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

            if (pDominantTracking->Status & (VRAPI_TRACKING_STATUS_POSITION_TRACKED | VRAPI_TRACKING_STATUS_POSITION_VALID)) {
                canUseBackpack = false;
            }
            else if (!canUseBackpack && pVRClientInfo->backpackitemactive == 0) {
                int channel = (vr_control_scheme >= 10) ? 0 : 1;
                    Doom3Quest_Vibrate(40, channel, 0.5); // vibrate to let user know they can switch

                canUseBackpack = true;
            }

            dominantGripPushed = (pDominantTrackedRemoteNew->Buttons &
                                  ovrButton_GripTrigger) != 0;
            bool dominantButton1Pushed = (pDominantTrackedRemoteNew->Buttons &
                                     domButton1) != 0;
            bool dominantButton2Pushed = (pDominantTrackedRemoteNew->Buttons &
                                          domButton2) != 0;

            if (!canUseBackpack)
            {
                if (dominantGripPushed) {
                    if (dominantGripPushTime == 0) {
                        dominantGripPushTime = GetTimeInMilliSeconds();
                    }
                }
                else
                {
                    if (pVRClientInfo->backpackitemactive == 1) {
                        //Restores last used weapon if possible
                        char buffer[32];
                        sprintf(buffer, "weapon %i", pVRClientInfo->lastweaponid);
                        sendButtonActionSimple(buffer);
                        pVRClientInfo->backpackitemactive = 0;
                    }
                    else if ((GetTimeInMilliSeconds() - dominantGripPushTime) < vr_reloadtimeoutms) {

                        Android_SetImpuse(UB_IMPULSE13);
                    }
                    dominantGripPushTime = 0;
                }

                if (!dominantButton1Pushed && pVRClientInfo->backpackitemactive == 2)
                {
                    char buffer[32];
                    sprintf(buffer, "weapon %i", pVRClientInfo->lastweaponid);
                    sendButtonActionSimple(buffer);
                    pVRClientInfo->backpackitemactive = 0;
                }

                if (!dominantButton2Pushed && pVRClientInfo->backpackitemactive == 3)
                {
                    pVRClientInfo->backpackitemactive = 0;
                }
            } else {
                if (pVRClientInfo->backpackitemactive == 0) {
                    if (dominantGripPushed) {
                        pVRClientInfo->lastweaponid = pVRClientInfo->weaponid;
                        //Initiate grenade from backpack mode
                        sendButtonActionSimple("weaponbank 6");
                        int channel = (vr_control_scheme >= 10) ? 0 : 1;
                        Doom3Quest_Vibrate(80, channel, 0.8); // vibrate to let user know they switched
                        pVRClientInfo->backpackitemactive = 1;
                    }
                    else if (dominantButton1Pushed)
                    {
                        pVRClientInfo->lastweaponid = pVRClientInfo->weaponid;

                        //Initiate knife from backpack mode
                        sendButtonActionSimple("weapon 1");
                        int channel = (vr_control_scheme >= 10) ? 0 : 1;
                        Doom3Quest_Vibrate(80, channel, 0.8); // vibrate to let user know they switched
                        pVRClientInfo->backpackitemactive = 2;
                    }
                    /*else if (dominantButton2Pushed && pVRClientInfo->hasbinoculars)
                    {
                        int channel = (vr_control_scheme >= 10) ? 0 : 1;
                        Doom3Quest_Vibrate(80, channel, 0.8); // vibrate to let user know they switched
                        pVRClientInfo->backpackitemactive = 3;
                    }*/
                }
            }
        }

        float controllerYawHeading = 0.0f;
        //off-hand stuff
        {
            pVRClientInfo->offhandoffset[0] = pOff->HeadPose.Pose.Position.x - pVRClientInfo->hmdposition[0];
            pVRClientInfo->offhandoffset[1] = pOff->HeadPose.Pose.Position.y - pVRClientInfo->hmdposition[1];
            pVRClientInfo->offhandoffset[2] = pOff->HeadPose.Pose.Position.z - pVRClientInfo->hmdposition[2];

            vec3_t rotation = {0};
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
            //NOTE: it'll never be above ~60fps since we use com_fixedTic of "-1"
            static float lastSampleTime = 0;
            float sampleTime = Sys_Milliseconds();
            float vr_positional_factor = 2400.0f * ((1000.0f / TIC_RATE) / (sampleTime-lastSampleTime));
            lastSampleTime = sampleTime;

            //This section corrects for the fact that the controller actually controls direction of movement, but we want to move relative to the direction the
            //player is facing for positional tracking
            vec2_t v;
            rotateAboutOrigin(-pVRClientInfo->hmdposition_delta[0] * vr_positional_factor,
                              pVRClientInfo->hmdposition_delta[2] * vr_positional_factor, - pVRClientInfo->hmdorientation[YAW], v);
            positional_movementSideways = v[0];
            positional_movementForward = v[1];

            //Jump (B Button)
            if (pVRClientInfo->backpackitemactive != 2 && !canUseBackpack) {

                if ((primaryButtonsNew & primaryButton2) != (primaryButtonsOld & primaryButton2))
                {
                    handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, false, primaryButton2, UB_UP);
                }
            }


            //Fire Primary
            if (pVRClientInfo->backpackitemactive != 3 && // Can't fire while holding binoculars
                !pVRClientInfo->velocitytriggered && // Don't fire velocity triggered weapons
                (pDominantTrackedRemoteNew->Buttons & ovrButton_Trigger) !=
                (pDominantTrackedRemoteOld->Buttons & ovrButton_Trigger)) {

                ALOGV("**WEAPON EVENT**  Not Grip Pushed %sattack", (pDominantTrackedRemoteNew->Buttons & ovrButton_Trigger) ? "+" : "-");

                handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, false, ovrButton_Trigger, UB_ATTACK);
            }

            //Duck
            if (pVRClientInfo->backpackitemactive != 2 &&
                !canUseBackpack &&
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
            //"Use"
            if ((pDominantTrackedRemoteNew->Buttons & ovrButton_Joystick) !=
                (pDominantTrackedRemoteOld->Buttons & ovrButton_Joystick)) {

                //Use Vehicle
                Android_SetImpuse(UB_IMPULSE40);
            }

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
            vr_movement_multiplier = 127;
            remote_movementSideways = v[0] * (pVRClientInfo->scopeengaged ? 0.3f : 1.0f) * vr_movement_multiplier;
            remote_movementForward = v[1] * (pVRClientInfo->scopeengaged ? 0.3f : 1.0f) * vr_movement_multiplier;

            if (!canUseQuickSave) {
                if (((secondaryButtonsNew & secondaryButton1) !=
                     (secondaryButtonsOld & secondaryButton1)) &&
                    (secondaryButtonsNew & secondaryButton1)) {

                    if (dominantGripPushed) {
                        Android_SetCommand("give all");
                    } else {
                        pVRClientInfo->visible_hud = !pVRClientInfo->visible_hud;
                    }
                }
            }

            if ((pOffTrackedRemoteNew->Buttons & ovrButton_Joystick) !=
                (pOffTrackedRemoteOld->Buttons & ovrButton_Joystick)) {

                //UNUSED

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

            //No snap turn when using mounted gun
            static int increaseSnap = true;
            if (!pVRClientInfo->scopeengaged) {
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
            else {
                if (fabs(pPrimaryJoystick->x) > 0.5f) {
                    increaseSnap = false;
                }
                else
                {
                    increaseSnap = true;
                }
            }
        }

        updateScopeAngles();
    }

    //Save state
    rightTrackedRemoteState_old = rightTrackedRemoteState_new;
    leftTrackedRemoteState_old = leftTrackedRemoteState_new;
}