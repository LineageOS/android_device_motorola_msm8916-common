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
 * Copyright (C) 2011-2015 Motorola Mobility LLC
 */

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

#include <linux/stml0xx.h>

#include <cutils/log.h>

#include <hardware/mot_sensorhub_stml0xx.h>

#include "HubSensor.h"

/*****************************************************************************/

HubSensor HubSensor::self;

HubSensor::HubSensor()
: SensorBase(SENSORHUB_DEVICE_NAME, NULL, SENSORHUB_AS_DATA_NAME),
      mEnabled(0),
      mWakeEnabled(0),
      mPendingMask(0),
      mEnabledHandles(0)
{
    // read the actual value of all sensors if they're enabled already
    struct input_absinfo absinfo;
    char flags[3];
    FILE *fp;
    int i;
    int err = 0;

    static_assert(MAX_SENSOR_ID < (sizeof(mEnabledHandles) * CHAR_BIT),
        "enabled handlers bit mask can NOT hold all the handles");

    memset(mErrorCnt, 0, sizeof(mErrorCnt));

    open_device();

#ifdef _ENABLE_MAGNETOMETER
    for (i=0; i< NUM_FUSION_DEVICES; i++) {
        mFusionEnabled[i] = 0;
        mFusionDelay[i] = 200;
    }
#endif

    if (!ioctl(dev_fd, STML0XX_IOCTL_GET_SENSORS, &flags))  {
        mEnabled = flags[0] | (flags[1] << 8) | (flags[2] << 16);
    }

    if (!ioctl(dev_fd, STML0XX_IOCTL_GET_WAKESENSORS, &flags))  {
        mWakeEnabled = flags[0] | (flags[1] << 8) | (flags[2] << 16);
    }
}

HubSensor::~HubSensor()
{
}

HubSensor *HubSensor::getInstance()
{
	return &self;
}

bool HubSensor::isHandleEnabled(uint64_t handle)
{
    return (mEnabledHandles & ((decltype(mEnabledHandles))1 << handle)) != 0;
}

