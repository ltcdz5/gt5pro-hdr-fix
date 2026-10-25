#!/system/bin/sh
# HDR Fix WebUI 后端（RGB 增益版）
MODDIR=${0%/*}
CONF=/data/adb/hdrfix.conf
DIR=/data/adb/hdrfix
UP=$DIR/upload.b64
CUSTOM=$DIR/land_custom.bin
PROC=/proc/sdehdrlut_land

mkdir -p "$DIR" 2>/dev/null

getk() { sed -n "s/^$1=//p" "$CONF" 2>/dev/null | head -1; }
setk() {
  [ -f "$CONF" ] || : > "$CONF"
  if grep -q "^$1=" "$CONF" 2>/dev/null; then
    sed -i "s|^$1=.*|$1=$2|" "$CONF"
  else
    echo "$1=$2" >> "$CONF"
  fi
}

case "$1" in
  json)
    r=$(getk rgain); g=$(getk ggain); b=$(getk bgain)
    [ -n "$r" ] || r=1.00
    [ -n "$g" ] || g=1.00
    [ -n "$b" ] || b=1.30
    en=$(cat /sys/module/sidecar56/parameters/enable 2>/dev/null)
    [ -n "$en" ] || en=0
    st=$(cat /proc/sdehdrstat 2>/dev/null | tr -d '\n')
    hc=0; [ -f "$CUSTOM" ] && hc=1
    printf '{"rgain":"%s","ggain":"%s","bgain":"%s","enable":"%s","custom":"%s","stats":"%s"}\n' "$r" "$g" "$b" "$en" "$hc" "$st"
    ;;
  begin) rm -f "$UP"; : > "$UP"; echo ok ;;
  chunk) printf '%s' "$2" >> "$UP" ;;
  commit)
    [ -s "$UP" ] || { echo empty; exit 1; }
    base64 -d < "$UP" > "$CUSTOM.tmp" 2>/dev/null || { echo decode-fail; exit 1; }
    sz=$(stat -c %s "$CUSTOM.tmp" 2>/dev/null)
    if [ "$sz" != "39536" ]; then rm -f "$CUSTOM.tmp"; echo "bad-size:$sz"; exit 1; fi
    mv -f "$CUSTOM.tmp" "$CUSTOM"
    cat "$CUSTOM" > "$PROC" 2>/dev/null || { echo write-fail; exit 1; }
    echo ok
    ;;
  save) setk rgain "$2"; setk ggain "$3"; setk bgain "$4"; echo ok ;;
  enable)
    echo "$2" > /sys/module/sidecar56/parameters/enable 2>/dev/null
    setk enable "$2"
    echo ok
    ;;
  reset)
    rm -f "$CUSTOM"
    setk rgain 1.00; setk ggain 1.00; setk bgain 1.30
    cat "$MODDIR/ghdr_lut.bin" > "$PROC" 2>/dev/null
    echo ok
    ;;
  stats) cat /proc/sdehdrstat 2>/dev/null ;;
  *) echo 'usage: webctl.sh json|begin|chunk <b64>|commit|save <r> <g> <b>|enable 0|1|reset|stats' ;;
esac
