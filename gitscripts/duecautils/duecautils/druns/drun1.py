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

tpath = os.getenv('HOME', '/home/repa')
for a, p in (
    ('gdapps', 'BLETest'),
    ('gdapps', 'WorldView')
        ):
    # ensure we are not re-using the previous project's repo instance
    try:
        del duecautils.modules.ProjectRepo.instance
    except:
        pass

    MoveAbout.push(f"{tpath}/{a}/{p}/{p}")
    policies = Policies(f"{tpath}/{a}/{p}/{p}",
        False, [f'file://{dpath}/gitscripts/default/homedco.xml'], True)
    report = policies.inventory()
    print(report)
    MoveAbout.pop()