int HubSensor::setEnable(int32_t handle, int en)
{
    int newState  = en ? 1 : 0;
    uint32_t new_enabled;
    int found = 0;
    int err = 0;

    new_enabled = mEnabled;
    switch (handle) {
        case ID_A:
#ifdef _ENABLE_MAGNETOMETER
            mFusionEnabled[ACCEL] = newState;
#else
            new_enabled &= ~M_ACCEL;
            if (newState)
                new_enabled |= M_ACCEL;
#endif
            found = 1;
            break;
        case ID_L:
            new_enabled &= ~M_ALS;
            if (newState)
                new_enabled |= M_ALS;
            found = 1;
            break;
        case ID_DR:
            new_enabled &= ~M_DISP_ROTATE;
            if (newState)
                new_enabled |= M_DISP_ROTATE;
            found = 1;
            break;
        case ID_A2:
            new_enabled &= ~M_ACCEL2;
            if (newState)
                new_enabled |= M_ACCEL2;
            found = 1;
            break;
#ifdef _ENABLE_MAGNETOMETER
        case ID_OR:
            mFusionEnabled[ORIENTATION] = newState;
            found = 1;
            break;
        case ID_RV:
            mFusionEnabled[ROTATION] = newState;
            found = 1;
            break;
#endif
    }

#ifdef _ENABLE_MAGNETOMETER
    if ((handle == ID_A) || (handle == ID_OR) || (handle == ID_RV)) {
	unsigned short delay =  200;

        if (mFusionEnabled[ACCEL])
		delay = mFusionDelay[ACCEL];
        if (mFusionEnabled[ORIENTATION] && (mFusionDelay[ORIENTATION] < delay))
		delay = mFusionDelay[ORIENTATION];
        if (mFusionEnabled[ROTATION] && (mFusionDelay[ROTATION] < delay))
		delay = mFusionDelay[ROTATION];

        err = ioctl(dev_fd,  STML0XX_IOCTL_SET_ACC_DELAY, &delay);
	ALOGE_IF(err, "Could not change delay(%s)", strerror(-err));

        int accel_enable = mFusionEnabled[ACCEL] | mFusionEnabled[ORIENTATION] | mFusionEnabled[ROTATION];
        new_enabled &= ~M_ACCEL;
        if (accel_enable)
            new_enabled |= M_ACCEL;
    }
#endif

    if (found && (new_enabled != mEnabled)) {
        err = ioctl(dev_fd, STML0XX_IOCTL_SET_SENSORS, &new_enabled);
        ALOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
        // Never return this error to the caller. This would result in a
        // failure to registerListener(), but regardless of failure, we
        // will consider these sensors 'registered' in the kernel driver.
        err = 0;
        mEnabled = new_enabled;
    }

    new_enabled = mWakeEnabled;
    found = 0;
    switch (handle) {
        case ID_P:
            new_enabled &= ~M_PROXIMITY;
            if (newState)
                new_enabled |= M_PROXIMITY;
            found = 1;
            break;
        case ID_FU:
            new_enabled &= ~M_FLATUP;
            if (newState)
                new_enabled |= M_FLATUP;
            found = 1;
            break;
        case ID_FD:
            new_enabled &= ~M_FLATDOWN;
            if (newState)
                new_enabled |= M_FLATDOWN;
            found = 1;
            break;
        case ID_S:
            new_enabled &= ~M_STOWED;
            if (newState)
                new_enabled |= M_STOWED;
            found = 1;
            break;
        case ID_CA:
            new_enabled &= ~M_CAMERA_ACT;
            if (newState)
                new_enabled |= M_CAMERA_ACT;
            found = 1;
            break;
#ifdef _ENABLE_LIFT
        case ID_LF:
            new_enabled &= ~M_LIFT;
            if (newState)
                new_enabled |= M_LIFT;
            found = 1;
            break;
#endif
#ifdef _ENABLE_CHOPCHOP
        case ID_CC:
            new_enabled &= ~M_CHOPCHOP;
            if (newState)
                new_enabled |= M_CHOPCHOP;
            found = 1;
            break;
#endif
        case ID_GLANCE_GESTURE:
            // enable only if moto_glance not enabled
            // disable only if moto_glance not enabled
            if (!isHandleEnabled(ID_MOTO_GLANCE_GESTURE)) {
                new_enabled &= ~M_GLANCE;
                if (newState)
                    new_enabled |= M_GLANCE;
                found = 1;
            }
            break;
        case ID_MOTO_GLANCE_GESTURE:
            // enable only if glance not enabled
            // disable only if glance not enabled
            if (!isHandleEnabled(ID_GLANCE_GESTURE)) {
                new_enabled &= ~M_GLANCE;
                if (newState)
                    new_enabled |= M_GLANCE;
                found = 1;
            }
            break;
    }

    if (found && (new_enabled != mWakeEnabled)) {
        err = ioctl(dev_fd, STML0XX_IOCTL_SET_WAKESENSORS, &new_enabled);
        ALOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
        // Never return this error to the caller. This would result in a
        // failure to registerListener(), but regardless of failure, we
        // will consider these sensors 'registered' in the kernel driver.
        err = 0;
        mWakeEnabled = new_enabled;
    }


    if (newState)
        mEnabledHandles |= ((decltype(mEnabledHandles))1 << handle);
    else
        mEnabledHandles &= ~((decltype(mEnabledHandles))1 << handle);

    return err;
}

