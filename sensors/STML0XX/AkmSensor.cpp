/*
 * Copyright (C) 2008 The Android Open Source Project
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

/*
 * Copyright (C) 2015 Motorola Mobility LLC
 */

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <dlfcn.h>

#include <cutils/log.h>

#include "AkmSensor.h"
#include <utils/SystemClock.h>

#define AKMD_DEFAULT_INTERVAL	200000000
#define AKM_SYSFS_PATH	"/sys/class/compass/akm09912/"

/*****************************************************************************/

AkmSensor AkmSensor::self;

AkmSensor::AkmSensor()
: SensorBase(NULL, "compass", NULL),
	mPendingMask(0),
	mInputReader(32)
{
	for (int i=0; i<numSensors; i++) {
		mEnabled[i] = 0;
		mDelay[i] = -1;
	}
	memset(mPendingEvents, 0, sizeof(mPendingEvents));
	
	mPendingEvents[Accelerometer].version = sizeof(sensors_event_t);
	mPendingEvents[Accelerometer].sensor = ID_A;
	mPendingEvents[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
	mPendingEvents[Accelerometer].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

	mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
	mPendingEvents[MagneticField].sensor = ID_M;
	mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
	mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_UNRELIABLE;

	mPendingEvents[UncalMagneticField].version = sizeof(sensors_event_t);
	mPendingEvents[UncalMagneticField].sensor = ID_UM;
	mPendingEvents[UncalMagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
	mPendingEvents[UncalMagneticField].magnetic.status = SENSOR_STATUS_UNRELIABLE;

	mPendingEvents[Orientation  ].version = sizeof(sensors_event_t);
	mPendingEvents[Orientation  ].sensor = ID_OR;
	mPendingEvents[Orientation  ].type = SENSOR_TYPE_ORIENTATION;
	mPendingEvents[Orientation  ].orientation.status = SENSOR_STATUS_UNRELIABLE;

	mPendingEvents[RotationVector].version = sizeof(sensors_event_t);
	mPendingEvents[RotationVector].sensor = ID_RV;
	mPendingEvents[RotationVector].type = SENSOR_TYPE_ROTATION_VECTOR;

	if (data_fd >= 0) {
		strcpy(input_sysfs_path, AKM_SYSFS_PATH);
		input_sysfs_path_len = strlen(input_sysfs_path);
	} else {
		input_sysfs_path[0] = '\0';
		input_sysfs_path_len = 0;
	}
}

AkmSensor::~AkmSensor()
{
}

AkmSensor *AkmSensor::getInstance()
{
	return &self;
}

int AkmSensor::writeMagDelay(int64_t ns)
{
	int err = 0;
	char buffer[32];
	int bytes;

	strcpy(&input_sysfs_path[input_sysfs_path_len], "delay_mag");

	bytes = sprintf(buffer, "%lld", (long long int)ns);
	err = write_sys_attribute(input_sysfs_path, buffer, bytes);
	if (err == 0) {
		ALOGD("AkmSensor::setDelay %s to %f ms.",
			&input_sysfs_path[input_sysfs_path_len], ns/1000000.0f);
	} else {
		ALOGE("AkmSensor: %s write failed.",
			&input_sysfs_path[input_sysfs_path_len]);
	}

	return err;
}

int AkmSensor::setEnable(int32_t handle, int enabled)
{
	int id = handle2id(handle);
	int err = 0;
	char buffer[2];
	int64_t ns;

	ALOGD("AkmSensor::setEnable handle=%d, enabled=%d", handle, enabled);

	buffer[0] = '\0';
	buffer[1] = '\0';

	if (enabled) {
		mEnabled[id] = 1;
	} else {
		mEnabled[id] = 0;
	}

	switch (id) {
	case MagneticField:
	case UncalMagneticField:
		if (mEnabled[MagneticField] || mEnabled[UncalMagneticField]) {
			buffer[0] = '1';
		} else {
			buffer[0] = '0';
		}
		if (mEnabled[MagneticField]) {
			ns = mDelay[MagneticField];
			if (mEnabled[UncalMagneticField] && (mDelay[UncalMagneticField] < ns)) {
				ns = mDelay[UncalMagneticField];
			}
			writeMagDelay(ns);
		 } else if (mEnabled[UncalMagneticField]) {
			ns = mDelay[UncalMagneticField];
			writeMagDelay(ns);
		}
		strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_mag");
		break;
	case Accelerometer:
		if (mEnabled[Accelerometer]) {
			buffer[0] = '1';
		} else {
			buffer[0] = '0';
		}
		strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_acc");
		break;
	case Orientation:
	case RotationVector:
		if (mEnabled[Orientation] || mEnabled[RotationVector]) {
			buffer[0] = '1';
		} else {
			buffer[0] = '0';
		}
		strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_fusion");
		break;
	default:
		ALOGE("AkmSensor::setEnable unknown handle (%d)", handle);
		return -EINVAL;
	}

	if (buffer[0] != '\0') {
		err = write_sys_attribute(input_sysfs_path, buffer, 1);
		if (err != 0) {
			ALOGE("AkmSensor: %s write failed.",
				&input_sysfs_path[input_sysfs_path_len]);
			return err;
		}
		ALOGD("AkmSensor::setEnable write %s to %s",
			buffer,
			&input_sysfs_path[input_sysfs_path_len]);
	}

	return err;
}

int AkmSensor::setDelay(int32_t handle, int64_t ns)
{
	int id = handle2id(handle);
	int err = 0;
	char buffer[32];
	int bytes;

	ALOGD("AkmSensor::setDelay handle=%d, ns=%lld", handle, ns);

	if (ns < -1 || 2147483647 < ns) {
		ALOGE("AkmSensor::setDelay invalid delay (%lld)", ns);
		return -EINVAL;
	}

	switch (id) {
	case Accelerometer:
		strcpy(&input_sysfs_path[input_sysfs_path_len], "delay_acc");
		break;
	case MagneticField:
	case UncalMagneticField:
		strcpy(&input_sysfs_path[input_sysfs_path_len], "delay_mag");
		break;
	case Orientation:
	case RotationVector:
		strcpy(&input_sysfs_path[input_sysfs_path_len], "delay_fusion");
		break;
	default:
		ALOGE("AkmSensor::setDelay unknown handle (%d)", handle);
		return -EINVAL;
	}

	mDelay[id] = ns;

	if (id == MagneticField) {
		if ((mEnabled[UncalMagneticField]) && (mDelay[UncalMagneticField] < ns))
			ns = mDelay[UncalMagneticField];
	}
	if (id == UncalMagneticField) {
		if ((mEnabled[MagneticField]) && (mDelay[MagneticField] < ns))
			ns = mDelay[MagneticField];
	}

	bytes = sprintf(buffer, "%lld", (long long int)ns);
	err = write_sys_attribute(input_sysfs_path, buffer, bytes);
	if (err == 0) {
		ALOGD("AkmSensor::setDelay %s to %f ms.",
			&input_sysfs_path[input_sysfs_path_len], ns/1000000.0f);
	} else {
		ALOGE("AkmSensor: %s write failed.",
			&input_sysfs_path[input_sysfs_path_len]);
	}

	return err;
}

int AkmSensor::readEvents(sensors_event_t* data, int count)
{
	if (count < 1)
		return -EINVAL;

	ssize_t n = mInputReader.fill(data_fd);
	if (n < 0)
		return n;

	int numEventReceived = 0;
	input_event const* event;

	while (count && mInputReader.readEvent(&event)) {
		int type = event->type;
		if (type == EV_ABS) {
			processEvent(event->code, event->value);
			mInputReader.next();
		} else if (type == EV_SYN) {
			int64_t time = android::elapsedRealtimeNano();
			for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {
				if (mPendingMask & (1<<j)) {
					mPendingMask &= ~(1<<j);
					mPendingEvents[j].timestamp = time;
					//ALOGD("%d data=%8.5f,%8.5f,%8.5f", j,
						//mPendingEvents[j].data[0],
						//mPendingEvents[j].data[1],
						//mPendingEvents[j].data[2]);
					if (mEnabled[j]) {
						*data++ = mPendingEvents[j];
						count--;
						numEventReceived++;
					}
				}
			}
			if (!mPendingMask) {
				mInputReader.next();
			}
		} else {
			ALOGE("AkmSensor: unknown event (type=%d, code=%d)",
				type, event->code);
			mInputReader.next();
		}
	}
	return numEventReceived;
}

int AkmSensor::setAccel(sensors_event_t* data)
{
	int err;
	int16_t acc[3];

	/* Input data is already formated to Android definition. */
	acc[0] = (int16_t)(data->acceleration.x / GRAVITY_EARTH * AKSC_LSG);
	acc[1] = (int16_t)(data->acceleration.y / GRAVITY_EARTH * AKSC_LSG);
	acc[2] = (int16_t)(data->acceleration.z / GRAVITY_EARTH * AKSC_LSG);

	strcpy(&input_sysfs_path[input_sysfs_path_len], "accel");
	err = write_sys_attribute(input_sysfs_path, (char*)acc, 6);
	if (err < 0) {
		ALOGD("AkmSensor: %s write failed.",
				&input_sysfs_path[input_sysfs_path_len]);
	}
	return err;
}

int AkmSensor::handle2id(int32_t handle)
{
	switch (handle) {
	case ID_A:
		return Accelerometer;
	case ID_M:
		return MagneticField;
	case ID_UM:
		return UncalMagneticField;
	case ID_OR:
		return Orientation;
	case ID_RV:
		return RotationVector;
	default:
		ALOGE("AkmSensor: unknown handle (%d)", handle);
		return -EINVAL;
	}
}

void AkmSensor::processEvent(int code, int value)
{
	/* Decode encoded value */
	value >>= 1;

	switch (code) {
/* Accel data is only reported through the Hub sensor
	case EVENT_TYPE_ACCEL_X:
		mPendingMask |= 1<<Accelerometer;
		mPendingEvents[Accelerometer].acceleration.x = value * CONVERT_A_AKM;
		break;
	case EVENT_TYPE_ACCEL_Y:
		mPendingMask |= 1<<Accelerometer;
		mPendingEvents[Accelerometer].acceleration.y = value * CONVERT_A_AKM;
		break;
	case EVENT_TYPE_ACCEL_Z:
		mPendingMask |= 1<<Accelerometer;
		mPendingEvents[Accelerometer].acceleration.z = value * CONVERT_A_AKM;
		break;
	case EVENT_TYPE_ACCEL_STATUS:
		mPendingMask |= 1<<Accelerometer;
		mPendingEvents[Accelerometer].acceleration.status = value;
	break;
*/

	case EVENT_TYPE_MAGV_X:
		mPendingMask |= 1<<MagneticField;
		mPendingEvents[MagneticField].magnetic.x = value * CONVERT_M;
		break;
	case EVENT_TYPE_MAGV_Y:
		mPendingMask |= 1<<MagneticField;
		mPendingEvents[MagneticField].magnetic.y = value * CONVERT_M;
		break;
	case EVENT_TYPE_MAGV_Z:
		mPendingMask |= 1<<MagneticField;
		mPendingEvents[MagneticField].magnetic.z = value * CONVERT_M;
		break;
	case EVENT_TYPE_MAGV_STATUS:
		mPendingMask |= 1<<MagneticField;
		mPendingEvents[MagneticField].magnetic.status = value;
		break;

	case EVENT_TYPE_UCMAGV_X:
		mPendingMask |= 1<<UncalMagneticField;
		mPendingEvents[UncalMagneticField].uncalibrated_magnetic.x_uncalib = value * CONVERT_M;
		mPendingEvents[UncalMagneticField].uncalibrated_magnetic.x_bias = 
			mPendingEvents[UncalMagneticField].uncalibrated_magnetic.x_uncalib
			- mPendingEvents[MagneticField].magnetic.x;
		break;
	case EVENT_TYPE_UCMAGV_Y:
		mPendingMask |= 1<<UncalMagneticField;
		mPendingEvents[UncalMagneticField].uncalibrated_magnetic.y_uncalib = value * CONVERT_M;
		mPendingEvents[UncalMagneticField].uncalibrated_magnetic.y_bias = 
			mPendingEvents[UncalMagneticField].uncalibrated_magnetic.y_uncalib
			- mPendingEvents[MagneticField].magnetic.y;
		break;
	case EVENT_TYPE_UCMAGV_Z:
		mPendingMask |= 1<<UncalMagneticField;
		mPendingEvents[UncalMagneticField].uncalibrated_magnetic.z_uncalib = value * CONVERT_M;
		mPendingEvents[UncalMagneticField].uncalibrated_magnetic.z_bias = 
			mPendingEvents[UncalMagneticField].uncalibrated_magnetic.z_uncalib
			- mPendingEvents[MagneticField].magnetic.z;
	break;

	case EVENT_TYPE_YAW:
		mPendingMask |= 1<<Orientation;
		mPendingEvents[Orientation].orientation.azimuth = value * CONVERT_OR;
		break;
	case EVENT_TYPE_PITCH:
		mPendingMask |= 1<<Orientation;
		mPendingEvents[Orientation].orientation.pitch = value * CONVERT_OR;
		break;
	case EVENT_TYPE_ROLL:
		mPendingMask |= 1<<Orientation;
		mPendingEvents[Orientation].orientation.roll = value * CONVERT_OR;
		break;

	case EVENT_TYPE_ROTVEC_X:
		mPendingMask |= 1<<RotationVector;
		mPendingEvents[RotationVector].data[0] = value * CONVERT_RV;
		break;
	case EVENT_TYPE_ROTVEC_Y:
		mPendingMask |= 1<<RotationVector;
		mPendingEvents[RotationVector].data[1] = value * CONVERT_RV;
		break;
	case EVENT_TYPE_ROTVEC_Z:
		mPendingMask |= 1<<RotationVector;
		mPendingEvents[RotationVector].data[2] = value * CONVERT_RV;
		break;
	case EVENT_TYPE_ROTVEC_W:
		mPendingMask |= 1<<RotationVector;
		mPendingEvents[RotationVector].data[3] = value * CONVERT_RV;
		break;
	}
}

int AkmSensor::flush(int32_t handle)
{
	(void)handle;
	return 0;
}
