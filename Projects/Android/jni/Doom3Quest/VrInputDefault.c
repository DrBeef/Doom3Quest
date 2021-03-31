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

float	vr_reloadtimeoutms = 300.0f;
float	vr_walkdirection = 0;
float	vr_weapon_pitchadjust = -30.0f;
float	vr_teleport;
float	vr_switch_sticks = 0;
int     give_weapon_count = 1;

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

void Android_SetImpulse(int impulse);
void Android_SetCommand(const char * cmd);
void Android_ButtonChange(int key, int state);
int Android_GetCVarInteger(const char* cvar);

extern bool inMenu;
extern bool inGameGuiActive;
extern bool objectiveSystemActive;
extern bool inCinematic;

void Doom3Quest_HapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight );

void HandleInput_Default( int controlscheme, int switchsticks, ovrInputStateGamepad *pFootTrackingNew, ovrInputStateGamepad *pFootTrackingOld, ovrInputStateTrackedRemote *pDominantTrackedRemoteNew, ovrInputStateTrackedRemote *pDominantTrackedRemoteOld, ovrTracking* pDominantTracking,
                          ovrInputStateTrackedRemote *pOffTrackedRemoteNew, ovrInputStateTrackedRemote *pOffTrackedRemoteOld, ovrTracking* pOffTracking,
                          int domButton1, int domButton2, int offButton1, int offButton2 )

