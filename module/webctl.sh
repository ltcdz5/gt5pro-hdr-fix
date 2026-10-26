#!/system/bin/sh
# HDR Fix WebUI 后端（分区 RGB 增益：暗部 / 中调 / 高光）
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
    out=""
    for k in rlo glo blo rmid gmid bmid rhi ghi bhi; do
      v=$(getk $k)
      [ -n "$v" ] || {
        case $k in
          blo) v=1.20 ;; bmid) v=1.33 ;; bhi) v=1.10 ;; *) v=1.00 ;;
        esac
      }
      out="$out\"$k\":\"$v\","
    done
    en=$(cat /sys/module/sidecar56/parameters/enable 2>/dev/null)
    [ -n "$en" ] || en=0
    st=$(cat /proc/sdehdrstat 2>/dev/null | tr -d '\n')
    hc=0; [ -f "$CUSTOM" ] && hc=1
    pre=$(getk preset); [ -n "$pre" ] || pre=cal
    hu=0; [ -f "$DIR/user_preset.conf" ] && hu=1
    printf '{%s"enable":"%s","custom":"%s","preset":"%s","hasuser":"%s","stats":"%s"}\n' "$out" "$en" "$hc" "$pre" "$hu" "$st"
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
  save)
    setk rlo "$2";  setk glo "$3";  setk blo "$4"
    setk rmid "$5"; setk gmid "$6"; setk bmid "$7"
    setk rhi "$8";  setk ghi "$9";  setk bhi "${10}"
    echo ok
    ;;
  enable)
    echo "$2" > /sys/module/sidecar56/parameters/enable 2>/dev/null
    setk enable "$2"
    echo ok
    ;;
  saveuser)
    # 保存当前状态为「我的预设」（用户可随时覆盖）
    cp -f "$CONF" "$DIR/user_preset.conf" 2>/dev/null
    if [ -f "$CUSTOM" ]; then
      cp -f "$CUSTOM" "$DIR/user_preset.bin" 2>/dev/null
    else
      rm -f "$DIR/user_preset.bin"
    fi
    setk preset user
    echo ok
    ;;
  loaduser)
    [ -f "$DIR/user_preset.conf" ] || { echo no-preset; exit 1; }
    for k in rlo glo blo rmid gmid bmid rhi ghi bhi; do
      v=$(sed -n "s/^$k=//p" "$DIR/user_preset.conf" 2>/dev/null | head -1)
      [ -n "$v" ] && setk $k "$v"
    done
    if [ -f "$DIR/user_preset.bin" ]; then
      cp -f "$DIR/user_preset.bin" "$CUSTOM" 2>/dev/null
      cat "$CUSTOM" > "$PROC" 2>/dev/null
    fi
    setk preset user
    echo ok
    ;;
  reset)
    # reset cal   = 恢复我的校准（ghdr_lut.bin + preset_cal.conf 的增益）
    # reset stock = 恢复原厂（base_land.bin + 增益全 1.00）
    MODE="$2"
    [ -n "$MODE" ] || MODE=cal
    rm -f "$CUSTOM"
    if [ "$MODE" = "stock" ]; then
      for k in rlo glo blo rmid gmid bmid rhi ghi bhi; do setk $k 1.00; done
      cat "$MODDIR/base_land.bin" > "$PROC" 2>/dev/null
      setk preset stock
    else
      for k in rlo glo blo rmid gmid bmid rhi ghi bhi; do
        v=$(sed -n "s/^$k=//p" "$MODDIR/preset_cal.conf" 2>/dev/null | head -1)
        [ -n "$v" ] || v=1.00
        setk $k "$v"
      done
      cat "$MODDIR/ghdr_lut.bin" > "$PROC" 2>/dev/null
      setk preset cal
    fi
    echo ok
    ;;
  stats) cat /proc/sdehdrstat 2>/dev/null ;;
  *) echo 'usage: webctl.sh json|begin|chunk <b64>|commit|save r g b x9|enable 0|1|reset [cal|stock]|saveuser|loaduser|stats' ;;
esac
