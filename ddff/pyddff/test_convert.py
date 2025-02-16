from ddff_convert import parser
import os

pres = parser.parse_args(
    ("--verbose", "hdf5", "--outfile=/tmp/result.hdf5", 
    f"{os.environ['HOME']}/gdapps/VarStabCitation2/VarStabCitation2/run/solo/solo/simlog-20250216_084323.ddff")
)

pres.handler()(pres)
