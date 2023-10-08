/************************************************************************************

Filename	:	Q2VR_SurfaceView.c based on VrCubeWorld_SurfaceView.c
Content		:	This sample uses a plain Android SurfaceView and handles all
				Activity and Surface life cycle events in native code. This sample
				does not use the application framework and also does not use LibOVR.
				This sample only uses the VrApi.
Created		:	March, 2015
Authors		:	J.M.P. van Waveren / Simon Brown

Copyright	:	Copyright 2015 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>					// for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h>	// for native window JNI
#include <android/input.h>

#include "VrInput.h"
#include "VrClientInfo.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_mutex.h>


//Define all variables here that were externs in the VrCommon.h
bool Doom3Quest_initialised;
float playerYaw;
float vrFOV = 0.0f;
bool vr_moveuseoffhand;
float vr_snapturn_angle;
bool vr_secondarybuttonmappings;
bool vr_twohandedweapons;
bool shutdown;


//Let's go to the maximum!
const int CPU_LEVEL			= 4;
const int GPU_LEVEL			= 4;

//Passed in from the Java code
int NUM_MULTI_SAMPLES	= -1;
float SS_MULTIPLIER    = -1.0f;
int DISPLAY_REFRESH		= -1;

vrClientInfo vr;
vrClientInfo *pVRClientInfo;

jclass clazz;
static jobject d3questCallbackObj=0;

float radians(float deg) {
	return (deg * M_PI) / 180.0;
}

float degrees(float rad) {
	return (rad * 180.0) / M_PI;
}

char **argv;
int argc=0;

bool forceVirtualScreen = false;
bool inMenu = false;
bool inGameGuiActive = false;
bool objectiveSystemActive = false;
bool inCinematic = false;
bool loading = false;

void Doom3Quest_setUseScreenLayer(int screen)
{
	inMenu = screen & 0x1;
	inGameGuiActive = !!(screen & 0x2);
	objectiveSystemActive = !!(screen & 0x4);
	inCinematic = !!(screen & 0x8);
	loading = !!(screen & 0x10);
}

bool Doom3Quest_useScreenLayer()
{
	return inMenu || forceVirtualScreen || inCinematic || loading;
}

static void UnEscapeQuotes( char *arg )
{
	char *last = NULL;
	while( *arg ) {
		if( *arg == '"' && *last == '\\' ) {
			char *c_curr = arg;
			char *c_last = last;
			while( *c_curr ) {
				*c_last = *c_curr;
				c_last = c_curr;
				c_curr++;
			}
			*c_last = '\0';
		}
		last = arg;
		arg++;
	}
}

static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	char *lastp = NULL;
	int argc, last_argc;
	argc = last_argc = 0;
	for ( bufp = cmdline; *bufp; ) {
		while ( isspace(*bufp) ) {
			++bufp;
		}
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			while ( *bufp && ( *bufp != '"' || *lastp == '\\' ) ) {
				lastp = bufp;
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
		if( argv && last_argc != argc ) {
			UnEscapeQuotes( argv[last_argc] );
		}
		last_argc = argc;
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
}


void updateHMDOrientation()
{
    //Position
    VectorSubtract(vr.hmdposition_last, vr.hmdposition, vr.hmdposition_delta);

    //Keep this for our records
    VectorCopy(vr.hmdposition, vr.hmdposition_last);
}

void setHMDPosition( float x, float y, float z, float yaw )
{
	VectorSet(vr.hmdposition, x, y, z);
}

void setHMDOrientation( float x, float y, float z, float w )
{
    Vector4Set(vr.hmdorientation_quat, x, y, z, w);
}

void setHMDTranslation( float x, float y, float z)
{
    VectorSet(vr.hmdtranslation, x, y, z);
}


/*
========================
Doom3Quest_Vibrate
========================
*/

//0 = left, 1 = right
float vibration_channel_intensity[2][2] = {{0.0f,0.0f},{0.0f,0.0f}};

void Doom3Quest_Vibrate(int channel, float low, float high)
{
	vibration_channel_intensity[channel][0] = low;
	vibration_channel_intensity[channel][1] = high;
}

void jni_haptic_event(const char* event, int position, int flags, int intensity, float angle, float yHeight);
void jni_haptic_updateevent(const char* event, int intensity, float angle);
void jni_haptic_stopevent(const char* event);
void jni_haptic_endframe();
void jni_haptic_enable();
void jni_haptic_disable();

