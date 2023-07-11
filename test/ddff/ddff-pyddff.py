from pyddff import DDFF, DDFFInventoried, DDFFTagged

import os

fname = os.path.dirname(__file__)+'/recordings-PHLAB-new.ddff'

# test simple DDFF; this file should have 4 streams
f0 = DDFF(fname)
assert(len(f0.streams)==4)

# read as inventoried; stream0 is the inventory, then streams 2 and 3 are
# data
f1 = DDFFInventoried(fname)
print(f1.keys())
assert(len(f1.keys()) == 2)
ticks = [t for t in f1.time('WriteUnified:first blip')]
rx = [x for x in f1['WriteUnified:first blip','rx']]
ry = [x for x in f1['WriteUnified:first blip','ry']]
assert(len(ticks) == len(rx))
ticks2 = [t for t in f1.time('WriteUnified:second blip')]
rx2 = [x for x in f1['WriteUnified:second blip','rx']]
ry2 = [x for x in f1['WriteUnified:second blip','ry']]
assert(len(ticks) == len(ticks2))

# read as tagged
f2 = DDFFTagged(fname)
# only one tag
assert(len(f2) == 1)

periods = [ k for k in f2.keys() ]
streamids = [ n for n in f2.inventory().keys() ]
for p in periods:
    for i in streamids:
        print("period", p, "stream", i)
        ticks = [t for t in f2.time(p, i)]
        rx = [x for x in f2[p,i,'rx']]
        ry = [y for y in f2[p,i,'ry']]

# the same with int index for the streams
for p in periods:
    for i in range(2):
        print("period", p, "stream", i)
        ticks = [t for t in f2.time(p, i)]
        rx = [x for x in f2[p,i,'rx']]
        ry = [y for y in f2[p,i,'ry']]
