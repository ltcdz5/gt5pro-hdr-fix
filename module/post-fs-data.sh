#!/system/bin/sh
# GT5 Pro HDR Fix —— LUT 校正 + HDR 亮度激发配置
MODDIR=${0%/*}
LOG=/data/local/tmp/hdrfix.log
exec >>"$LOG" 2>&1
echo "=== $(date) hdrfix start ==="

# ---- 1. HDR 亮度激发配置（factor 19 + 解除全屏限制）----
if [ -f "$MODDIR/display_brightness_app_list.xml" ]; then
  cp -f "$MODDIR/display_brightness_app_list.xml" /data/system/display_brightness_app_list.xml
  chown system:system /data/system/display_brightness_app_list.xml 2>/dev/null
  chmod 644 /data/system/display_brightness_app_list.xml
  chcon u:object_r:system_data_file:s0 /data/system/display_brightness_app_list.xml 2>/dev/null
  echo "  hdr config installed"
fi

# ---- 2. LUT 校正 ----
if [ -f "$MODDIR/sidecar56.ko" ]; then
  insmod "$MODDIR/sidecar56.ko" 2>/dev/null
  sleep 1
  if [ -f /data/adb/hdrfix/land_custom.bin ]; then
    cat /data/adb/hdrfix/land_custom.bin > /proc/sdehdrlut_land 2>/dev/null
  elif [ -f "$MODDIR/ghdr_lut.bin" ]; then
    cat "$MODDIR/ghdr_lut.bin" > /proc/sdehdrlut_land 2>/dev/null
  fi
  [ -f "$MODDIR/ghdr_port.bin" ] && cat "$MODDIR/ghdr_port.bin" > /proc/sdehdrlut_port 2>/dev/null
  echo "  lut applied"
fi
echo "=== done ==="
