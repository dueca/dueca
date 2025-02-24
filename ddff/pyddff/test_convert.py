from ddff_convert import parser
import os

# debug/test?
from src.ddffinventoried import DDFFInventoried
from src.ddfftagged import DDFFTagged


import numpy as np

dtype = np.dtype([("one", np.float64),("two", np.float32),("three", np.int32)])

d = np.array([(3, 2.2, 3), (4.0, 5.1, 0)], dtype=dtype)
d2 = np.zeros((2,3), dtype=dtype)
d2[0] = (1, 2, 3)

fname = f"{os.environ['HOME']}/tmp/recording-PHLAB-new.ddff"
#fname = f"{os.environ['HOME']}/tmp/varstab/runlogs/2025-02-13_15:07:08/simlog-20250213_140803.ddff"
# separate test of the readstream
#df = DDFFTagged(fname)

pres = parser.parse_args(
    ("--verbose", "info", "--streamid", "/data/servodetails", fname))
pres.handler()(pres)

pres = parser.parse_args(
    ("--verbose", "hdf5", "--streamids", "/data/servodetails", "--outfile=/tmp/result.hdf5", fname))
pres.handler()(pres)

# overview of contents
pres = parser.parse_args(
    ("--verbose", "info", fname))
pres.handler()(pres)

pres = parser.parse_args(
    ("--verbose", "info", "--period", "0", fname))
pres.handler()(pres)

pres = parser.parse_args(
    ("--verbose", "info", "--streamid", "/data/pilot", fname))
pres.handler()(pres)

