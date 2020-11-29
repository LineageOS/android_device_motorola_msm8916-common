#
# Copyright (C) 2015 The CyanogenMod Project
# Copyright (C) 2017 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

-include vendor/motorola/msm8916-common/BoardConfigVendor.mk

VENDOR_PATH := device/motorola/msm8916-common

BOARD_VENDOR := motorola-qcom

TARGET_FS_CONFIG_GEN += \
    $(VENDOR_PATH)/fs_config/file_caps.fs \
    $(VENDOR_PATH)/fs_config/qcom_aids.fs \
    $(VENDOR_PATH)/fs_config/mot_aids.fs

TARGET_SPECIFIC_HEADER_PATH := $(VENDOR_PATH)/include

# Platform
TARGET_BOARD_PLATFORM := msm8916
TARGET_BOARD_PLATFORM_GPU := qcom-adreno306

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := MSM8916
TARGET_NO_BOOTLOADER := true

# Architecture
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a53

# Binder API version
TARGET_USES_64_BIT_BINDER := true

# Kernel
BOARD_CUSTOM_BOOTIMG := true
BOARD_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=qcom msm_rtb.filter=0x3F ehci-hcd.park=3 vmalloc=400M androidboot.bootdevice=7824900.sdhci utags.blkdev=/dev/block/bootdevice/by-name/utags utags.backup=/dev/block/bootdevice/by-name/utagsBackup movablecore=160M
BOARD_KERNEL_BASE := 0x80000000
BOARD_KERNEL_IMAGE_NAME := zImage
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_SEPARATED_DT := true
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x01000000 --tags_offset 0x00000100
TARGET_KERNEL_ARCH := arm
TARGET_KERNEL_SOURCE := kernel/motorola/msm8916

# ANT+
BOARD_ANT_WIRELESS_DEVICE := "qualcomm-smd"

# Audio
AUDIO_FEATURE_ENABLED_COMPRESS_VOIP := true
AUDIO_FEATURE_ENABLED_EXTENDED_COMPRESS_FORMAT := true
AUDIO_FEATURE_ENABLED_FLUENCE := true
AUDIO_FEATURE_ENABLED_FM_POWER_OPT := true
AUDIO_FEATURE_ENABLED_MULTI_VOICE_SESSIONS := true
AUDIO_FEATURE_ENABLED_NEW_SAMPLE_RATE := true
AUDIO_FEATURE_ENABLED_PROXY_DEVICE := true
AUDIO_FEATURE_ENABLED_SND_MONITOR := true
BOARD_USES_ALSA_AUDIO := true
USE_CUSTOM_AUDIO_POLICY := 1
USE_XML_AUDIO_POLICY_CONF := 1

# Bluetooth
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(VENDOR_PATH)/bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_QCOM := true
QCOM_BT_READ_ADDR_FROM_PROP := true

# Camera
USE_DEVICE_SPECIFIC_CAMERA := true
TARGET_USES_MEDIA_EXTENSIONS := true
TARGET_PROVIDES_CAMERA_HAL := true

# Charger
BACKLIGHT_PATH := /sys/class/leds/lcd-backlight/brightness
BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_NO_CHARGER_LED := true

# Crypto
TARGET_HW_DISK_ENCRYPTION := true

# Display
OVERRIDE_RS_DRIVER := libRSDriver_adreno.so
TARGET_CONTINUOUS_SPLASH_ENABLED := true
TARGET_USES_ION := true
TARGET_DISABLE_POSTRENDER_CLEANUP := true

# UI
TARGET_ADDITIONAL_GRALLOC_10_USAGE_BITS := 0x02000000U

# FM
BOARD_HAVE_QCOM_FM := true
TARGET_QCOM_NO_FM_FIRMWARE := true

# memfd
TARGET_HAS_MEMFD_BACKPORT := true

# Power
TARGET_USES_INTERACTION_BOOST := true

# Partitions
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_FLASH_BLOCK_SIZE := 131072

# Qualcomm support
BOARD_USES_QCOM_HARDWARE := true
MALLOC_SVELTE := true

# Recovery
BOARD_HAS_NO_SELECT_BUTTON := true
TARGET_RECOVERY_FSTAB := $(VENDOR_PATH)/rootdir/etc/fstab.qcom
TARGET_RECOVERY_DEVICE_DIRS += $(VENDOR_PATH)
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true

# Release Tools
TARGET_RELEASETOOLS_EXTENSIONS := $(VENDOR_PATH)

# Root
BOARD_ROOT_EXTRA_FOLDERS := firmware persist fsg

# SELinux
include device/qcom/sepolicy-legacy/sepolicy.mk
BOARD_VENDOR_SEPOLICY_DIRS += $(VENDOR_PATH)/sepolicy

# Shims
TARGET_LD_SHIM_LIBS := \
    /system/vendor/lib/libril-qc-qmi-1.so|libcutils_shim.so \
    /system/vendor/lib/libmdmcutback.so|libqsap_shim.so \
    /system/vendor/lib/libmot_sensorlistener.so|libsensor.so \
    /system/vendor/lib/libjustshoot.so|libshims_camera.so \
    /system/vendor/lib/libmmcamera_wavelet_lib.so|libc_util.so \
    /system/vendor/lib/libizat_core.so|libshims_get_process_name.so \
    /system/vendor/lib/libqomx_jpegenc.so|libboringssl-compat.so \
    /system/vendor/lib/libmmqjpeg_codec.so|libboringssl-compat.so

# Sp-shim
TARGET_LD_PRELOAD := \
    /system/lib/libboringssl-compat.so

# Vendor Interface Manifest
DEVICE_MANIFEST_FILE := $(VENDOR_PATH)/manifest.xml
DEVICE_MATRIX_FILE := $(VENDOR_PATH)/compatibility_matrix.xml

# Peripheral manager
TARGET_PER_MGR_ENABLED := true

# Wifi
BOARD_HAS_QCOM_WLAN := true
BOARD_WLAN_DEVICE := qcwcn
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_qcwcn
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_qcwcn
PRODUCT_VENDOR_MOVE_ENABLED := true
TARGET_HAS_BROKEN_WLAN_SET_INTERFACE := true
WIFI_DRIVER_FW_PATH_AP  := "ap"
WIFI_DRIVER_FW_PATH_STA := "sta"
WPA_SUPPLICANT_VERSION := VER_0_8_X
WIFI_HIDL_FEATURE_DISABLE_AP_MAC_RANDOMIZATION := true
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true
