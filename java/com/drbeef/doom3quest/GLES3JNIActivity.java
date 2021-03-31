
package com.drbeef.doom3quest;


import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

import com.drbeef.hapticsservice.IHapticsService;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import static android.system.Os.setenv;

@SuppressLint("SdCardPath") public class GLES3JNIActivity extends Activity implements SurfaceHolder.Callback, ServiceConnection
{
	// Load the gles3jni library right away to make sure JNI_OnLoad() gets called as the very first thing.
	static
	{
		System.loadLibrary( "doom3" );
	}

	private boolean hasHapticService = false;
	private IHapticsService hapticsService = null;

	private int permissionCount = 0;
	private static final int READ_EXTERNAL_STORAGE_PERMISSION_ID = 1;
	private static final int WRITE_EXTERNAL_STORAGE_PERMISSION_ID = 2;


	private static final String APPLICATION = "Doom3Quest";

	private String commandLineParams;

	private SurfaceView mView;
	private SurfaceHolder mSurfaceHolder;
	private long mNativeHandle;


	public void shutdown() {
		System.exit(0);
	}


	public void haptic_event(String event, int position, int flags, int intensity, float angle, float yHeight)  {

		if (hasHapticService) {
			try {
				hapticsService.hapticEvent(APPLICATION, event, position, flags, intensity, angle, yHeight);
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}
	}

	public void haptic_updateevent(String event, int intensity, float angle) {

		if (hasHapticService) {
			try {
				hapticsService.hapticUpdateEvent(APPLICATION, event, intensity, angle);
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}
	}

	public void haptic_stopevent(String event) {

		if (hasHapticService) {
			try {
				hapticsService.hapticStopEvent(APPLICATION, event);
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}
	}

	public void haptic_endframe() {

		if (hasHapticService) {
			try {
				hapticsService.hapticFrameTick();
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}
	}

	public void haptic_enable() {

		if (hasHapticService) {
			try {
				hapticsService.hapticEnable();
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}
	}

	public void haptic_disable() {

		if (hasHapticService) {
			try {
				hapticsService.hapticDisable();
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}
	}

	@Override protected void onCreate( Bundle icicle )
	{
		super.onCreate( icicle );

		mView = new SurfaceView( this );
		setContentView( mView );
		mView.getHolder().addCallback( this );

		// Force the screen to stay on, rather than letting it dim and shut off
		// while the user is watching a movie.
		getWindow().addFlags( WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON );

		// Force screen brightness to stay at maximum
		WindowManager.LayoutParams params = getWindow().getAttributes();
		params.screenBrightness = 1.0f;
		getWindow().setAttributes( params );

		checkPermissionsAndInitialize();
	}

	private void requestPermissions() {
		ActivityCompat.requestPermissions(this,
				new String[]{Manifest.permission.READ_EXTERNAL_STORAGE,
						Manifest.permission.WRITE_EXTERNAL_STORAGE},
				1);
	}

	/** Initializes the Activity only if the permission has been granted. */
	private void checkPermissionsAndInitialize() {
		// Boilerplate for checking runtime permissions in Android.
		if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
				!= PackageManager.PERMISSION_GRANTED){
			ActivityCompat.requestPermissions(this,
					new String[]{Manifest.permission.READ_EXTERNAL_STORAGE,
							Manifest.permission.WRITE_EXTERNAL_STORAGE},
					1);
		}
		else
		{
			permissionCount++;
		}

		if (permissionCount == 1) {
			// Permissions have already been granted.
			create();
		}
	}

	/** Handles the user accepting the permission. */
	@Override
	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
		if (requestCode == WRITE_EXTERNAL_STORAGE_PERMISSION_ID) {
			if (results.length > 0 && results[0] != PackageManager.PERMISSION_GRANTED) {

				finish();
				System.exit(0);
			}
		}

		checkPermissionsAndInitialize();
	}

	public void create() {

		boolean exitAfterCopy = false;

		//If this is first run on clean system, or user hasn't copied anything yet, just exit after we have copied
		if (!(new File("/sdcard/Doom3Quest/base/pak000.pk4").exists()))
		{
			exitAfterCopy = true;
		}

		copy_asset("/sdcard/Doom3Quest", "commandline.txt", false);

		//Create all required folders
		new File("/sdcard/Doom3Quest/base").mkdirs();

		copy_asset("/sdcard/Doom3Quest/base", "pak399.pk4", true);

		//Now copy our defaults for the two headsets - These are only our defaults, users shouldn't change
		//these so forcefully overwrite
		copy_asset("/sdcard/Doom3Quest/config/base", "quest1_default.cfg", true);
		copy_asset("/sdcard/Doom3Quest/config/base", "quest2_default.cfg", true);

		if (exitAfterCopy)
		{
			finish();
			System.exit(0);
		}

		//Read these from a file and pass through
		commandLineParams = new String("doom3quest");

		//See if user is trying to use command line params
		if(new File("/sdcard/Doom3Quest/commandline.txt").exists()) // should exist now!
		{
			BufferedReader br;
			try {
				br = new BufferedReader(new FileReader("/sdcard/Doom3Quest/commandline.txt"));
				String s;
				StringBuilder sb=new StringBuilder(0);
				while ((s=br.readLine())!=null)
					sb.append(s + " ");
				br.close();

				commandLineParams = new String(sb.toString());
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

		try {
			ApplicationInfo ai =  getApplicationInfo();

			setenv("USER_FILES", "/sdcard/Doom3Quest", true);
			setenv("GAMELIBDIR", getApplicationInfo().nativeLibraryDir, true);
			setenv("GAMETYPE", "16", true); // hard coded for now
		}
		catch (Exception e)
		{

		}

		//Parse the config file for these values
		long refresh = 60; // Default to 60
		float ss = -1.0F;
		long msaa = 1; // default for both HMDs
		String configFileName = "/sdcard/Doom3Quest/config/base/doom3quest.cfg";
		if(new File(configFileName).exists())
		{
			BufferedReader br;
			try {
				br = new BufferedReader(new FileReader(configFileName));
				String s;
				while ((s=br.readLine())!=null) {
					int i1 = s.indexOf("\"");
					int i2 = s.lastIndexOf("\"");
					if (i1 != -1 && i2 != -1) {
						String value = s.substring(i1+1, i2);
						if (s.contains("vr_refresh")) {
							refresh = Long.parseLong(value);
						} else if (s.contains("vr_msaa")) {
							msaa = Long.parseLong(value);
						} else if (s.contains("vr_supersampling")) {
							ss = Float.parseFloat(value);
						}
					}
				}
				br.close();
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (NumberFormatException e)
			{
				e.printStackTrace();
			}
		}

		mNativeHandle = GLES3JNILib.onCreate( this, commandLineParams, refresh, ss, msaa );
	}
	
	public void copy_asset(String path, String name, boolean force) {
		File f = new File(path + "/" + name);
		if (!f.exists() || force) {
			
			//Ensure we have an appropriate folder
			String fullname = path + "/" + name;
			String directory = fullname.substring(0, fullname.lastIndexOf("/"));
			new File(directory).mkdirs();
			_copy_asset(name, path + "/" + name);
		}
	}

	public void _copy_asset(String name_in, String name_out) {
		AssetManager assets = this.getAssets();

		try {
			InputStream in = assets.open(name_in);
			OutputStream out = new FileOutputStream(name_out);

			copy_stream(in, out);

			out.close();
			in.close();

		} catch (Exception e) {

			e.printStackTrace();
		}

	}

	public static void copy_stream(InputStream in, OutputStream out)
			throws IOException {
		byte[] buf = new byte[1024];
		while (true) {
			int count = in.read(buf);
			if (count <= 0)
				break;
			out.write(buf, 0, count);
		}
	}

	@Override protected void onStart()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onStart()" );
		super.onStart();

		// Bind to the service - Make this a config file thing
		bindService(new Intent("com.drbeef.hapticservice.HapticService_bHaptics").setPackage("com.drbeef.hapticservice"), this,
				Context.BIND_AUTO_CREATE);

		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onStart(mNativeHandle, this);
		}
	}

	@Override protected void onResume()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onResume()" );
		super.onResume();

		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onResume(mNativeHandle);
		}
	}

	@Override protected void onPause()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onPause()" );
		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onPause(mNativeHandle);
		}
		super.onPause();
	}

	@Override protected void onStop()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onStop()" );
		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onStop(mNativeHandle);
		}

