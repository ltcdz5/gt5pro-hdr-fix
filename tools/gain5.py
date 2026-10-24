import struct, sys
# 每通道: gain(x) = a + b*(x/4095)
AR,BR,AG,BG,AB,BB,OUT = [float(x) for x in sys.argv[1:7]] + [sys.argv[7]]
d=bytearray(open('/mnt/f/工作区/_tmp/ghdr4.bin','rb').read())
off=204
def adj(v,a,b):
    g=a+b*(v/4095.0)
    g=max(0.0,min(1.5,g))
    return max(0,min(4095,int(round(v*g))))
for k in range(4*1229):
    o=off+k*8
    c2c1,c0=struct.unpack_from('<II',d,o)
    c1=c2c1 & 0xFFF  # B
    c2=(c2c1>>16)&0xFFF  # G
    c0=adj(c0,AR,BR)
    c1=adj(c1,AB,BB)
    c2=adj(c2,AG,BG)
    struct.pack_into('<II',d,o,(c2<<16)|c1,c0)
open(OUT,'wb').write(d)
print('built',OUT,'R',AR,BR,'G',AG,BG,'B',AB,BB)