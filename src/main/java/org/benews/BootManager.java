package org.benews;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * Created by zeno on 15/10/14.
 */
public class BootManager extends BroadcastReceiver {
	@Override
	public void onReceive(Context context, Intent intent) {

		// le due righe seguenti potrebbero diventare:
		final Intent serviceIntent = new Intent(context, PullIntentService.class);
		context.startService(serviceIntent);
	}
}
