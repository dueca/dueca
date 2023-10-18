#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Mar 24 12:26:46 2023

@author: repa
"""
import argparse
import os
import sys
import subprocess
from datetime import date
from collections import ChainMap
from duecautils.schemereader import readscheme
from duecautils.schemereader.readmod import _pool, new_mod_header

parser = argparse.ArgumentParser(
    description=
""" Convert a scheme-based configuration to Python

This finds the dueca.cnf files and dueca.mod file in one platform, and
tries to convert these to a Python configuration.

- The dueca.cnf files are read for their characteristic parameters (
  number of nodes, priority, ip addresses and the like), and the
  result is used to populate new dueca_cnf.py files based on the
  template. New style communication is used.

- The dueca.mod files are read and parsed as much as possible, and written
  on the basis of the parsed scheme code. Note that this is not perfect,
  only the most common structures for dueca.mod configuration are recognized.
  Particularly the formatting and comments might have to be corrected
  to get a nicely readable file.

  New dueca_cnf.py and dueca_mod.py files are written. When happy with
  these, add them to your repository, and do not forget to change the
  selected language in the main CMakeLists.txt. When you have configured
  multiple platforms, convert each one by one.
  """)

parser.add_argument(
    'platform', type=str, default='', nargs='?',
    help="Name of the platform, or file location")

parser.add_argument(
    '--file', type=str,
    help="Alternative run mode, convert a single (helper) file"
)

parser.add_argument(
    '--verbose', action='store_true',
    help='Verbose output')

parser.add_argument(
    '--overwrite', action='store_true',
    help='Overwrite any existing dueca_mod.py or dueca_cnf.py files')

parser.add_argument(
    '--master', type=str, default='correct this value',
    help="Communication master")

runargs = parser.parse_args(sys.argv[1:])

_verbose = runargs.verbose
def dprint(*args, **kwargs):
    if _verbose:
        print(*args, **kwargs)

# find a project folder
class OnExistingProject():
    def __init__(self, *args, **kwargs):

        # to remember push and pops
        self.dirpath = []

        # figure out the projectdir and project name
        curpath = os.getcwd().split(os.sep)
        self.inprojectdir = True
        while len(curpath):
            projectdir = '/'.join(curpath)
            if os.path.exists(f'{projectdir}/.config/machine') and \
                os.path.exists(f'{projectdir}/CMakeLists.txt') and \
                os.path.isdir(f'{projectdir}/run') and \
                os.path.isdir(f'{projectdir}/.git'):
                    break
            del curpath[-1]
            self.inprojectdir = False

        if len(curpath) < 2 or curpath[-1] != curpath[-2]:
            print(f"Could not find project folder in {os.getcwd()}",
                  file=sys.stderr)
            raise Exception("dueca-scheme-to-python cannot find"
                            " the main project directory")

        self.project = curpath[-1]
        self.projectdir = projectdir

    def pushDir(self, pdir=None):
        self.dirpath.append(os.getcwd())
        os.chdir(pdir or self.projectdir)

    def popDir(self):
        os.chdir(self.dirpath[-1])
        del self.dirpath[-1]

# for generic use, DUECA files location
dc = subprocess.run(("dueca-config", "--path-datafiles"),
                    stdout=subprocess.PIPE)
duecabase = dc.stdout.strip().decode('UTF-8') + \
    os.sep + "data" + os.sep + "default" + os.sep

def canWrite(fname, overwrite=False):
    if os.path.exists(fname):
        if overwrite:
            for i in range(1000):
                ftry = fname + f'.bak{i:03d}'
                if not os.path.exists(ftry):
                    os.rename(fname, ftry)
                    break
    return not os.path.exists(fname)

def write_cnf_from_template(outfile, subst):
    with open(f"{duecabase}/dueca_cnf.py.in", 'r', encoding='utf-8') as fr:
        fdata = ''.join(fr.readlines())
    for k, v in subst.items():
        if f'@{k}@' in fdata:
            fdata = str(v).join(fdata.split(f'@{k}@'))
        else:
            dprint(f"Key '@{k}@' not found")
    if _verbose and fdata.count('@'):
        dprint(f"There are {fdata.count('@')} remaining @ signs")

    with open(outfile, 'w', encoding='utf-8') as fw:
        fw.write(fdata)

def get_dueca_version():
    dc = subprocess.run(("dueca-config", "--version"), stdout=subprocess.PIPE)
    return dc.stdout.strip().decode("UTF-8")

_dueca_cnf_defaults = {
    'this-node-id': 0,
    'no-of-nodes': 1,
    'send-order': 0,
    'highest-manager': 4,
    'run-in-multiple-threads?': True,
    'rt-sync-mode': 2,
    'graphic-interface': 'gtk3',
    'tick-base-increment': 100,
    'tick-compatible-increment': 100,
    'tick-time-step': 0.01,
    'communication-interval': 100,
    'if-address': "127.0.0.1",
    'mc-address': "224.0.0.1",
    'mc-port': 7500,
    'master-host': runargs.master,
    'packet-size': 4096,
    'bulk-max-size': 128*1024,
    'comm-prio-level': 3,
    'unpack-prio-level': 2,
    'bulk-unpack-prio-level': 1,
    'dueca-version': get_dueca_version(),
    'date': date.today().strftime("%d-%b-%Y"),
    }

if runargs.platform == '':
    try:
        out = runargs.file + '.py'
        if canWrite(out, runargs.overwrite):

            with open(runargs.file, 'r', encoding='utf-8') as f:
                res = readscheme.contents.parseFile(f)

            with open(out, 'w', encoding='utf-8') as f:
                for r in res:
                    print(r.convert(0, _pool), file=f)
            print(f"Converted to {out}")
        else:
            print(f"Not overwriting {out}")
    except Exception as e:
        print(f"Could not find or convert specific file '{runargs.file}'")
    sys.exit(0)

elif os.path.isdir(runargs.platform):
    dprint(f"Found folder {runargs.platform}")
    platform = runargs.platform
else:
    prj = OnExistingProject()
    platform = f'{prj.projectdir}/run/{runargs.platform}'
    if not os.path.isdir(platform):
        raise ValueError(
            f"Cannot find platform {runargs.platform} in {prj.projectdir}/run")
    dprint(f"Found platform {platform}")

# find the files to convert
cnffiles = []
modfiles = []
for d in os.listdir(platform):
        if os.path.isdir(f'{platform}/{d}'):
            dprint(f"Checking folder {d}")
            for f in os.listdir(f'{platform}/{d}'):
                dprint(f"Checking file {f}")
                if os.path.isfile(f'{platform}/{d}/{f}') and f == 'dueca.mod':
                    modfiles.append(f'{platform}/{d}/{f}')
                elif os.path.isfile(f'{platform}/{d}/{f}') and f == 'dueca.cnf':
                    cnffiles.append(f'{platform}/{d}/{f}')

dprint(f"to convert {cnffiles}, {modfiles}")
# do the conversion
for cnf in cnffiles:

    out = cnf[:-4] + '_cnf.py'
    if canWrite(out, runargs.overwrite):

        readscheme.clearValues()
        with open(cnf, 'r', encoding='utf-8') as f:
            res = readscheme.contents.parseFile(f)
        for r in res:
            if isinstance(r, readscheme.Expression):
                r.run()
        dprint("Found contents", readscheme.getValues())
        write_cnf_from_template(out, ChainMap(readscheme.getValues(),
                                              _dueca_cnf_defaults))
        print(f"Converted to {out}")
    else:
        print(f"Not overwriting {out}")

for mod in modfiles:

    out = mod[:-4] + '_mod.py'
    if canWrite(out, runargs.overwrite):

        with open(mod, 'r', encoding='utf-8') as f:
            res = readscheme.contents.parseFile(f)

        with open(out, 'w', encoding='utf-8') as f:
            print(new_mod_header, file=f)
            for r in res:
                print(r.convert(0, _pool), file=f)
        print(f"Converted to {out}")
    else:
        print(f"Not overwriting {out}")


