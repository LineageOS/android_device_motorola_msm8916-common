# Copyright (C) 2018 Alberto97
# Copyright (C) 2009-2014 Motorola Mobility, Inc.
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_TOP_DIR := $(call my-dir)
LOCAL_PATH := $(LOCAL_TOP_DIR)

# Must be called before including any other makefiles.
include $(call all-subdir-makefiles)

# Restore LOCAL_PATH. Other makefiles probably modified it.
LOCAL_PATH := $(LOCAL_TOP_DIR)

###########################################
# Motorola SensorHub section only         #
# Sensors are connected to motorola       #
# internal sensorhub like STM401          #
###########################################
ifeq ($(BOARD_USES_STML0XX_SENSOR_HUB), true)

    UTILS_PATH := utils

    SH_MODULE := stml0xx
    SH_PATH := STML0XX
    SH_LOGTAG := \"STML0XX\"
    ifneq (,$(filter eng,$(TARGET_BUILD_VARIANT)))
        # Expose secondary accel for non-user builds
        SH_CFLAGS += -D_ENABLE_ACCEL_SECONDARY
    endif
    ifneq ($(filter lux osprey, $(TARGET_DEVICE)),)
        SH_CFLAGS += -D_ENABLE_MAGNETOMETER
        SH_CFLAGS += -D_ENABLE_CHOPCHOP
    endif
    ifneq ($(filter merlin, $(TARGET_DEVICE)),)
        SH_CFLAGS += -D_ENABLE_CHOPCHOP
    endif

    ######################
    # Sensors HAL module #
    ######################
    include $(CLEAR_VARS)

    LOCAL_CFLAGS := -DLOG_TAG=\"MotoSensors\"
    LOCAL_CFLAGS += $(SH_CFLAGS)

    ifneq ($(filter surnia lux merlin osprey, $(TARGET_DEVICE)),)
        # Sensor HAL file for M0 hub (low-tier) products
        LOCAL_SRC_FILES :=              \
            $(SH_PATH)/SensorBase.cpp   \
            $(SH_PATH)/HubSensor.cpp    \
            $(SH_PATH)/SensorHal.cpp

        ifneq ($(filter lux osprey, $(TARGET_DEVICE)),)
            # Additional Sensor HAL file for M0 hub products with magnetometer
            LOCAL_SRC_FILES +=              \
                $(SH_PATH)/AkmSensor.cpp    \
                $(SH_PATH)/InputEventReader.cpp
        endif # lux || osprey

        # This file must be last
        LOCAL_SRC_FILES += \
            $(SH_PATH)/SensorsPollContext.cpp
    endif # surnia || lux || merlin || osprey

    LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SH_PATH)
    LOCAL_C_INCLUDES += external/zlib

    LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
    # Need the UAPI output directory to be populated with stml0xx.h
    LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

    LOCAL_PRELINK_MODULE := false
    LOCAL_MODULE_RELATIVE_PATH := hw
    LOCAL_MODULE_TAGS := optional
    LOCAL_SHARED_LIBRARIES := liblog libcutils libz libdl libutils
    LOCAL_PROPRIETARY_MODULE := true
    LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

    include $(BUILD_SHARED_LIBRARY)

    #########################
    # Sensor Hub HAL module #
    #########################
    include $(CLEAR_VARS)

    LOCAL_PRELINK_MODULE := false
    LOCAL_MODULE_RELATIVE_PATH := hw
    LOCAL_SRC_FILES := $(SH_PATH)/sensorhub.c
    LOCAL_SRC_FILES += $(UTILS_PATH)/sensor_time.cpp

    LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
    # Need the UAPI output directory to be populated with stml0xx.h
    LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

    LOCAL_SHARED_LIBRARIES := libcutils libc libutils liblog
    LOCAL_PROPRIETARY_MODULE := true
    LOCAL_MODULE := sensorhub.$(TARGET_BOARD_PLATFORM)
    LOCAL_MODULE_TAGS := optional

    include $(BUILD_SHARED_LIBRARY)

    #########################
    # AKM executable        #
    #########################

    ifneq ($(filter lux osprey, $(TARGET_DEVICE)),)
        include $(CLEAR_VARS)

        AKM_PATH := 8916_ak09912_akmd_6D
        SMARTCOMPASS_LIB := libSmartCompass

        LOCAL_MODULE_TAGS := optional

        LOCAL_MODULE  := akmd09912
        LOCAL_PROPRIETARY_MODULE := true

        LOCAL_C_INCLUDES := \
            $(LOCAL_PATH)/$(AKM_PATH) \
            $(LOCAL_PATH)/$(AKM_PATH)/$(SMARTCOMPASS_LIB)

        LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
        # Need the UAPI output directory to be populated with akm09912.h
        LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

        LOCAL_SRC_FILES := \
            $(AKM_PATH)/AKMD_Driver.c \
            $(AKM_PATH)/DispMessage.c \
            $(AKM_PATH)/FileIO.c \
            $(AKM_PATH)/Measure.c \
            $(AKM_PATH)/main.c \
            $(AKM_PATH)/misc.c \
            $(AKM_PATH)/FST_AK09912.c \
            $(AKM_PATH)/Acc_aot.c

        LOCAL_CFLAGS := -DAKMD_FOR_AK09912
        LOCAL_CFLAGS += -DAKMD_AK099XX
        LOCAL_CFLAGS += -DAKMD_ACC_EXTERNAL
        LOCAL_CFLAGS += -Wall -Wextra
        LOCAL_CFLAGS += -DENABLE_AKMDEBUG=1
        #LOCAL_CFLAGS += -DAKM_LOG_ENABLE

        LOCAL_STATIC_LIBRARIES := AK09912

        LOCAL_FORCE_STATIC_EXECUTABLE := false
        LOCAL_SHARED_LIBRARIES := libc libm libutils libcutils liblog

        include $(BUILD_EXECUTABLE)


        include $(CLEAR_VARS)
        LOCAL_MODULE        := AK09912
        LOCAL_MODULE_TAGS   := optional
        LOCAL_MODULE_CLASS  := STATIC_LIBRARIES
        LOCAL_MODULE_SUFFIX := .a
        LOCAL_SRC_FILES_arm   := $(AKM_PATH)/$(SMARTCOMPASS_LIB)/arm/libAK09912.a
        LOCAL_SRC_FILES_arm64 := $(AKM_PATH)/$(SMARTCOMPASS_LIB)/arm64/libAK09912.a
        include $(BUILD_PREBUILT)

    endif # lux osprey

    ###########################
    # Sensor Hub Flash loader #
    ###########################
    include $(CLEAR_VARS)

    LOCAL_REQUIRED_MODULES := sensorhub.$(TARGET_BOARD_PLATFORM)
    LOCAL_REQUIRED_MODULES += sensors.$(TARGET_BOARD_PLATFORM)

    LOCAL_MODULE_TAGS := optional
    LOCAL_CFLAGS := -DLOG_TAG=$(SH_LOGTAG)
    LOCAL_SRC_FILES := $(SH_PATH)/$(SH_MODULE).cpp
    LOCAL_MODULE:= $(SH_MODULE)
    LOCAL_PROPRIETARY_MODULE := true
    #LOCAL_CFLAGS+= -D_DEBUG
    LOCAL_CFLAGS += -Wall -Wextra -Weffc++
    LOCAL_SHARED_LIBRARIES := libcutils libc liblog

    LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
    # Need the UAPI output directory to be populated with stml0xx.h
    LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

    include $(BUILD_EXECUTABLE)

endif # BOARD_USES_STML0XX_SENSOR_HUB
