#!/usr/bin/env python3
"""竖屏版：只补蓝，保持亮度。"""
import struct, sys

BLUE = float(sys.argv[1])
OUT = sys.argv[2]

SRC = '/mnt/f/工作区/_tmp/p_sample.bin'   # 竖屏原厂表
COL_OFF = 204
GRID = 17

d = bytearray(open(SRC,'rb').read())
for g in range(GRID):
    for b in range(GRID):
        for r in range(GRID):
            n = g*289 + b*17 + r
            o = COL_OFF + (n//4)*8 + (n%4)*1229*8
            c2c1, c0 = struct.unpack_from('<II', d, o)
            B = (c2c1 & 0xFFF)/4095.0
            G = ((c2c1>>16) & 0xFFF)/4095.0
            B = min(1.0, B * BLUE)
            struct.pack_into('<II', d, o,
                             (int(round(G*4095))<<16)|int(round(B*4095)), c0)
open(OUT,'wb').write(d)
print('built',OUT,'blue',BLUE)

def chk(n,label):
    o = COL_OFF + (n//4)*8 + (n%4)*1229*8
    c2c1,c0 = struct.unpack_from('<II', d, o)
    b_ = c2c1 & 0xFFF
    print('%-8s R=%4d B=%4d G=%4d  B/R=%.3f' % (label,c0,b_,(c2c1>>16)&0xFFF,b_/max(c0,1)))
chk(16*289+16*17+16,'white')
chk(8*289+8*17+8,'gray')
