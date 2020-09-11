/************************************************************************************

Filename	:	VrInputRight.c 
Content		:	Handles common controller input functionality
Created		:	September 2019
Authors		:	Simon Brown

*************************************************************************************/

#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <VrApi_SystemUtils.h>
#include <VrApi_Input.h>
#include <VrApi_Types.h>

#include "VrInput.h"

void Sys_AddMouseMoveEvent(int dx, int dy);
void Sys_AddMouseButtonEvent(int button, bool pressed);
void Sys_AddKeyEvent(int key, bool pressed);

void Android_ButtonChange(int key, int state);
int Android_GetButton(int key);

void handleTrackedControllerButton_AsButton(ovrInputStateTrackedRemote * trackedRemoteState, ovrInputStateTrackedRemote * prevTrackedRemoteState, bool mouse, uint32_t button, int key)
{
    if ((trackedRemoteState->Buttons & button) != (prevTrackedRemoteState->Buttons & button))
    {
        if (mouse)
        {
            Sys_AddMouseButtonEvent(key, (trackedRemoteState->Buttons & button) != 0);
        }
        else
        {
            Android_ButtonChange(key, ((trackedRemoteState->Buttons & button) != 0) ? 1 : 0);
        }
    }
}

void handleTrackedControllerButton_AsKey(ovrInputStateTrackedRemote * trackedRemoteState, ovrInputStateTrackedRemote * prevTrackedRemoteState, uint32_t button, int key)
{
    if ((trackedRemoteState->Buttons & button) != (prevTrackedRemoteState->Buttons & button))
    {
        Sys_AddKeyEvent(key, (trackedRemoteState->Buttons & button) != 0);
    }
}

void handleTrackedControllerButton_AsToggleButton(ovrInputStateTrackedRemote * trackedRemoteState, ovrInputStateTrackedRemote * prevTrackedRemoteState, uint32_t button, int key)
{
    if ((trackedRemoteState->Buttons & button) != (prevTrackedRemoteState->Buttons & button))
    {
        Android_ButtonChange(key, Android_GetButton(key) ? 0 : 1);
    }
}

void sendButtonAction(const char* action, long buttonDown) {}
void sendButtonActionSimple(const char* action) {}

void rotateAboutOrigin(float x, float y, float rotation, vec2_t out)
{
    out[0] = cosf(DEG2RAD(-rotation)) * x  +  sinf(DEG2RAD(-rotation)) * y;
    out[1] = cosf(DEG2RAD(-rotation)) * y  -  sinf(DEG2RAD(-rotation)) * x;
}

float length(float x, float y)
{
    return sqrtf(powf(x, 2.0f) + powf(y, 2.0f));
}

#define NLF_DEADZONE 0.1
#define NLF_POWER 2.2

float nonLinearFilter(float in)
{
    float val = 0.0f;
    if (in > NLF_DEADZONE)
    {
        val = in;
        val -= NLF_DEADZONE;
        val /= (1.0f - NLF_DEADZONE);
        val = powf(val, NLF_POWER);
    }
    else if (in < -NLF_DEADZONE)
    {
        val = in;
        val += NLF_DEADZONE;
        val /= (1.0f - NLF_DEADZONE);
        val = -powf(fabsf(val), NLF_POWER);
    }

    return val;
}

bool between(float min, float val, float max)
{
    return (min < val) && (val < max);
}

void acquireTrackedRemotesData(ovrMobile *Ovr, double displayTime) {//The amount of yaw changed by controller
    for ( int i = 0; ; i++ ) {
        ovrInputCapabilityHeader cap;
        ovrResult result = vrapi_EnumerateInputDevices(Ovr, i, &cap);
        if (result < 0) {
            break;
        }

        if (cap.Type == ovrControllerType_TrackedRemote) {
            ovrTracking remoteTracking;
            ovrInputStateTrackedRemote trackedRemoteState;
            trackedRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
            result = vrapi_GetCurrentInputState(Ovr, cap.DeviceID, &trackedRemoteState.Header);

            if (result == ovrSuccess) {
                ovrInputTrackedRemoteCapabilities remoteCapabilities;
                remoteCapabilities.Header = cap;
                result = vrapi_GetInputDeviceCapabilities(Ovr, &remoteCapabilities.Header);

                result = vrapi_GetInputTrackingState(Ovr, cap.DeviceID, displayTime,
                                                     &remoteTracking);

                if (remoteCapabilities.ControllerCapabilities & ovrControllerCaps_RightHand) {
                    rightTrackedRemoteState_new = trackedRemoteState;
                    rightRemoteTracking_new = remoteTracking;
                    controllerIDs[1] = cap.DeviceID;
                } else{
                    leftTrackedRemoteState_new = trackedRemoteState;
                    leftRemoteTracking_new = remoteTracking;
                    controllerIDs[0] = cap.DeviceID;
                }
            }
        }
    }
}


//YAW:  Left increase, Right decrease
void updateScopeAngles()
{
    //Bit of a hack, but use weapon orientation / position for view when scope is engaged
    static vec3_t currentScopeAngles;
    static vec3_t lastScopeAngles;
    if (false)//pVRClientInfo->scopeengaged)
    {
        //Clear weapon offset
        VectorSet(pVRClientInfo->calculated_weaponoffset, 0, 0, 0);

        VectorSet(currentScopeAngles, pVRClientInfo->weaponangles[PITCH], pVRClientInfo->weaponangles[YAW], pVRClientInfo->hmdorientation[ROLL]);

        //Set "view" Angles
        VectorCopy(currentScopeAngles, pVRClientInfo->hmdorientation);

        //Orientation
        VectorSubtract(lastScopeAngles, currentScopeAngles, pVRClientInfo->hmdorientation_delta);

        //Keep this for our records
        VectorCopy(currentScopeAngles, lastScopeAngles);
    } else {
        VectorSet(currentScopeAngles, pVRClientInfo->weaponangles[PITCH], pVRClientInfo->weaponangles[YAW], pVRClientInfo->hmdorientation[ROLL]);
        VectorCopy(currentScopeAngles, lastScopeAngles);
    }
}


#ifndef max
#define max( x, y ) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
#define min( x, y ) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )
#endif


float clamp(float _min, float _val, float _max)
{
    return max(min(_val, _max), _min);
}


void interactWithTouchScreen(bool reset, ovrInputStateTrackedRemote *newState, ovrInputStateTrackedRemote *oldState) {
    static int cursorX = 0;
    static int cursorY = 0;

    cursorX = (float)(pVRClientInfo->weaponangles_delta[YAW] * 15.0f);
    cursorY = (float)(-pVRClientInfo->weaponangles_delta[PITCH] * 15.0f);

    Sys_AddMouseMoveEvent(cursorX, cursorY);
}