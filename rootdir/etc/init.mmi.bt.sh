#!/vendor/bin/sh

btaddr=`getprop ro.boot.btmacaddr | sed 's/../&:/g;s/:$//'`

setprop persist.service.bdroid.bdaddr $btaddr
