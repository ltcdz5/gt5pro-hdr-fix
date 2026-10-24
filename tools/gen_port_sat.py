#!/usr/bin/env python3
"""竖屏：在已中性化基础上加饱和度，保持白点。"""
import struct, sys

SAT = float(sys.argv[1])
OUT = sys.argv[2]

SRC = '/mnt/f/工作区/_tmp/port_v2.bin'
COL_OFF = 204
GRID = 17

def to_lin(v):
    v = max(0.0, min(1.0, v))
    return v/12.92 if v <= 0.04045 else ((v+0.055)/1.055)**2.4

def to_srgb(v):
    v = max(0.0, min(1.0, v))
    return 12.92*v if v <= 0.0031308 else 1.055*(v**(1/2.4))-0.055

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

            lr, lg, lb = to_lin(R), to_lin(G), to_lin(B)
            lum = 0.2126*lr + 0.7152*lg + 0.0722*lb
            lr = lum + (lr-lum)*SAT
            lg = lum + (lg-lum)*SAT
            lb = lum + (lb-lum)*SAT
            R, G, B = to_srgb(lr), to_srgb(lg), to_srgb(lb)

            R = max(0.0,min(1.0,R)); G = max(0.0,min(1.0,G)); B = max(0.0,min(1.0,B))
            struct.pack_into('<II', d, o,
                             (int(round(G*4095))<<16)|int(round(B*4095)),
                             int(round(R*4095)))

open(OUT,'wb').write(d)
print('built',OUT,'sat',SAT)

def chk(n,label):
    o = COL_OFF + (n//4)*8 + (n%4)*1229*8
    c2c1,c0 = struct.unpack_from('<II', d, o)
    b_ = c2c1 & 0xFFF
    print('%-8s R=%4d B=%4d G=%4d  B/R=%.3f' % (label,c0,b_,(c2c1>>16)&0xFFF,b_/max(c0,1)))
chk(16*289+16*17+16,'white')
chk(8*289+8*17+8,'gray')