int HubSensor::setDelay(int32_t handle, int64_t ns)
{
    int err = 0;

    if (ns < 0)
        return -EINVAL;

    unsigned short delay = int64_t(ns) / 1000000;
    switch (handle) {
        case ID_A:
#ifdef _ENABLE_MAGNETOMETER
                mFusionDelay[ACCEL] = delay;
#else
	        err = ioctl(dev_fd, STML0XX_IOCTL_SET_ACC_DELAY, &delay);
#endif
		break;
        case ID_A2:
		err = ioctl(dev_fd, STML0XX_IOCTL_SET_ACC2_DELAY, &delay);
		break;
        case ID_L:
	        err = ioctl(dev_fd, STML0XX_IOCTL_SET_ALS_DELAY, &delay);
                break;
        case ID_DR:
        case ID_P:
        case ID_FU:
        case ID_FD:
        case ID_S:
        case ID_CA:
        case ID_GLANCE_GESTURE:
        case ID_MOTO_GLANCE_GESTURE:
#ifdef _ENABLE_LIFT
        case ID_LF:
#endif
#ifdef _ENABLE_CHOPCHOP
        case ID_CC:
#endif
		break;
#ifdef _ENABLE_MAGNETOMETER
	case ID_OR:
                mFusionDelay[ORIENTATION] = delay;
		break;
	case ID_RV:
                mFusionDelay[ROTATION] = delay;
		break;
#endif
	default:
		err = -EINVAL;
    }

#ifdef _ENABLE_MAGNETOMETER
    if ((handle == ID_A) || (handle == ID_OR) || (handle == ID_RV)) {
	if (mFusionEnabled[ACCEL])
		delay = mFusionDelay[ACCEL];
        if (mFusionEnabled[ORIENTATION] && (mFusionDelay[ORIENTATION] < delay))
		delay = mFusionDelay[ORIENTATION];
        if (mFusionEnabled[ROTATION] && (mFusionDelay[ROTATION] < delay))
		delay = mFusionDelay[ROTATION];

        err = ioctl(dev_fd,  STML0XX_IOCTL_SET_ACC_DELAY, &delay);
	ALOGE_IF(err, "Could not change delay(%s)", strerror(-err));
    }
#endif
    // Never return this error to the caller. This would result in a
    // failure to registerListener(), but regardless of failure, we
    // will consider these sensors 'registered' at the rate we tried
    // to write in the kernel driver.
    if( err == -EIO )
        err = 0;
    return err;
}

void HubSensor::logAlsEvent(int16_t lux, int64_t ts_ns) {
    static int16_t last_logged_val = -1;
    static int64_t last_logged_ts_ns;
    int16_t luxDelta = abs(lux - last_logged_val);
    if (last_logged_val == -1 ||
        (luxDelta > last_logged_val * 0.15 && luxDelta >= 5 &&
         ts_ns - last_logged_ts_ns >= 1000000000LL)) {
        ALOGD("ALS %d", lux);
        last_logged_val = lux;
        last_logged_ts_ns = ts_ns;
    }
}

