#if !defined(vrcommon_h)
#define vrcommon_h

#include <VrApi_Input.h>

#include <android/log.h>

#include "mathlib.h"
#include "VrClientInfo.h"

#define LOG_TAG "D3QUESTVR"

#ifndef NDEBUG
#define DEBUG 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__ )

#if DEBUG
#define ALOGV(...) __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__ )
#else
#define ALOGV(...)
#endif

float playerHeight;
float playerYaw;

bool showingScreenLayer;

ovrTracking2 tracking;

float radians(float deg);

float degrees(float rad);

bool isMultiplayer();

double GetTimeInMilliSeconds();

float length(float x, float y);

float nonLinearFilter(float in);

bool between(float min, float val, float max);

void rotateAboutOrigin(float v1, float v2, float rotation, vec2_t out);

void QuatToYawPitchRoll(ovrQuatf q, vec3_t rotation, vec3_t out);

void handleTrackedControllerButton(ovrInputStateTrackedRemote *trackedRemoteState,
                                   ovrInputStateTrackedRemote *prevTrackedRemoteState,
                                   bool mouse, uint32_t button, int key);

void interactWithTouchScreen(bool reset, ovrInputStateTrackedRemote *newState,
                             ovrInputStateTrackedRemote *oldState);


//Called from engine code
bool D3Quest_useScreenLayer();

void D3Quest_GetScreenRes(int *width, int *height);

void D3Quest_Vibrate(int duration, int channel, float intensity);

bool D3Quest_processMessageQueue();

void D3Quest_FrameSetup();

void D3Quest_setUseScreenLayer(bool use);

void D3Quest_processHaptics();

void D3Quest_getHMDOrientation();

void D3Quest_getTrackedRemotesOrientation(int vr_control_scheme);

void D3Quest_ResyncClientYawWithGameYaw();

void D3Quest_prepareEyeBuffer(int eye);

void D3Quest_finishEyeBuffer(int eye);

void D3Quest_submitFrame();

void GPUDropSync();

void GPUWaitSync();

#ifdef __cplusplus
}
#endif

#endif //vrcommon_h