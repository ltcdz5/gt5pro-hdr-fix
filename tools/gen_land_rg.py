#!/usr/bin/env python3
"""B 已顶格时，改为压 R/G 高光来平衡白点；中段保持现状。"""
import struct, sys

RG_TOP = float(sys.argv[1])    # 高光 R/G 压缩 (0.90 = 压 10%)
OUT    = sys.argv[2]

SRC = '/mnt/f/工作区/_tmp/land_bl1.bin'   # 上一版（中段已准）
COL_OFF = 204
GRID = 17

def rg_curve(x, top):
    """只在接近高光时压缩，中段不动"""
    # x=0.5 以下不动，0.5->1 逐渐压到 top
    if x <= 0.5:
        return 1.0
    t = (x - 0.5) / 0.5
    return 1.0 + (top - 1.0) * (t ** 1.5)

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

            R = R * rg_curve(R, RG_TOP)
            G = G * rg_curve(G, RG_TOP)
            R = max(0.0,min(1.0,R)); G = max(0.0,min(1.0,G))

            struct.pack_into('<II', d, o,
                             (int(round(G*4095))<<16)|int(round(B*4095)),
                             int(round(R*4095)))

open(OUT,'wb').write(d)
print('built',OUT,'rg_top',RG_TOP)

def chk(n,label):
    o = COL_OFF + (n//4)*8 + (n%4)*1229*8
    c2c1,c0 = struct.unpack_from('<II', d, o)
    b_ = c2c1 & 0xFFF
    print('%-8s R=%4d B=%4d G=%4d  B/R=%.3f' % (label,c0,b_,(c2c1>>16)&0xFFF,b_/max(c0,1)))

chk(16*289+16*17+16,'white')
chk(12*289+12*17+12,'bright')
chk(8*289+8*17+8,'gray')
chk(4*289+4*17+4,'dark')
