# test gitscript_homedco
#
# Command: "/home/repa/dueca/gitscripts/dueca-gproject.py"
#          "--verbose" "policies" "--policiesurl"
#          "file:///home/repa/dueca/test/gitscript/homedco.xml"
#
#
# in        testarea/TestProject1

import os
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent.parent))
print(str(Path(__file__).parent.parent.parent))
from duecautils.policy import Policies
import duecautils
duecautils.verboseprint._verbose_print = True

dpath = str(Path(__file__).parent.parent.parent.parent.parent)
tpath = "/home/repa"

class MoveAbout:
    dirpath = []

    @classmethod
    def push(cls, ndir):
        cls.dirpath.append(os.getcwd())
        os.chdir(ndir)

    @classmethod
    def pop(cls):
        os.chdir(cls.dirpath[-1])
        del cls.dirpath[-1]

for a, p in (
    ("gdapps", "MotionSenseOptimalAccelerations"),):

    # ensure we are not re-using the previous project's repo instance
    try:
        del duecautils.modules.ProjectRepo.instance
    except:
        pass

    MoveAbout.push(f"{tpath}/{a}/{p}/{p}")
    policies = Policies(f"{tpath}/{a}/{p}/{p}",
        False, [f'file:///home/repa/cssoft/policies/common/hmilib/hmilib-as-module.xml'], True)
    report = policies.inventory()
    # policies.apply()
    print(report)

    MoveAbout.pop()
