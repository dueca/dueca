from ddff_convert import parser
import os

pres = parser.parse_args(
    ("--verbose", "hdf5", "--outfile=/tmp/result.hdf5", 
    f"{os.environ['HOME']}/tmp/varstab/runlogs/2025-02-13_15:07:08/simlog-20250213_140803.ddff")
)

pres.handler()(pres)