		// Unbind from the service
		unbindService(this);

		super.onStop();
	}

	@Override protected void onDestroy()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onDestroy()" );

		if ( mSurfaceHolder != null )
		{
			GLES3JNILib.onSurfaceDestroyed( mNativeHandle );
		}

		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onDestroy(mNativeHandle);
		}

		super.onDestroy();
		mNativeHandle = 0;
	}

	@Override public void surfaceCreated( SurfaceHolder holder )
	{
		Log.v(APPLICATION, "GLES3JNIActivity::surfaceCreated()" );
		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onSurfaceCreated( mNativeHandle, holder.getSurface() );
			mSurfaceHolder = holder;
		}
	}

	@Override public void surfaceChanged( SurfaceHolder holder, int format, int width, int height )
	{
		Log.v(APPLICATION, "GLES3JNIActivity::surfaceChanged()" );
		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onSurfaceChanged( mNativeHandle, holder.getSurface() );
			mSurfaceHolder = holder;
		}
	}
	
	@Override public void surfaceDestroyed( SurfaceHolder holder )
	{
		Log.v(APPLICATION, "GLES3JNIActivity::surfaceDestroyed()" );
		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onSurfaceDestroyed( mNativeHandle );
			mSurfaceHolder = null;
		}
	}

	@Override
	public void onServiceConnected(ComponentName name, IBinder service) {
		hapticsService = IHapticsService.Stub.asInterface(service);
		hasHapticService = true;
	}

	@Override
	public void onServiceDisconnected(ComponentName name) {
		stopService(new Intent("com.drbeef.hapticservice.HapticService_bHaptics").setPackage("com.drbeef.hapticservice"));

		hasHapticService = false;
		hapticsService = null;
	}
}
