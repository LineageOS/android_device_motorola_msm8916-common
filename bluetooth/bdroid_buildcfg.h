/*
 * Copyright (C) 2016 The CyanogenMod Project
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

#ifndef _BDROID_BUILDCFG_H
#define _BDROID_BUILDCFG_H

#include <cutils/properties.h>
#include <string.h>

inline const char* BtmGetDefaultName()
{
	char device[PROPERTY_VALUE_MAX];
	property_get("ro.boot.device", device, "");

	if (!strcmp("surnia", device)) {
		return "Moto E";
	} else if (!strcmp("osprey", device)) {
		return "Moto G";
	} else if (!strcmp("merlin", device)) {
		return "Moto G Turbo";
	} else if (!strcmp("lux", device)) {
		return "Moto X Play";
	} else if (!strcmp("harpia", device)) {
		return "Moto G Play";
	}

	return "Motorola";
}

#define BTM_DEF_LOCAL_NAME BtmGetDefaultName()
#define BLUETOOTH_QTI_SW TRUE
#define MAX_ACL_CONNECTIONS   7
#define MAX_L2CAP_CHANNELS    16
#define BLE_VND_INCLUDED   TRUE
#define BT_CLEAN_TURN_ON_DISABLED TRUE
#endif