void Doom3Quest_HapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight )
{
    jni_haptic_event(event, position, flags, intensity, angle, yHeight);
}

void Doom3Quest_HapticUpdateEvent(const char* event, int intensity, float angle )
{
    jni_haptic_updateevent(event, intensity, angle);
}

void Doom3Quest_HapticEndFrame()
{
	jni_haptic_endframe();
}

void Doom3Quest_HapticStopEvent(const char* event)
{
	jni_haptic_stopevent(event);
}

void Doom3Quest_HapticEnable()
{
    jni_haptic_enable();
}

void Doom3Quest_HapticDisable()
{
    jni_haptic_disable();
}

void VR_Doom3Main(int argc, char** argv);

void VR_GetMove( float *joy_forward, float *joy_side, float *hmd_forward, float *hmd_side, float *up, float *yaw, float *pitch, float *roll ) {
    *joy_side = remote_movementSideways;
    *joy_forward = remote_movementForward;
    *hmd_forward = positional_movementForward;
    *hmd_side = positional_movementSideways;
    *up = remote_movementUp;
    //*yaw = vr.hmdorientation[YAW] + snapTurn;
    *yaw = snapTurn;
    //*pitch = vr.hmdorientation[PITCH];
    //*roll = vr.hmdorientation[ROLL];}
}

static bool destroyed = false;

int Doom3Quest_GetRefresh()
{
	return 60;//Doom3Quest_initialised ? vrapi_GetSystemPropertyInt(&gAppState.Java, VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE) : 60;
}


bool VR_GetVRProjection(int eye, float zNear, float zFar, float* projection)
{
	if (strstr(gAppState.OpenXRHMD, "meta") != NULL)
	{
		XrFovf fov = {};
		for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
			fov.angleLeft += gAppState.Projections[eye].fov.angleLeft / 2.0f;
			fov.angleRight += gAppState.Projections[eye].fov.angleRight / 2.0f;
			fov.angleUp += gAppState.Projections[eye].fov.angleUp / 2.0f;
			fov.angleDown += gAppState.Projections[eye].fov.angleDown / 2.0f;
		}
		XrMatrix4x4f_CreateProjectionFov(
				&(gAppState.ProjectionMatrices[eye]), GRAPHICS_OPENGL_ES,
				fov, zNear, zFar);
	}

	if (strstr(gAppState.OpenXRHMD, "pico") != NULL)
	{
		XrMatrix4x4f_CreateProjectionFov(
				&(gAppState.ProjectionMatrices[eye]), GRAPHICS_OPENGL_ES,
				gAppState.Projections[eye].fov, zNear, zFar);
	}

	memcpy(projection, gAppState.ProjectionMatrices[eye].m, 16 * sizeof(float));
	return true;
}


long shutdownCountdown;

int m_width;
int m_height;

void Doom3Quest_GetScreenRes(int *width, int *height)
{
    *width = m_width;
    *height = m_height;
}

void Android_MessageBox(const char *title, const char *text)
{
    ALOGE("%s %s", title, text);
}

//void initialize_gl4es();

void VR_Init()
{
	//Initialise all our variables
	screenYaw = 0.0f;
	remote_movementSideways = 0.0f;
	remote_movementForward = 0.0f;
	remote_movementUp = 0.0f;
	positional_movementSideways = 0.0f;
	positional_movementForward = 0.0f;
	snapTurn = 0.0f;
	vr.visible_hud = true;

	//init randomiser
	srand(time(NULL));

	shutdown = false;
}

void shutdownVR() {
    SDL_DestroyMutex(gAppState.RenderThreadFrameIndex_Mutex);
	ovrRenderer_Destroy( &gAppState.Renderer );
	ovrEgl_DestroyContext( &gAppState.Egl );
	(*java.Vm)->DetachCurrentThread( java.Vm );
}

void showLoadingIcon();
void jni_shutdown();

/* Called before SDL_main() to initialize JNI bindings in SDL library */
extern void SDL_Android_Init(JNIEnv* env, jclass cls);

