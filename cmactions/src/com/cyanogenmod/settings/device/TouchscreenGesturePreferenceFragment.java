/*
 * Copyright (C) 2015-2016 The CyanogenMod Project
 * Copyright (C) 2017 The LineageOS Project
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

import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.NotificationManager;
import android.os.Bundle;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.support.v7.preference.PreferenceCategory;
import android.support.v14.preference.PreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v14.preference.SwitchPreference;
import android.view.Menu;
import android.view.MenuItem;

public class TouchscreenGesturePreferenceFragment extends PreferenceFragment {
    private SwitchPreference mFlipPref;
    private NotificationManager mNotificationManager;
    private boolean mFlipClick = false;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.gesture_panel);
        final ActionBar actionBar = getActivity().getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        if (Device.isSurnia()){
            //Check if we have to hide the chop chop entry
            SwitchPreference chopChopPref = (SwitchPreference) findPreference("gesture_chop_chop");
            PreferenceCategory mCategory = (PreferenceCategory) findPreference("actions_key");
            mCategory.removePreference(chopChopPref);
        }
        mNotificationManager = (NotificationManager) getActivity().getSystemService(Context.NOTIFICATION_SERVICE);
        mFlipPref = (SwitchPreference) findPreference("gesture_flip_to_mute");
        mFlipPref.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            public boolean onPreferenceClick(Preference preference) {
                if (!mNotificationManager.isNotificationPolicyAccessGranted()) {
                    mFlipPref.setChecked(false);
                    new AlertDialog.Builder(getContext())
                        .setTitle(getString(R.string.flip_to_mute_title))
                        .setMessage(getString(R.string.dnd_access))
                        .setNegativeButton(android.R.string.cancel, null)
                        .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                mFlipClick = true;
                                startActivity(new Intent(
                                   android.provider.Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS));
                            }
                        }).show();
                }
                return true;
            }
        });

        //Users may deny DND access after giving it
        if (!mNotificationManager.isNotificationPolicyAccessGranted()) {
            mFlipPref.setChecked(false);
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mNotificationManager.isNotificationPolicyAccessGranted() && mFlipClick) {
            mFlipPref.setChecked(true);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            getActivity().onBackPressed();
            return true;
        }
        return false;
    }
}
