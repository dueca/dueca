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

fname = f"{os.environ['HOME']}/tmp/varstab/feb17/simlog-20250217_151129.ddff"
# separate test of the readstream
#df = DDFFTagged(fname)

#print(df.streams[2].getData())


pres = parser.parse_args(
    ("--verbose", "hdf5", "--outfile=/tmp/result.hdf5", fname))

pres.handler()(pres)
