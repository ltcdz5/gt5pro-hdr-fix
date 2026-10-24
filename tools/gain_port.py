#!/usr/bin/env python3
"""按比例抬 B 通道，生成竖屏 HDR 校正表。

用法: python3 gain_port.py <kB百分比> <基准竖屏表> <输出>
例:   python3 gain_port.py 112 base_portrait.bin out.bin
"""
import struct, sys

kB = int(sys.argv[1])
src, out = sys.argv[2], sys.argv[3]

COL_OFF = 204
NCOL = 4 * 1229

d = bytearray(open(src, 'rb').read())
for k in range(NCOL):
    o = COL_OFF + (k // 4) * 8 + (k % 4) * 1229 * 8
    c2_c1, c0 = struct.unpack_from('<II', d, o)
    b = c2_c1 & 0xFFF          # 低 16 位是 B
    g = (c2_c1 >> 16) & 0xFFF  # 高 16 位是 G
    b = min(4095, b * kB // 100)
    struct.pack_into('<II', d, o, (g << 16) | b, c0)

open(out, 'wb').write(d)
print('built', out, 'kB =', kB)
