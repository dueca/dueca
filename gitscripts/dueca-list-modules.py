#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Feb 22 15:38:47 2021

@author: repa
"""

import argparse
import sys

from duecautils.modules import Modules
from duecautils import verboseprint

parser = argparse.ArgumentParser(
    description="""List modules of a DUECA project
    """)

parser.add_argument(
    '--projectdir', type=str, required=True,
    help="Project folder")
parser.add_argument(
    '--machineclass', type=str,
    help="Project folder")
parser.add_argument(
    '--compact', action='store_true', 
    help="Compact listing output")
parser.add_argument(
    '--only-active', action='store_true',
    help="Only include modules that need compiling")
parser.add_argument(
    '--verbose', action='store_true',
    help='Verbose debug output')


ns = parser.parse_args(sys.argv[1:])
if ns.verbose:
    verboseprint._verbose_print = True

modules = Modules(ns.projectdir, ns.machineclass)

if ns.compact:
    print(modules.compactPrint(ns.only_active))
else:
    print(modules.debugPrint(ns.only_active))
