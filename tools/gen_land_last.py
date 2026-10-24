#!/usr/bin/env python3
"""最后微调：R 全段压 4%（灰色实测 R 偏高 3.7%），B 保持顶格。"""
import struct, sys

R_PRESS = float(sys.argv[1])   # R 全段压缩 (0.96)
G_PRESS = float(sys.argv[2])   # G 全段压缩 (0.98)
OUT     = sys.argv[3]

SRC = '/mnt/f/工作区/_tmp/land_f1.bin'
COL_OFF = 204
GRID = 17

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

            R = max(0.0, min(1.0, R * R_PRESS))
            G = max(0.0, min(1.0, G * G_PRESS))

            struct.pack_into('<II', d, o,
                             (int(round(G*4095))<<16)|int(round(B*4095)),
                             int(round(R*4095)))

open(OUT,'wb').write(d)
print('built',OUT,'R',R_PRESS,'G',G_PRESS)

def chk(n,label):
    o = COL_OFF + (n//4)*8 + (n%4)*1229*8
    c2c1,c0 = struct.unpack_from('<II', d, o)
    b_ = c2c1 & 0xFFF
    print('%-8s R=%4d B=%4d G=%4d  B/R=%.3f' % (label,c0,b_,(c2c1>>16)&0xFFF,b_/max(c0,1)))

chk(16*289+16*17+16,'white')
chk(12*289+12*17+12,'bright')
chk(8*289+8*17+8,'gray')
chk(4*289+4*17+4,'dark')
