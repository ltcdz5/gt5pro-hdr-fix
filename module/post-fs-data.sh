#!/system/bin/sh
MODDIR=${0%/*}
insmod $MODDIR/sidecar42.ko 2>/dev/null
sleep 1
cat $MODDIR/ghdr_lut.bin > /proc/sdehdrlut 2>/dev/null