int HubSensor::readEvents(sensors_event_t* d, int dLen)
{
    struct stml0xx_android_sensor_data buff;
    int ret;
    char timeBuf[32];
    struct tm* ptm = NULL;
    struct timeval timeutc;
    static long int sent_bug2go_sec = 0;
    sensors_event_t* data = d;

    // Ensure there are at least 2 slots free in the buffer
    // because we can send 2 events at once below.
    sensors_event_t const* const dataEnd = d + dLen - 1;

    if (!data) {
        ALOGE("HubSensor::readEvents - null data buffer");
        return -EINVAL;
    }
    if (dLen < 1) {
        ALOGE("HubSensor::readEvents - bad length %d", dLen);
        return -EINVAL;
    }

    while (data < dataEnd && ((ret = read(data_fd, &buff, sizeof(struct stml0xx_android_sensor_data))) != 0)) {
        /* Sensorhub reset occurred, upload a bug2go if its been at least 10mins since previous bug2go*/
        /* remove this if-clause when corruption issue resolved */
        switch (buff.type) {
            case DT_FLUSH:
                data->version = META_DATA_VERSION;
                data->sensor = 0;
                data->type = SENSOR_TYPE_META_DATA;
                data->reserved0 = 0;
                data->timestamp = 0;
                data->meta_data.what = META_DATA_FLUSH_COMPLETE;
                data->meta_data.sensor = STM32TOH(buff.data + FLUSH);
                data++;
                break;
            case DT_ACCEL:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_A;
                data->type = SENSOR_TYPE_ACCELEROMETER;
                data->acceleration.x = STM16TOH(buff.data+ACCEL_X) * CONVERT_A_X;
                data->acceleration.y = STM16TOH(buff.data+ACCEL_Y) * CONVERT_A_Y;
                data->acceleration.z = STM16TOH(buff.data+ACCEL_Z) * CONVERT_A_Z;
                data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
                data->timestamp = buff.timestamp;
                data++;
                break;
            case DT_ACCEL2:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_A2;
                data->type = SENSOR_TYPE_ACCELEROMETER;
                data->acceleration.x = STM16TOH(buff.data+ACCEL_X) * CONVERT_A_X;
                data->acceleration.y = STM16TOH(buff.data+ACCEL_Y) * CONVERT_A_Y;
                data->acceleration.z = STM16TOH(buff.data+ACCEL_Z) * CONVERT_A_Z;
                data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
                data->timestamp = buff.timestamp;
                data++;
                break;
            case DT_ALS:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_L;
                data->type = SENSOR_TYPE_LIGHT;
                data->light = (uint16_t)STM16TOH(buff.data + LIGHT_LIGHT);
                data->timestamp = buff.timestamp;
                logAlsEvent((int16_t)data->light, data->timestamp);
                data++;
                break;
            case DT_DISP_ROTATE:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_DR;
                data->type = SENSOR_TYPE_DISPLAY_ROTATE;
                if (buff.data[ROTATE_ROTATE] == DISP_FLAT)
                    data->data[0] = DISP_UNKNOWN;
                else
                    data->data[0] = buff.data[ROTATE_ROTATE];

                data->timestamp = buff.timestamp;
                data++;
                break;
            case DT_PROX:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_P;
                data->type = SENSOR_TYPE_PROXIMITY;
                if (buff.data[PROXIMITY_PROXIMITY] == 0) {
                    data->distance = PROX_UNCOVERED;
                    ALOGE("Proximity uncovered");
                } else if (buff.data[PROXIMITY_PROXIMITY] == 1) {
                    data->distance = PROX_COVERED;
                    ALOGE("Proximity covered 1");
                } else {
                    data->distance = PROX_SATURATED;
                    ALOGE("Proximity covered 2");
                }
                data->timestamp = buff.timestamp;
                data++;
                break;
            case DT_FLAT_UP:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_FU;
                data->type = SENSOR_TYPE_FLAT_UP;
                if (buff.data[FLAT_FLAT] == 0x01)
                    data->data[0] = FLAT_DETECTED;
                else
                    data->data[0] = FLAT_NOTDETECTED;
                data->timestamp = buff.timestamp;
                data++;
                break;
            case DT_FLAT_DOWN:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_FD;
                data->type = SENSOR_TYPE_FLAT_DOWN;
                if (buff.data[FLAT_FLAT] == 0x02)
                    data->data[0] = FLAT_DETECTED;
                else
                    data->data[0] = FLAT_NOTDETECTED;
                data->timestamp = buff.timestamp;
                data++;
                break;
            case DT_STOWED:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_S;
                data->type = SENSOR_TYPE_STOWED;
                data->data[0] = buff.data[STOWED_STOWED];
                data->timestamp = buff.timestamp;
                data++;
                break;
            case DT_CAMERA_ACT:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_CA;
                data->type = SENSOR_TYPE_CAMERA_ACTIVATE;
                data->data[0] = STML0XX_CAMERA_DATA;
                data->data[1] = STM16TOH(buff.data + CAMERA_CAMERA);
                data->timestamp = buff.timestamp;
                data++;
                break;
            case DT_RESET:
                time(&timeutc.tv_sec);
                if (buff.data[0] > 0 && buff.data[0] <= ERROR_TYPES)
                    mErrorCnt[buff.data[0] - 1]++;
                if ((sent_bug2go_sec == 0) ||
                    (timeutc.tv_sec - sent_bug2go_sec > 24*60*60)) {
                    // put timestamp in dropbox file
                    ptm = localtime(&(timeutc.tv_sec));
                    if (ptm != NULL) {
                        strftime(timeBuf, sizeof(timeBuf), "%m-%d %H:%M:%S", ptm);
                        capture_dump(timeBuf, buff.type, SENSORHUB_DUMPFILE,
                            DROPBOX_FLAG_TEXT | DROPBOX_FLAG_GZIP);
                    }
                    sent_bug2go_sec = timeutc.tv_sec;
                }
                break;
#ifdef _ENABLE_LIFT
            case DT_LIFT:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_LF;
                data->type = SENSOR_TYPE_LIFT_GESTURE;
                data->data[0] = STM32TOH(buff.data + LIFT_DISTANCE);
                data->data[1] = STM32TOH(buff.data + LIFT_ROTATION);
                data->data[2] = STM32TOH(buff.data + LIFT_GRAV_DIFF);
                data->timestamp = buff.timestamp;
                data++;
                break;
#endif
#ifdef _ENABLE_CHOPCHOP
            case DT_CHOPCHOP:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = SENSORS_HANDLE_BASE + ID_CC;
                data->type = SENSOR_TYPE_CHOPCHOP_GESTURE;
                data->data[0] = STM32TOH(buff.data + CHOPCHOP_CHOPCHOP);
                data->timestamp = buff.timestamp;
                data++;
                break;
#endif
            case DT_GLANCE:
                if (isHandleEnabled(ID_MOTO_GLANCE_GESTURE)) {
                    data->version = SENSORS_EVENT_T_SIZE;
                    data->sensor = ID_MOTO_GLANCE_GESTURE;
                    data->type = SENSOR_TYPE_MOTO_GLANCE_GESTURE;
                    data->data[0] = STM16TOH(buff.data); /* Gesture that triggered glance */
                    data->data[1] = 0;
                    data->data[2] = 0;
                    data->timestamp = buff.timestamp;
                    data++;

                    /* Disable, because this is a one shot sensor */
                    setEnable(ID_MOTO_GLANCE_GESTURE, 0);
                }

                if (isHandleEnabled(ID_GLANCE_GESTURE)) {
                    data->version = SENSORS_EVENT_T_SIZE;
                    data->sensor = ID_GLANCE_GESTURE;
                    data->type = SENSOR_TYPE_GLANCE_GESTURE;
                    data->data[0] = 1.0;                   /* set to 1 for Android compatibility */
                    data->data[1] = STM16TOH(buff.data);   /* Currently blocked by Android FW */
                    data->data[2] = 0;
                    data->timestamp = buff.timestamp;
                    data++;

                    /* Disable, because this is a one shot sensor */
                    setEnable(ID_GLANCE_GESTURE, 0);
                }

                break;
            default:
                break;
        }
    }

    return data - d;
}

