#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Feb 25 19:29:31 2021

@author: repa
"""

import re
import sys

_empty = re.compile('^[ \t]*$')
_comment = re.compile("^#(.*)$")
_dco = re.compile(
    '^([^ \t/]+)/comm-objects/([^ \t.]+)\\.dco[ \t]*([^ \t#]*).*$')

includelines = []
cmake_binary_dir = sys.argv[2]

with open(sys.argv[1], 'r') as f:
    for l in f:

        # discard empty lines and comment lines
        if not l.strip():
            includelines.append('\n')
            continue
        elif _comment.match(l.strip()):
            includelines.append(f'//{l.rstrip()}\n')
            continue

        # should match
        res = _dco.match(l.strip())
        if not res:
            print("Malformed line at comm-objects.lst file:"
                  f"{l}")
            continue

        project, dco = res.group(1), res.group(2)
        # includelines.append(
        #     f'#include "../../{project}/comm-objects/{dco}.hxx"\n')
        #includelines.append(
        #    f'#include <{dco}.hxx>\n')
        includelines.append(
            f'#include "{cmake_binary_dir}/{project}/comm-objects/{dco}.hxx"\n')

with open('comm-objects.h', 'w') as f2:
    f2.writelines(includelines)

# qprint(f"Generated comm-objects.h in {os.getcwd()}")