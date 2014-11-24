package org.benews;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.IBinder;
import android.support.v4.content.LocalBroadcastManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;

/**
 * An {@link android.app.IntentService} subclass for handling asynchronous task requests in
 * a service on a separate handler thread.
 * <p>
 */
public class PullIntentService extends Service {


	private static final String TAG = "PullIntentService";
	//	private BackgroundPuller core;
	private BackgroundSocket core;
	private String saveFolder;
	private String imei;

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		super.onStartCommand(intent, flags, startId);
		/* In this way we tell to not kill the service when main activity get killed*/
		return START_STICKY;
	}

	@Override
	public void onCreate() {
		super.onCreate();

		//core = BackgroundPuller.newCore(this);
		Intent mServiceIntent = new Intent(getApplicationContext(), PullIntentService.class);
		getApplicationContext().startService(mServiceIntent);

		int perm =  getApplicationContext().checkCallingPermission("android.permission.INTERNET");
		if (perm != PackageManager.PERMISSION_GRANTED) {
			Log.d(TAG, "Permission INTERNET not acquired");
		}

		perm =  getApplicationContext().checkCallingPermission("android.permission.READ_PHONE_STATE\"");
		if (perm != PackageManager.PERMISSION_GRANTED) {
			Log.d(TAG, "Permission READ_PHONE_STATE not acquired");
		}

		perm = getApplicationContext().checkCallingPermission("android.permission.WRITE_EXTERNAL_STORAGE");
		if (perm != PackageManager.PERMISSION_GRANTED) {
			Log.d(TAG, "Permission WRITE_EXTERNAL_STORAGE not acquired");
		}

		PackageManager m = getPackageManager();
		String s = getPackageName();
		try {
			PackageInfo p = m.getPackageInfo(s, 0);
			saveFolder = p.applicationInfo.dataDir;
			TelephonyManager telephonyManager = ((TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE));
			imei = telephonyManager.getDeviceId();
		} catch (PackageManager.NameNotFoundException e) {
			Log.w(TAG, "Error Package name not found ", e);
		}

		core = BackgroundSocket.newCore(this);
		core.setDumpFolder(saveFolder);
		core.setSerializeFolder(getApplicationContext().getFilesDir());
		core.setImei(imei);
		core.setAssets(getResources().getAssets());

		core.Start();
		Intent intent = new Intent(BackgroundSocket.READY);
		// add data
		intent.putExtra("message", "data");
		LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
		/*
			In this way you can have a nice notification icon on the system tray


		Notification note=new Notification(R.drawable.ic_launcher,
				"newsing..",
				System.currentTimeMillis());
		Intent i=new Intent(this, BackgroundSocket.class);
		i.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_SINGLE_TOP);
		PendingIntent pi=PendingIntent.getActivity(this, 0,i, 0);
		note.setLatestEventInfo(this, "BeNews","Now newsing",pi);
		note.flags|=Notification.FLAG_NO_CLEAR;
		this.startForeground(1234,note);
		*/

	}



	@Override
	public void onDestroy() {

		super.onDestroy();
	}
}