{
    static bool dominantGripPushed = false;
	static float dominantGripPushTime = 0.0f;


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
    uint32_t weaponButtonsNew;
    uint32_t weaponButtonsOld;
    uint32_t offhandButtonsNew;
    uint32_t offhandButtonsOld;
    int primaryButton1;
    int primaryButton2;
    int secondaryButton1;
    int secondaryButton2;

    weaponButtonsNew = pDominantTrackedRemoteNew->Buttons;
    weaponButtonsOld = pDominantTrackedRemoteOld->Buttons;
    offhandButtonsNew = pOffTrackedRemoteNew->Buttons;
    offhandButtonsOld = pOffTrackedRemoteOld->Buttons;

    if ((controlscheme == 0 &&switchsticks == 1) ||
       (controlscheme == 1 &&switchsticks == 0))
    {
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
    } else {
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
        //Store original values
        const ovrQuatf quatRHand = pWeapon->HeadPose.Pose.Orientation;
        const ovrVector3f positionRHand = pWeapon->HeadPose.Pose.Position;
        const ovrQuatf quatLHand = pOff->HeadPose.Pose.Orientation;
        const ovrVector3f positionLHand = pOff->HeadPose.Pose.Position;

        /*VectorSet(pVRClientInfo->rhandposition, positionRHand.x, positionRHand.y, positionRHand.z);
        Vector4Set(pVRClientInfo->rhand_orientation_quat, quatRHand.x, quatRHand.y, quatRHand.z, quatRHand.w);
        VectorSet(pVRClientInfo->lhandposition, positionLHand.x, positionLHand.y, positionLHand.z);
        Vector4Set(pVRClientInfo->lhand_orientation_quat, quatLHand.x, quatLHand.y, quatLHand.z, quatLHand.w);*/

        //Right Hand
        //GB - FP Already does this so we end up with backward hands
        if(controlscheme == 1) {//Left Handed
            VectorSet(pVRClientInfo->lhandposition, positionRHand.x, positionRHand.y, positionRHand.z);
            Vector4Set(pVRClientInfo->lhand_orientation_quat, quatRHand.x, quatRHand.y, quatRHand.z, quatRHand.w);
            VectorSet(pVRClientInfo->rhandposition, positionLHand.x, positionLHand.y, positionLHand.z);
            Vector4Set(pVRClientInfo->rhand_orientation_quat, quatLHand.x, quatLHand.y, quatLHand.z, quatLHand.w);
        } else {
            VectorSet(pVRClientInfo->rhandposition, positionRHand.x, positionRHand.y, positionRHand.z);
            Vector4Set(pVRClientInfo->rhand_orientation_quat, quatRHand.x, quatRHand.y, quatRHand.z, quatRHand.w);
            VectorSet(pVRClientInfo->lhandposition, positionLHand.x, positionLHand.y, positionLHand.z);
            Vector4Set(pVRClientInfo->lhand_orientation_quat, quatLHand.x, quatLHand.y, quatLHand.z, quatLHand.w);
        }


        //Set gun angles - We need to calculate all those we might need (including adjustments) for the client to then take its pick
        vec3_t rotation = {0};
        rotation[PITCH] = 30;
        rotation[PITCH] = vr_weapon_pitchadjust;
        QuatToYawPitchRoll(pWeapon->HeadPose.Pose.Orientation, rotation, pVRClientInfo->weaponangles_temp);
        VectorSubtract(pVRClientInfo->weaponangles_last_temp, pVRClientInfo->weaponangles_temp, pVRClientInfo->weaponangles_delta_temp);
        VectorCopy(pVRClientInfo->weaponangles_temp, pVRClientInfo->weaponangles_last_temp);
        /*
        vec3_t rotation = {0};
        rotation[PITCH] = 30;
        QuatToYawPitchRoll(pWeapon->HeadPose.Pose.Orientation, rotation, pVRClientInfo->weaponangles_unadjusted);

        rotation[PITCH] = vr_weapon_pitchadjust;
        QuatToYawPitchRoll(pWeapon->HeadPose.Pose.Orientation, rotation, pVRClientInfo->weaponangles);

        VectorSubtract(pVRClientInfo->weaponangles_last, pVRClientInfo->weaponangles, pVRClientInfo->weaponangles_delta);
        VectorCopy(pVRClientInfo->weaponangles, pVRClientInfo->weaponangles_last);
         */
    }

    //Menu button - can be used in all modes
    handleTrackedControllerButton_AsKey(leftTrackedRemoteState_new.Buttons, leftTrackedRemoteState_old.Buttons, ovrButton_Enter, K_ESCAPE);
    handleTrackedControllerButton_AsKey(rightTrackedRemoteState_new.Buttons, rightTrackedRemoteState_old.Buttons, ovrButton_Enter, K_ESCAPE); // in case user has switched menu/home buttons

    controlMouse(inMenu, pDominantTrackedRemoteNew, pDominantTrackedRemoteOld);

    if ( inMenu || inCinematic ) // Specific cases where we need to interact using mouse etc
    {
        handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew->Buttons, pDominantTrackedRemoteOld->Buttons, true, ovrButton_Trigger, 1);
    }

    if ( !inCinematic && !inMenu )
    {
        static bool canUseQuickSave = false;
        if (pOffTracking->Status & (VRAPI_TRACKING_STATUS_POSITION_TRACKED | VRAPI_TRACKING_STATUS_POSITION_VALID)) {
            canUseQuickSave = false;
        }
        else if (!canUseQuickSave) {
            canUseQuickSave = true;
        }

        if (canUseQuickSave)
        {
            if (((secondaryButtonsNew & secondaryButton1) !=
                 (secondaryButtonsOld & secondaryButton1)) &&
                (secondaryButtonsNew & secondaryButton1)) {
                Android_SetCommand("savegame quick");
            }

            if (((secondaryButtonsNew & secondaryButton2) !=
                 (secondaryButtonsOld & secondaryButton2)) &&
                (secondaryButtonsNew & secondaryButton2)) {
                Android_SetCommand("loadgame quick");
            }
        } else
        {
            //PDA
            if (((secondaryButtonsNew & secondaryButton1) !=
                 (secondaryButtonsOld & secondaryButton1)) &&
                (secondaryButtonsNew & secondaryButton1)) {
                Android_SetImpulse(UB_IMPULSE19);
            }

            //Toggle LaserSight
            if (((secondaryButtonsNew & secondaryButton2) !=
                 (secondaryButtonsOld & secondaryButton2)) &&
                (secondaryButtonsNew & secondaryButton2)) {
                Android_SetImpulse(UB_IMPULSE33);
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
            /*
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
            }*/

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
                    ALOGV("**WEAPON EVENT** velocity triggered %s", velocityTriggeredAttack ? "+attack" : "-attack");
                    Android_ButtonChange(UB_ATTACK, velocityTriggeredAttack ? 1 : 0);
                    fired = velocityTriggeredAttack;
                }
            }
            else if (velocityTriggeredAttack)
            {
                //send a stop attack as we have an unfinished velocity attack
                velocityTriggeredAttack = false;
                ALOGV("**WEAPON EVENT**  velocity triggered -attack");
                Android_ButtonChange(UB_ATTACK, velocityTriggeredAttack ? 1 : 0);
            }

            //Does weapon velocity trigger attack (knife) and is it fast enough
            static bool velocityTriggeredAttackOffHand = false;
            pVRClientInfo->velocitytriggeredoffhandstate = false;
            if (pVRClientInfo->velocitytriggeredoffhand)
            {
                static bool firedOffHand = false;
                float velocity = sqrtf(powf(pOff->HeadPose.LinearVelocity.x, 2) +
                                       powf(pOff->HeadPose.LinearVelocity.y, 2) +
                                       powf(pOff->HeadPose.LinearVelocity.z, 2));

                velocityTriggeredAttackOffHand = (velocity > VELOCITY_TRIGGER);

                if (firedOffHand != velocityTriggeredAttackOffHand) {
                    ALOGV("**WEAPON EVENT** velocity triggered (offhand) %s", velocityTriggeredAttackOffHand ? "+attack" : "-attack");
                    //Android_ButtonChange(UB_IMPULSE37, velocityTriggeredAttackOffHand ? 1 : 0);
                    //Android_SetImpulse(UB_IMPULSE37);
                    pVRClientInfo->velocitytriggeredoffhandstate = true;
                    firedOffHand = velocityTriggeredAttackOffHand;
                }
            }
            else //GB This actually nevers gets run currently as we are always returning true for pVRClientInfo->velocitytriggeredoffhand (but we might not in the future when weapons are sorted)
            {
                //send a stop attack as we have an unfinished velocity attack
                velocityTriggeredAttackOffHand = false;
                ALOGV("**WEAPON EVENT**  velocity triggered -attack (offhand)");
                //Android_ButtonChange(UB_IMPULSE37, velocityTriggeredAttackOffHand ? 1 : 0);
                pVRClientInfo->velocitytriggeredoffhandstate = false;
            }

            /*if (pVRClientInfo->weapon_stabilised)
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
            }*/



            dominantGripPushed = (pDominantTrackedRemoteNew->Buttons &
                                  ovrButton_GripTrigger) != 0;

            if (dominantGripPushed) {
                if (dominantGripPushTime == 0) {
                    dominantGripPushTime = GetTimeInMilliSeconds();
                }
            }
            else
            {
                if ((GetTimeInMilliSeconds() - dominantGripPushTime) < vr_reloadtimeoutms) {
                    //Reload
                    Android_SetImpulse(UB_IMPULSE13);
                }

                dominantGripPushTime = 0;
            }
        }

        float controllerYawHeading = 0.0f;
        //GBFP - off-hand stuff
        {
            pVRClientInfo->offhandoffset_temp[0] = pOff->HeadPose.Pose.Position.x - pVRClientInfo->hmdposition[0];
            pVRClientInfo->offhandoffset_temp[1] = pOff->HeadPose.Pose.Position.y - pVRClientInfo->hmdposition[1];
            pVRClientInfo->offhandoffset_temp[2] = pOff->HeadPose.Pose.Position.z - pVRClientInfo->hmdposition[2];

            vec3_t rotation = {0};
            rotation[PITCH] = -45;
            QuatToYawPitchRoll(pOff->HeadPose.Pose.Orientation, rotation, pVRClientInfo->offhandangles_temp);
        }
        /*
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
        }*/

        //Right-hand specific stuff
        {
            //Adjust positional factor for this sample based on how long the last frame took, it should
            //approximately even out the positional movement on a per frame basis (especially when fps is much lower than 60)
            /*
            static float lastSampleTime = 0;
            float sampleTime = Sys_Milliseconds();
            float vr_positional_factor = 2400.0f * ((1000.0f / (float)Doom3Quest_GetRefresh()) / (sampleTime-lastSampleTime));
            lastSampleTime = sampleTime;

            //This section corrects for the fact that the controller actually controls direction of movement, but we want to move relative to the direction the
            //player is facing for positional tracking

            vec2_t v;
            rotateAboutOrigin(-pVRClientInfo->hmdposition_delta[0] * vr_positional_factor,
                              pVRClientInfo->hmdposition_delta[2] * vr_positional_factor, - pVRClientInfo->hmdorientation[YAW], v);
            positional_movementSideways = v[0];
            positional_movementForward = v[1];
             */



            //Fire Primary
            if ((pDominantTrackedRemoteNew->Buttons & ovrButton_Trigger) !=
                (pDominantTrackedRemoteOld->Buttons & ovrButton_Trigger)) {

                ALOGV("**WEAPON EVENT**  Not Grip Pushed %sattack", (pDominantTrackedRemoteNew->Buttons & ovrButton_Trigger) ? "+" : "-");

                handleTrackedControllerButton_AsButton(pDominantTrackedRemoteNew->Buttons, pDominantTrackedRemoteOld->Buttons, false, ovrButton_Trigger, UB_ATTACK);
            }

            //Duck
            if ((primaryButtonsNew & primaryButton1) !=
                (primaryButtonsOld & primaryButton1)) {

                handleTrackedControllerButton_AsToggleButton(primaryButtonsNew, primaryButtonsOld, primaryButton1, UB_DOWN);
            }

            //Jump
            if ((primaryButtonsNew & primaryButton2) != (primaryButtonsOld & primaryButton2))
            {
                handleTrackedControllerButton_AsButton(primaryButtonsNew, primaryButtonsOld, false, primaryButton2, UB_UP);
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
                        Android_SetImpulse(UB_IMPULSE15);
					}
					else
					{
					    //Next Weapon
                        Android_SetImpulse(UB_IMPULSE14);
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
            float x = (nlf * pSecondaryJoystick->x) + pFootTrackingNew->LeftJoystick.x;
            float y = (nlf * pSecondaryJoystick->y) - pFootTrackingNew->LeftJoystick.y;

            pVRClientInfo->player_moving = (fabs(x) + fabs(y)) > 0.05f;

            //Adjust to be off-hand controller oriented
            vec2_t v;
            rotateAboutOrigin(x, y, controllerYawHeading, v);

            //Move a lot slower if scope is engaged
            float vr_movement_multiplier = 127;
            remote_movementSideways = v[0] *  vr_movement_multiplier;
            remote_movementForward = v[1] *  vr_movement_multiplier;


            if (dominantGripPushed) {
                if (((offhandButtonsNew & ovrButton_Joystick) !=
                     (offhandButtonsOld & ovrButton_Joystick)) &&
                    (offhandButtonsNew & ovrButton_Joystick)) {

                    Android_SetCommand("give all");
#ifdef DEBUG
                    if(give_weapon_count == 1){
                        Android_SetCommand("give weapon_pistol");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 2){
                        Android_SetCommand("give weapon_shotgun");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 3){
                        Android_SetCommand("give weapon_shotgun");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 4){
                        Android_SetCommand("give weapon_machinegun");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 5){
                        Android_SetCommand("give weapon_chaingun");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 6){
                        Android_SetCommand("give weapon_rocketlauncher");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 7){
                        Android_SetCommand("give weapon_plasmagun");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 8){
                        Android_SetCommand("give weapon_chainsaw");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 9){
                        Android_SetCommand("give weapon_soulcube");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 10){
                        Android_SetCommand("give weapon_bfg");
                        give_weapon_count = give_weapon_count + 1;
                    }
                    else if(give_weapon_count == 11){
                        Android_SetCommand("give ammo_grenade_small");
                        give_weapon_count = 1;
                    }
#endif
                        //Recenter Body
                        Android_SetImpulse(UB_IMPULSE32);
                }

                if (((weaponButtonsNew & ovrButton_Joystick) !=
                     (weaponButtonsOld & ovrButton_Joystick)) &&
                    (weaponButtonsNew & ovrButton_Joystick)) {
                    //Toggle Torch Mode
                    Android_SetImpulse(UB_IMPULSE35);
                }
            } else {
                if (((weaponButtonsNew & ovrButton_Joystick) !=
                     (weaponButtonsOld & ovrButton_Joystick)) &&
                    (weaponButtonsNew & ovrButton_Joystick)) {

                    //Toggle Body
                    Android_SetImpulse(UB_IMPULSE34);
                }

                if (((offhandButtonsNew & ovrButton_Joystick) !=
                     (offhandButtonsOld & ovrButton_Joystick)) &&
                    (offhandButtonsNew & ovrButton_Joystick)) {

                    //Turn on Flashlight
                    Android_SetImpulse(UB_IMPULSE16);
                }
            }

            //We need to record if we have started firing primary so that releasing trigger will stop definitely firing, if user has pushed grip
            //in meantime, then it wouldn't stop the gun firing and it would get stuck
            handleTrackedControllerButton_AsButton(pOffTrackedRemoteNew->Buttons, pOffTrackedRemoteOld->Buttons, false, ovrButton_Trigger, UB_SPEED);

            int vr_turn_mode = Android_GetCVarInteger("vr_turnmode");
            float vr_turn_angle = Android_GetCVarInteger("vr_turnangle");

            //No snap turn when using mounted gun
            snapTurn = 0;
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
                    } else {
                        snapTurn = 0;
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

                    } else {
                        snapTurn = 0;
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