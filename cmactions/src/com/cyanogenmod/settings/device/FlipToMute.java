/*
 * Copyright (c) 2016 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.cyanogenmod.settings.device;

import android.app.NotificationManager;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.util.Log;

public class FlipToMute implements SensorEventListener, UpdatedStateNotifier {
    private static final String TAG = "CMActions-FlipToMute";

    private final NotificationManager mNotificationManager;
    private final CMActionsSettings mCMActionsSettings;
    private final SensorHelper mSensorHelper;
    private final Sensor mFlatDown;
    private final Sensor mStow;

    private boolean mIsStowed;
    private boolean mLastStowed;
    private boolean mLastFlatDown;
    //private int previousFilter;
    private boolean mIsEnabled;
    private int moveTimes = 0;

    public FlipToMute(CMActionsSettings cmActionsSettings, Context context,
                SensorHelper sensorHelper) {
        mNotificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        mCMActionsSettings = cmActionsSettings;
        mSensorHelper = sensorHelper;
        mFlatDown = sensorHelper.getFlatDownSensor();
        mStow = sensorHelper.getStowSensor();
    }

    @Override
    public void updateState() {
        if (mCMActionsSettings.isFlipToMuteEnabled() && !mIsEnabled) {
            Log.d(TAG, "Enabling");
            mSensorHelper.registerListener(mFlatDown, this);
            mSensorHelper.registerListener(mStow, mStowListener);
            mIsEnabled = true;
        } else if (!mCMActionsSettings.isFlipToMuteEnabled() && mIsEnabled) {
            Log.d(TAG, "Disabling");
            mSensorHelper.unregisterListener(this);
            mSensorHelper.unregisterListener(mStowListener);
            mIsEnabled = false;
        }
    }

    @Override
    public synchronized void onSensorChanged(SensorEvent event) {
        boolean mIsFlatDown = (event.values[0] != 0);

        Log.d(TAG, "event: " + mIsFlatDown + " mLastFlatDown=" + mLastFlatDown + " mIsStowed=" +
            mIsStowed + " mLastStowed=" + mLastStowed);

        boolean in_pocket = false;
        if (mLastStowed && mIsStowed) {
            moveTimes++;
            if (moveTimes > 2) {
                in_pocket = true;
                moveTimes = 0;
                Log.d(TAG, "In pocket");
            }
        }

        /* Can't do this correctly due to lack of APIs (can't restore countdown timer for DND)
        if (!mLastFlatDown && !mIsStowed) {
            previousFilter = mNotificationManager.getCurrentInterruptionFilter();
        }
        */

        if (!mLastFlatDown && mIsFlatDown && mIsStowed && !in_pocket) {
            mNotificationManager.setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_NONE);
            Log.d(TAG, "Interrupt filter: Allow none");
        } else if ((mLastFlatDown && !mIsFlatDown && mLastStowed) || in_pocket) {
            //mNotificationManager.setInterruptionFilter(previousFilter);
            mNotificationManager.setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            Log.d(TAG, "Interrupt filter: Allow all");
        }
        mLastStowed = mIsStowed;
        mLastFlatDown = mIsFlatDown;
    }

    @Override
    public void onAccuracyChanged(Sensor mSensor, int accuracy) {
    }

    private SensorEventListener mStowListener = new SensorEventListener() {
        @Override
        public synchronized void onSensorChanged(SensorEvent event) {
            mIsStowed = (event.values[0] != 0);
        }

        @Override
        public void onAccuracyChanged(Sensor mSensor, int accuracy) {
        }
    };
}