int HubSensor::flush(int32_t handle)
{
    int ret = 0;
    if (handle > MIN_SENSOR_ID && handle < MAX_SENSOR_ID) {
        ret = ioctl(dev_fd,  STML0XX_IOCTL_SET_FLUSH, &handle);
    }
    return ret;
}

gzFile HubSensor::open_dropbox_file(const char* timestamp, const char* dst, const int flags)
{
    (void)dst;

    char dropbox_path[128];
    pid_t pid = getpid();

    snprintf(dropbox_path, sizeof(dropbox_path), "%s/%s:%d:%u-%s",
             DROPBOX_DIR, DROPBOX_TAG, flags, pid, timestamp);
    ALOGD("stml0xx - dumping to dropbox file[%s]...\n", dropbox_path);

    return gzopen(dropbox_path, "wb");
}

short HubSensor::capture_dump(char* timestamp, const int id, const char* dst, const int flags)
{
    char buffer[COPYSIZE] = {0};
    int rc = 0, i = 0;
    gzFile dropbox_file = NULL;

    dropbox_file = open_dropbox_file(timestamp, dst, flags);
    if(dropbox_file == NULL) {
        ALOGE("ERROR! unable to open dropbox file[errno:%d(%s)]\n", errno, strerror(errno));
    } else {
        // put timestamp in dropbox file
        rc = snprintf(buffer, COPYSIZE, "timestamp:%s\n", timestamp);
        gzwrite(dropbox_file, buffer, rc);
        rc = snprintf(buffer, COPYSIZE, "reason:%02d\n", id);
        gzwrite(dropbox_file, buffer, rc);

        for (i = 0; i < ERROR_TYPES; i++) {
            rc = snprintf(buffer, COPYSIZE, "[%d]:%d\n", i+1, mErrorCnt[i]);
            gzwrite(dropbox_file, buffer, rc);
        }
        memset(mErrorCnt, 0, sizeof(mErrorCnt));

        gzclose(dropbox_file);
        // to commit buffer cache to disk
        sync();
    }

    return 0;
}
