
package com.drbeef.doom3quest;

import android.app.Activity;
import android.view.Surface;

// Wrapper for native library

public class GLES3JNILib
{
	// Activity lifecycle
	public static native long onCreate( Activity obj, String commandLineParams, long refresh, float ss, long msaa );
	public static native void onStart( long handle, Object obj );
	public static native void onResume( long handle );
	public static native void onPause( long handle );
	public static native void onStop( long handle );
	public static native void onDestroy( long handle );

	// Surface lifecycle
	public static native void onSurfaceCreated( long handle, Surface s );
	public static native void onSurfaceChanged( long handle, Surface s );
	public static native void onSurfaceDestroyed( long handle );
}
