import re

_dco = re.compile(
    '^(([^ \t/]+)(/comm-objects)?/)?([^ \t.]+)\\.dco[ \t]*(#.*)?$')

testlines = """  MyProject/comm-objects/something.dco
Otherproject/mine.dco
me.dco
OtherProject dco
Stuff.dco  # and a comment
MyProject/comm-objects/Stuff.dco # and a comment"""

for t in testlines.split('\n'):

    res = _dco.match(t.strip())
    if res:
        print(res.groups())

    else:
        print("no match on ", t)