//Calld on the main thread before the rendering thread is started
void DeactivateContext()
{
    eglMakeCurrent( gAppState.Egl.Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
}

//Caled by the rendering thread to take charge of the context
void ActivateContext()
{
    eglMakeCurrent( gAppState.Egl.Display, gAppState.Egl.TinySurface, gAppState.Egl.TinySurface, gAppState.Egl.Context );

    gAppState.RenderThreadTid = gettid();
}

int questType;

void * AppThreadFunction(void * parm ) {
	gAppThread = (ovrAppThread *) parm;

	java.Vm = gAppThread->JavaVm;
	(*java.Vm)->AttachCurrentThread(java.Vm, &java.Env, NULL);
	java.ActivityObject = gAppThread->ActivityObject;

	jclass cls = (*java.Env)->GetObjectClass(java.Env, java.ActivityObject);

    /* This interface could expand with ABI negotiation, callbacks, etc. */
    SDL_Android_Init(java.Env, cls);

    SDL_SetMainReady();

    pVRClientInfo = &vr;

	// Note that AttachCurrentThread will reset the thread name.
	prctl(PR_SET_NAME, (long) "OVR::Main", 0, 0, 0);

	Doom3Quest_initialised = false;

	TBXR_InitialiseOpenXR();

	TBXR_EnterVR();
	TBXR_InitRenderer();
	TBXR_InitActions();

    chdir("/sdcard/Doom3Quest");

	TBXR_WaitForSessionActive();
	
    //Should now be all set up and ready - start the Doom3 main loop
    VR_Doom3Main(argc, argv);

    //Take the context back
    ActivateContext();

	//We are done, shutdown cleanly
	shutdownVR();

	//Ask Java to shut down
    jni_shutdown();

	return NULL;
}

//All the stuff we want to do each frame
void VR_FrameSetup(int controlscheme, int switch_sticks, int refresh)
{
    ALOGV("Refresh = %i", refresh);

	if (!Doom3Quest_useScreenLayer())
	{
		screenYaw = vr.hmdorientation_temp[YAW];
	}
}


void Doom3Quest_getHMDOrientation() {

	//Update the main thread frame index in a thread safe way
	{
		SDL_LockMutex(gAppState.RenderThreadFrameIndex_Mutex);

		gAppState.MainThreadFrameIndex = gAppState.RenderThreadFrameIndex + 1;

		SDL_UnlockMutex(gAppState.RenderThreadFrameIndex_Mutex);
	}

    gAppState.DisplayTime[gAppState.MainThreadFrameIndex % MAX_TRACKING_SAMPLES] = vrapi_GetPredictedDisplayTime(gAppState.Ovr, gAppState.MainThreadFrameIndex);

    ovrTracking2 *tracking = &gAppState.Tracking[gAppState.MainThreadFrameIndex % MAX_TRACKING_SAMPLES];
	*tracking = vrapi_GetPredictedTracking2(gAppState.Ovr, gAppState.DisplayTime[gAppState.MainThreadFrameIndex % MAX_TRACKING_SAMPLES]);

	//Don't update game with tracking if we are in big screen mode
    //GB Do pass the stuff but block at my end (if big screen prompt is needed)
    const ovrQuatf quatHmd = tracking->HeadPose.Pose.Orientation;
    const ovrVector3f positionHmd = tracking->HeadPose.Pose.Position;
    //const ovrVector3f translationHmd = tracking->HeadPose.Pose.Translation;
    vec3_t rotation = {0};
    QuatToYawPitchRoll(quatHmd, rotation, vr.hmdorientation_temp);
    setHMDPosition(positionHmd.x, positionHmd.y, positionHmd.z, 0);
    //GB
    setHMDOrientation(quatHmd.x, quatHmd.y, quatHmd.z, quatHmd.w);
    //setHMDTranslation(translationHmd.x, translationHmd.y, translationHmd.z);
    //End GB
    updateHMDOrientation();
}

void VR_HandleControllerInput() {
	TBXR_UpdateControllers();

    //Call additional control schemes here
    if (controlscheme == RIGHT_HANDED_DEFAULT) {
		HandleInput_Default(controlscheme, switch_sticks,
							/*&footTrackedRemoteState_new, &footTrackedRemoteState_old,*/
							&rightTrackedRemoteState_new, &rightTrackedRemoteState_old,
							&rightRemoteTracking_new,
							&leftTrackedRemoteState_new, &leftTrackedRemoteState_old,
							&leftRemoteTracking_new,
							ovrButton_A, ovrButton_B, ovrButton_X, ovrButton_Y);
	} else {
		//Left handed
		HandleInput_Default(controlscheme, switch_sticks,
							/*&footTrackedRemoteState_new, &footTrackedRemoteState_old,*/
							&leftTrackedRemoteState_new, &leftTrackedRemoteState_old,
							&leftRemoteTracking_new,
							&rightTrackedRemoteState_new, &rightTrackedRemoteState_old,
							&rightRemoteTracking_new,
							ovrButton_X, ovrButton_Y, ovrButton_A, ovrButton_B);
	}
}

/*
================================================================================

Activity lifecycle

================================================================================
*/

jmethodID android_shutdown;
jmethodID android_haptic_event;
jmethodID android_haptic_updateevent;
jmethodID android_haptic_stopevent;
jmethodID android_haptic_endframe;
jmethodID android_haptic_enable;
jmethodID android_haptic_disable;
static JavaVM *jVM;
static jobject jniCallbackObj=0;

void jni_shutdown()
{
    ALOGV("Calling: jni_shutdown");
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }
    return (*env)->CallVoidMethod(env, jniCallbackObj, android_shutdown);
}

