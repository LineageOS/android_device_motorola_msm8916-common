#!/vendor/bin/sh

start_copying_prebuilt_qcril_db()
{
   if [ -f /system/vendor/qcril.db -a ! -f /data/misc/radio/qcril.db ]; then
      cp /system/vendor/qcril.db /data/misc/radio/qcril.db
      chown -h radio.radio /data/misc/radio/qcril.db
   fi
}

# We take this from cpuinfo because hex "letters" are lowercase there
set -A cinfo `cat /proc/cpuinfo | sed -n "/Revision/p"`
hw=${cinfo[2]#?}

# Now "cook" the value so it can be matched against devtree names
m2=${hw%?}
minor2=${hw#$m2}
m1=${m2%?}
minor1=${m2#$m1}
if [ "$minor2" == "0" ]; then
	minor2=""
	if [ "$minor1" == "0" ]; then
		minor1=""
	fi
fi
setprop ro.hw.revision p${hw%??}$minor1$minor2
unset hw cinfo m1 m2 minor1 minor2

# reload UTAGS
echo 1 > /proc/config/reload

manufacturedate=$(cat /proc/config/date/ascii 2>/dev/null)
if [ ! -z "$manufacturedate" ]; then
	setprop ro.manufacturedate $manufacturedate
fi
unset manufacturedate

t=$(getprop ro.build.tags)
if [[ "$t" != *release* ]]; then
	for p in $(cat /proc/cmdline); do
		if [ ${p%%:*} = "@" ]; then
			v=${p#@:}; a=${v%=*}; b=${v#*=}
			${a%%:*} ${a##*:} $b
	fi
	done
fi
unset p v a b t

# Copy qcril.db if needed for RIL
start_copying_prebuilt_qcril_db
echo 1 > /data/misc/radio/db_check_done

#
# Make modem config folder and copy firmware config to that folder for RIL
#
if [ -f /data/misc/radio/ver_info.txt ]; then
    prev_version_info=`cat /data/misc/radio/ver_info.txt`
else
    prev_version_info=""
fi

cur_version_info=`cat /firmware/verinfo/ver_info.txt`
if [ ! -f /firmware/verinfo/ver_info.txt -o "$prev_version_info" != "$cur_version_info" ]; then
    rm -rf /data/misc/radio/modem_config
    mkdir /data/misc/radio/modem_config
    chmod 770 /data/misc/radio/modem_config
    cp -r /firmware/image/modem_pr/mcfg/configs/* /data/misc/radio/modem_config
    chown -hR radio.radio /data/misc/radio/modem_config
    cp /firmware/verinfo/ver_info.txt /data/misc/radio/ver_info.txt
    chown radio.radio /data/misc/radio/ver_info.txt
fi
cp /firmware/image/modem_pr/mbn_ota.txt /data/misc/radio/modem_config
chown radio.radio /data/misc/radio/modem_config/mbn_ota.txt
echo 1 > /data/misc/radio/copy_complete

# Let kernel know our image version/variant/crm_version
image_version="10:"
image_version+=`getprop ro.build.id`
image_version+=":"
image_version+=`getprop ro.build.version.incremental`
image_variant=`getprop ro.product.name`
image_variant+="-"
image_variant+=`getprop ro.build.type`
oem_version=`getprop ro.build.version.codename`
echo 10 > /sys/devices/soc0/select_image
echo $image_version > /sys/devices/soc0/image_version
echo $image_variant > /sys/devices/soc0/image_variant
echo $oem_version > /sys/devices/soc0/image_crm_version
