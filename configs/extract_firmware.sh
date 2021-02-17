#!/sbin/sh

export SYSDEV="$(readlink -nf "/dev/block/bootdevice/by-name/system")"
export FWDEV="$(readlink -nf "/dev/block/bootdevice/by-name/modem")"
export SYSFS="ext4"

determine_system_mount() {
  if grep -q -e"^$SYSDEV" /proc/mounts; then
    umount $(grep -e"^$SYSDEV" /proc/mounts | cut -d" " -f2)
  fi

  if [ -d /mnt/system ]; then
    SYSMOUNT="/mnt/system"
  elif [ -d /system_root ]; then
    SYSMOUNT="/system_root"
  else
    SYSMOUNT="/system"
  fi

  export S=$SYSMOUNT/system
}

mount_system() {
  mount -t $SYSFS $SYSDEV $SYSMOUNT -o rw,discard
}

unmount_system() {
  umount $SYSMOUNT
}

mount_firmware() {
  mount $FWDEV /firmware
}

unmount_firmware() {
  umount /firmware
}

mount_firmware
determine_system_mount

mount_system

for file in /firmware/image/*.gz; do
  OUT_FILE=$(basename $file .gz)
  gzip -dc $file > $S/vendor/firmware/$OUT_FILE
  chmod 644 $S/vendor/firmware/$OUT_FILE
  chcon u:object_r:firmware_file:s0 $S/vendor/firmware/$OUT_FILE
done

unmount_system
unmount_firmware