void jni_haptic_event(const char* event, int position, int flags, int intensity, float angle, float yHeight)
{
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    jstring StringArg1 = (*env)->NewStringUTF(env, event);

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_event, StringArg1, position, flags, intensity, angle, yHeight);
}

void jni_haptic_updateevent(const char* event, int intensity, float angle)
{
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    jstring StringArg1 = (*env)->NewStringUTF(env, event);

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_updateevent, StringArg1, intensity, angle);
}

void jni_haptic_stopevent(const char* event)
{
    ALOGV("Calling: jni_haptic_stopevent");
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    jstring StringArg1 = (*env)->NewStringUTF(env, event);

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_stopevent, StringArg1);
}

void jni_haptic_endframe()
{
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_endframe);
}

void jni_haptic_enable()
{
    ALOGV("Calling: jni_haptic_enable");
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_enable);
}

void jni_haptic_disable()
{
    ALOGV("Calling: jni_haptic_disable");
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_disable);
}

JNIEXPORT jint JNICALL SDL_JNI_OnLoad(JavaVM* vm, void* reserved);

int JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
    jVM = vm;
	if((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
	{
		ALOGE("Failed JNI_OnLoad");
		return -1;
	}

	return SDL_JNI_OnLoad(vm, reserved);
}

JNIEXPORT jlong JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onCreate( JNIEnv * env, jclass activityClass, jobject activity,
																	   jstring commandLineParams, jlong refresh, jfloat ss, jlong msaa)
{
	ALOGV( "    GLES3JNILib::onCreate()" );

	jboolean iscopy;
	const char *arg = (*env)->GetStringUTFChars(env, commandLineParams, &iscopy);


	char *cmdLine = NULL;
	if (arg && strlen(arg))
	{
		cmdLine = strdup(arg);
	}

	(*env)->ReleaseStringUTFChars(env, commandLineParams, arg);

	ALOGV("Command line %s", cmdLine);
	argv = malloc(sizeof(char*) * 255);
	argc = ParseCommandLine(strdup(cmdLine), argv);

	if (ss != -1.0f)
	{
		SS_MULTIPLIER = ss;
	}

	if (msaa != -1)
	{
		NUM_MULTI_SAMPLES = msaa;
	}

	if (refresh != -1)
	{
		//unused at the moment
		DISPLAY_REFRESH = refresh;
	}


	ovrAppThread * appThread = (ovrAppThread *) malloc( sizeof( ovrAppThread ) );
	ovrAppThread_Create( appThread, env, activity, activityClass );

	ovrMessageQueue_Enable( &appThread->MessageQueue, true );
	ovrMessage message;
	ovrMessage_Init( &message, MESSAGE_ON_CREATE, MQ_WAIT_PROCESSED );
	ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );

	return (jlong)((size_t)appThread);
}


