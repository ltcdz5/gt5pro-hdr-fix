#!/usr/bin/env python3
"""保持 R/G 白点亮度，只在 B 通道全段补蓝（含高光）。
这样白色不会变暗，只会变白。
"""
import struct, sys

BLUE_TOP = float(sys.argv[1])   # 高光补蓝 (1.10 = +10%)
BLUE_MID = float(sys.argv[2])   # 中段补蓝
OUT      = sys.argv[3]

SRC = '/mnt/f/工作区/_tmp/ghdr4.bin'
COL_OFF = 204
GRID = 17

def blue_curve(x, top, mid):
    """x=0 -> mid, x=1 -> top，线性插值"""
    return mid + (top - mid) * x

d = bytearray(open(SRC,'rb').read())

for g in range(GRID):
    for b in range(GRID):
        for r in range(GRID):
            n = g*289 + b*17 + r
            o = COL_OFF + (n//4)*8 + (n%4)*1229*8
            c2c1, c0 = struct.unpack_from('<II', d, o)
            R = c0/4095.0
            B = (c2c1 & 0xFFF)/4095.0
            G = ((c2c1>>16) & 0xFFF)/4095.0

            # 只动 B，R/G 完全不碰（亮度不变）
            B = B * blue_curve(B, BLUE_TOP, BLUE_MID)
            B = max(0.0, min(1.0, B))

            struct.pack_into('<II', d, o,
                             (int(round(G*4095))<<16)|int(round(B*4095)),
                             int(round(R*4095)))

open(OUT,'wb').write(d)
print('built',OUT,'blue_top',BLUE_TOP,'blue_mid',BLUE_MID)

def chk(n,label):
    o = COL_OFF + (n//4)*8 + (n%4)*1229*8
    c2c1,c0 = struct.unpack_from('<II', d, o)
    b_ = c2c1 & 0xFFF
    print('%-8s R=%4d B=%4d G=%4d  B/R=%.3f' % (label,c0,b_,(c2c1>>16)&0xFFF,b_/max(c0,1)))

chk(16*289+16*17+16,'white')
chk(12*289+12*17+12,'bright')
chk(8*289+8*17+8,'gray')
chk(4*289+4*17+4,'dark')
