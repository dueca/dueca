from ddff_convert import parser
import os

import numpy as np

dtype = np.dtype([("one", np.float64),("two", np.float32),("three", np.int32)])

d = np.array([(3, 2.2, 3), (4.0, 5.1, 0)], dtype=dtype)
d2 = np.zeros((2,3), dtype=dtype)
d2[0] = (1, 2, 3)

pres = parser.parse_args(
    ("--verbose", "hdf5", "--outfile=/tmp/result.hdf5",
    f"{os.environ['HOME']}/gdapps/VarStabCitation2/VarStabCitation2/run/solo/solo/simlog-20250216_161100.ddff")
)

pres.handler()(pres)
