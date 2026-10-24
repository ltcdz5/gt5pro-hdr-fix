#!/system/bin/sh
MODDIR=${0%/*}
insmod $MODDIR/sidecar56.ko 2>/dev/null
sleep 1
cat $MODDIR/ghdr_lut.bin > /proc/sdehdrlut_land 2>/dev/null
cat $MODDIR/ghdr_port.bin > /proc/sdehdrlut_port 2>/dev/null
