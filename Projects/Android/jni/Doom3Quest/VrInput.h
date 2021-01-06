
#if !defined(vrinput_h)
#define vrinput_h

#include "VrCommon.h"

//New control scheme definitions to be defined L1VR_SurfaceView.c enumeration
enum control_scheme;

#define STABILISATION_DISTANCE   0.5
#define FLASHLIGHT_HOLSTER_DISTANCE   0.15
#define VELOCITY_TRIGGER        1.6

ovrInputStateTrackedRemote leftTrackedRemoteState_old;
ovrInputStateTrackedRemote leftTrackedRemoteState_new;
ovrTracking leftRemoteTracking_new;

ovrInputStateTrackedRemote rightTrackedRemoteState_old;
ovrInputStateTrackedRemote rightTrackedRemoteState_new;
ovrTracking rightRemoteTracking_new;

ovrDeviceID controllerIDs[2];

float remote_movementSideways;
float remote_movementForward;
float remote_movementUp;
float positional_movementSideways;
float positional_movementForward;
float snapTurn;

void sendButtonAction(const char* action, long buttonDown);
void sendButtonActionSimple(const char* action);

void acquireTrackedRemotesData(ovrMobile *Ovr, double displayTime);

void HandleInput_Default( int controlscheme, int switchsticks, ovrInputStateTrackedRemote *pDominantTrackedRemoteNew, ovrInputStateTrackedRemote *pDominantTrackedRemoteOld, ovrTracking* pDominantTracking,
                          ovrInputStateTrackedRemote *pOffTrackedRemoteNew, ovrInputStateTrackedRemote *pOffTrackedRemoteOld, ovrTracking* pOffTracking,
                          int domButton1, int domButton2, int offButton1, int offButton2 );



void updateScopeAngles();

#endif //vrinput_h