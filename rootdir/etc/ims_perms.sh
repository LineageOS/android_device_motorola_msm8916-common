#!/system/bin/sh
export PATH=/system/xbin:$PATH

sign=`pm dump org.codeaurora.ims | grep signatures`

if [[ "$sign" == *"b4addb29"* ]]; then
    pm grant org.codeaurora.ims android.permission.READ_PHONE_STATE
fi
