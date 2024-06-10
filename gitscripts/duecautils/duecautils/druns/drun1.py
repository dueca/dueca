import os
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent.parent))
print(str(Path(__file__).parent.parent.parent))
from duecautils.policy import Policies
import duecautils
duecautils.verboseprint._verbose_print = True

dpath = str(Path(__file__).parent.parent.parent.parent.parent)
tpath = f"{dpath}/build-linux/test/gitscript/"

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

for a, p in (("testarea", "TestProject1"),):

    MoveAbout.push(f"{tpath}/{a}/{p}/{p}")
    policies = Policies(f"{tpath}/{a}/{p}/{p}",
        False, [f'file://{dpath}/gitscripts/default/homedco.xml'], True)
    report = policies.inventory()
    print(report)
    MoveAbout.pop()