JNIEXPORT void JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onStart( JNIEnv * env, jobject obj, jlong handle, jobject obj1)
{
	ALOGV( "    GLES3JNILib::onStart()" );

    jniCallbackObj = (jobject)(*env)->NewGlobalRef(env, obj1);
    jclass callbackClass = (*env)->GetObjectClass(env, jniCallbackObj);

    android_shutdown = (*env)->GetMethodID(env,callbackClass,"shutdown","()V");
	android_haptic_event = (*env)->GetMethodID(env, callbackClass, "haptic_event", "(Ljava/lang/String;IIIFF)V");
	android_haptic_updateevent = (*env)->GetMethodID(env, callbackClass, "haptic_updateevent", "(Ljava/lang/String;IF)V");
	android_haptic_stopevent = (*env)->GetMethodID(env, callbackClass, "haptic_stopevent", "(Ljava/lang/String;)V");
	android_haptic_endframe = (*env)->GetMethodID(env, callbackClass, "haptic_endframe", "()V");
    android_haptic_enable = (*env)->GetMethodID(env, callbackClass, "haptic_enable", "()V");
    android_haptic_disable = (*env)->GetMethodID(env, callbackClass, "haptic_disable", "()V");

	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	ovrMessage message;
	ovrMessage_Init( &message, MESSAGE_ON_START, MQ_WAIT_PROCESSED );
	ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onResume( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onResume()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	ovrMessage message;
	ovrMessage_Init( &message, MESSAGE_ON_RESUME, MQ_WAIT_PROCESSED );
	ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onPause( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onPause()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	ovrMessage message;
	ovrMessage_Init( &message, MESSAGE_ON_PAUSE, MQ_WAIT_PROCESSED );
	ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onStop( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onStop()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	ovrMessage message;
	ovrMessage_Init( &message, MESSAGE_ON_STOP, MQ_WAIT_PROCESSED );
	ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onDestroy( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onDestroy()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	ovrMessage message;
	ovrMessage_Init( &message, MESSAGE_ON_DESTROY, MQ_WAIT_PROCESSED );
	ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	ovrMessageQueue_Enable( &appThread->MessageQueue, false );

	ovrAppThread_Destroy( appThread, env );
	free( appThread );
}

/*
================================================================================

Surface lifecycle

================================================================================
*/

JNIEXPORT void JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onSurfaceCreated( JNIEnv * env, jobject obj, jlong handle, jobject surface )
{
	ALOGV( "    GLES3JNILib::onSurfaceCreated()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);

	ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
	if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
	{
		// An app that is relaunched after pressing the home button gets an initial surface with
		// the wrong orientation even though android:screenOrientation="landscape" is set in the
		// manifest. The choreographer callback will also never be called for this surface because
		// the surface is immediately replaced with a new surface with the correct orientation.
		ALOGE( "        Surface not in landscape mode!" );
	}

	ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
	appThread->NativeWindow = newNativeWindow;
	ovrMessage message;
	ovrMessage_Init( &message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED );
	ovrMessage_SetPointerParm( &message, 0, appThread->NativeWindow );
	ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onSurfaceChanged( JNIEnv * env, jobject obj, jlong handle, jobject surface )
{
	ALOGV( "    GLES3JNILib::onSurfaceChanged()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);

	ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
	if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
	{
		// An app that is relaunched after pressing the home button gets an initial surface with
		// the wrong orientation even though android:screenOrientation="landscape" is set in the
		// manifest. The choreographer callback will also never be called for this surface because
		// the surface is immediately replaced with a new surface with the correct orientation.
		ALOGE( "        Surface not in landscape mode!" );
	}

	if ( newNativeWindow != appThread->NativeWindow )
	{
		if ( appThread->NativeWindow != NULL )
		{
			ovrMessage message;
			ovrMessage_Init( &message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
			ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
			ALOGV( "        ANativeWindow_release( NativeWindow )" );
			ANativeWindow_release( appThread->NativeWindow );
			appThread->NativeWindow = NULL;
		}
		if ( newNativeWindow != NULL )
		{
			ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
			appThread->NativeWindow = newNativeWindow;
			ovrMessage message;
			ovrMessage_Init( &message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED );
			ovrMessage_SetPointerParm( &message, 0, appThread->NativeWindow );
			ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
		}
	}
	else if ( newNativeWindow != NULL )
	{
		ANativeWindow_release( newNativeWindow );
	}
}

JNIEXPORT void JNICALL Java_com_drbeef_doom3quest_GLES3JNILib_onSurfaceDestroyed( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onSurfaceDestroyed()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	ovrMessage message;
	ovrMessage_Init( &message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
	ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	ALOGV( "        ANativeWindow_release( NativeWindow )" );
	ANativeWindow_release( appThread->NativeWindow );
	appThread->NativeWindow = NULL;
}

