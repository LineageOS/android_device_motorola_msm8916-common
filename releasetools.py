# Copyright (C) 2016 The CyanogenMod Project
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

def IncrementalOTA_VerifyBegin(info):
  # Workaround for apn list changes
  RestoreApnList(info)

def IncrementalOTA_InstallEnd(info):
  ReplaceApnList(info)

def FullOTA_InstallBegin(info):
  IMSWorkaround(info)

def FullOTA_InstallEnd(info):
  ReplaceApnList(info)
  ExtractFirmwares(info)

def IMSWorkaround(info):
  info.script.Mount("/system")
  # 1481155200 = 08 Dec 2016 00:00:00 GMT
  info.script.AppendExtra('if less_than_int(file_getprop("/system/build.prop", "ro.build.date.utc"), 1481155200) ||'
    + ' is_substring("13.0", file_getprop("/system/build.prop", "ro.cm.version")) then')
  info.script.AppendExtra("if is_mounted(\"/data\") then")
  info.script.AppendExtra('delete("/data/system/packages.xml");')
  info.script.AppendExtra("else")
  info.script.Mount("/data")
  info.script.AppendExtra('delete("/data/system/packages.xml");')
  info.script.Unmount("/data")
  info.script.AppendExtra("endif;")
  info.script.Unmount("/system")

def ExtractFirmwares(info):
  info.script.Mount("/system")
  info.script.AppendExtra('mount("ext4", "EMMC", "/dev/block/bootdevice/by-name/modem", "/firmware", "");')
  info.script.AppendExtra('ui_print("Extracting modem firmware");')
  info.script.AppendExtra('run_program("/sbin/sh", "/tmp/install/bin/extract_firmware.sh");')
  info.script.AppendExtra('ui_print("Firmware extracted");')
  info.script.AppendExtra('unmount("/firmware");')
  info.script.Unmount("/system")


# --- osprey only ---
def ReplaceApnList(info):
  info.script.AppendExtra('if getprop("ro.boot.hardware.sku") == "XT1548" then')
  info.script.Mount("/system")
  info.script.AppendExtra('run_program("/sbin/sh", "-c", "mv /system/etc/apns-conf.xml /system/etc/apns-conf.xml.bak");')
  info.script.AppendExtra('ifelse(getprop("ro.boot.carrier") == "sprint", ' +
                          'run_program("/sbin/sh", "-c", "mv /system/etc/apns-conf-vmob.xml /system/etc/apns-conf.xml"), ' +
                          'run_program("/sbin/sh", "-c", "mv /system/etc/apns-conf-usc.xml /system/etc/apns-conf.xml"));')
  info.script.Unmount("/system")
  info.script.AppendExtra('endif;')

def RestoreApnList(info):
  info.script.AppendExtra('if getprop("ro.boot.hardware.sku") == "XT1548" then')
  info.script.Mount("/system")
  info.script.AppendExtra('delete("/system/etc/apns-conf.xml");')
  info.script.AppendExtra('run_program("/sbin/sh", "-c", "mv /system/etc/apns-conf.xml.bak /system/etc/apns-conf.xml");')
  info.script.Unmount("/system")
  info.script.AppendExtra('endif;')
