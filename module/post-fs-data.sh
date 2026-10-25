#!/system/bin/sh
MODDIR=${0%/*}
insmod $MODDIR/sidecar56.ko 2>/dev/null
sleep 1
# 优先用 WebUI 调好的自定义表
if [ -f /data/adb/hdrfix/land_custom.bin ]; then
  cat /data/adb/hdrfix/land_custom.bin > /proc/sdehdrlut_land 2>/dev/null
else
  cat $MODDIR/ghdr_lut.bin > /proc/sdehdrlut_land 2>/dev/null
fi
cat $MODDIR/ghdr_port.bin > /proc/sdehdrlut_port 2>/dev/null
