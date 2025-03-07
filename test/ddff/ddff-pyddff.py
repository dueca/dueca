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
ticks = [t for t in f1['WriteUnified:first blip'].time()]
rx = [x for x in f1['WriteUnified:first blip']['rx']]
ry = [x for x in f1['WriteUnified:first blip']['ry']]
assert(len(ticks) == len(rx))
ticks2 = [t for t in f1['WriteUnified:second blip'].time()]
rx2 = [x for x in f1['WriteUnified:second blip']['rx']]
ry2 = [x for x in f1['WriteUnified:second blip']['ry']]
assert(len(ticks) == len(ticks2))

# read as tagged
f2 = DDFFTagged(fname)
# only one tag
assert(len(f2.tags()) == 1)

periods = [ k for k in f2.tags().keys() ]
streamids = [ n for n in f2.keys() ]
for p in periods:
    for i in streamids:
        print("period", p, "stream", i)
        ticks = [t for t in f2[i].time(p)]
        rx = [x for x in f2[i][p,'rx']]
        ry = [y for y in f2[i][p,'ry']]

# the same with int index for the streams
for p in periods:
    for i in range(2):
        print("period", p, "stream", i)
        ticks = [t for t in f2[i].time(p)]
        rx = [x for x in f2[i][p,'rx']]
        ry = [y for y in f2[i][p,'ry']]

t0, t1, data = f2[0].getData()
