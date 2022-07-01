#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Feb  4 20:47:22 2021

@author: repa
"""

import git
import tempfile
import os

rdir = tempfile.mkdtemp(suffix='.git')
print(f"remote dir in {rdir}")
gr = git.Repo.init(rdir, bare=True)

tdir = tempfile.mkdtemp()
print(f"git local dir in {tdir}")
gl = git.Repo.init(tdir)
with open(f'{tdir}/test1', 'w') as f:
    f.writelines(('oneline\n',))
with open(f'{tdir}/test2', 'w') as f:
    f.writelines(('oneline\n',))
os.mkdir(f'{tdir}/dir1')
with open(f'{tdir}/dir1/test3', 'w') as f:
    f.writelines(('oneline\n',))

# add a file
print(gl.untracked_files)
for f in gl.untracked_files:
    gl.index.add(f)
gl.index.commit("initial creation of project")

# add the remote
origin = gl.create_remote('origin', f'file://{rdir}')
# push
gl.git.push('--set-upstream', 'origin', 'master')

# create a second checkout
tdir2 = tempfile.mkdtemp()
print(f"git local dir in {tdir2}")
gl = git.Repo.init(tdir2)

# shallow clone
ge = git.Repo.init(tdir2)
ge.create_remote('origin', f'file://{tdir}')
ge.git.config('core.sparseCheckout', 'true')

# set the config request
with open(f'{tdir2}/.git/info/sparse-checkout', 'w') as sc:
    sc.write('test1\n')

ge.remote().pull('master')
print(ge.remote().url)

# export a part
tf = tempfile.mkstemp(suffix='.tar')
ge.git.archive('--format=tar', f'--remote={tdir}',
               f'HEAD:dir1/',
               f'--prefix="newname"',
               f'--output={tf[1]}')
print(tf)
