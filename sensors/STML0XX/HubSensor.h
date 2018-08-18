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

#ifndef ANDROID_HUB_SENSOR_H
#define ANDROID_HUB_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <endian.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <zlib.h>
#include <time.h>
#include <private/android_filesystem_config.h>

#include <linux/stml0xx.h>

#include "Sensors.h"
#include "SensorBase.h"

/*****************************************************************************/

#define SENSORHUB_DEVICE_NAME       "/dev/stml0xx"
#define SENSORHUB_AS_DATA_NAME      "/dev/stml0xx_as"

#define SENSORS_EVENT_T_SIZE sizeof(sensors_event_t);
#define DROPBOX_DIR "/data/system/dropbox-add"
#define DROPBOX_TAG "SENSOR_HUB"
#define SENSORHUB_DUMPFILE  "sensor_hub"
#define DROPBOX_FLAG_TEXT        2
#define DROPBOX_FLAG_GZIP        4
#define COPYSIZE 256

// Defines for offsets into the sensorhub event data.
#define ACCEL_X (0 * sizeof(int16_t))
#define ACCEL_Y (1 * sizeof(int16_t))
#define ACCEL_Z (2 * sizeof(int16_t))
#define LIGHT_LIGHT (0 * sizeof(int16_t))
#define ROTATE_ROTATE (0 * sizeof(int8_t))
#define PROXIMITY_PROXIMITY (0 * sizeof(int8_t))
#define FLAT_FLAT (0 * sizeof(int8_t))
#define STOWED_STOWED (0 * sizeof(int8_t))
#define CAMERA_CAMERA (0 * sizeof(int16_t))
#define FLUSH (0 * sizeof(int32_t))
#define LIFT_DISTANCE (0 * sizeof(int8_t))
#define LIFT_ROTATION (4 * sizeof(int8_t))
#define LIFT_GRAV_DIFF (8 * sizeof(int8_t))
#define CHOPCHOP_CHOPCHOP (0 * sizeof(int8_t))

#define STM16TOH(p) (int16_t) be16toh(*((uint16_t *) (p)))
#define STM32TOH(p) (int32_t) be32toh(*((uint32_t *) (p)))

#define ERROR_TYPES    4

struct input_event;

class HubSensor : public SensorBase {
public:
	HubSensor();
	virtual ~HubSensor();

	static HubSensor* getInstance();
	virtual int setEnable(int32_t handle, int enabled);
	virtual int setDelay(int32_t handle, int64_t ns);
	virtual int readEvents(sensors_event_t* data, int count);
	virtual int flush(int32_t handle);

private:
#ifdef _ENABLE_MAGNETOMETER
	typedef enum fusion_enum
	{
		ACCEL,
		ORIENTATION,
		ROTATION,
		NUM_FUSION_DEVICES
	} Fusion_Device;

	uint32_t mFusionEnabled[NUM_FUSION_DEVICES];
	unsigned short mFusionDelay[NUM_FUSION_DEVICES];
#endif

	static HubSensor self;
	uint32_t mEnabled;
	uint32_t mWakeEnabled;
	uint32_t mPendingMask;
	uint32_t mFlushEnabled;

	uint8_t mErrorCnt[ERROR_TYPES];
	gzFile open_dropbox_file(const char* timestamp, const char* dst, const int flags);
	short capture_dump(char* timestamp, const int id, const char* dst, const int flags);
	void logAlsEvent(int16_t lux, int64_t ts_ns);
};

/*****************************************************************************/

#endif  // ANDROID_HUB_SENSOR_H
