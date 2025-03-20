
package com.drbeef.doom3quest;

import android.app.Activity;
import android.view.Surface;

// Wrapper for native library

public class GLES3JNILib
{
	// Activity lifecycle
	public static native void onCreate( Activity obj, String commandLineParams, String devicename );
	public static native void onStart( Object obj );
	public static native void onDestroy();

	// Surface lifecycle
	public static native void onSurfaceCreated( Surface s );
	public static native void onSurfaceChanged( Surface s );
}
