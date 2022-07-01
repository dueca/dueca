#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Feb 18 15:30:24 2021

@author: repa
"""

import argparse
import sys
import os
import shutil
import re

parser = argparse.ArgumentParser(
    description="""Simple programmatic file editing.

    A target file will be modified by a file with modification.
    Three types of modification:

        - A: add lines after the current
        - R: replace code in current line
        - C: comment a line

    The argument after A, R, C can be numeric, then the line number will
    be found. If non-numeric, a string match will be tried, and finally a
    regex match.

    Following non-empty lines are used for replacement string, or addition
    string for A and R commands.

    R requires either a string or regex after the line number, or only
    a string/regex
    """)

parser.add_argument(
    '--target', type=str, required=True,
    help="File to be modified")
parser.add_argument(
    '--mods', type=str,
    help="File with modifications")
parser.add_argument(
    '--source', type=str,
    help="File to be copied")

ns = parser.parse_args(sys.argv[1:])

def free_backup_name(target):
    for i in range(1000):
        ntarget = f'{target}.bak{i:03d}'
        if not os.path.exists(ntarget):
            return ntarget


def run_command(lines, cmd, arg):

    line = None
    try:
        # line number?
        lno = int(cmd[1].split()[0])
        line = lines[lno - 1]
        pattern = cmd[1].lstrip()[len(str(lno)):].strip()

    except:

        # find with pattern
        pattern = cmd[1].strip()
        for l in lines:
            if pattern in l[0]:
                line = l
                break
        # last recourse, regex match
        if not line:
            reg = re.compile(pattern)
            for l in lines:
                if reg.search(l):
                    line = l
                    break
    if not line:
        raise ValueError(
            f"Unable to find line matching {cmd[1]}")



    if cmd[0].upper() == 'A':
        # append the argument lines
        line.extend(arg)

    elif cmd[0].upper() == 'R':



        # replace something in the line, use direct first, try regex later
        res = line[0].split(pattern)
        if len(res) == 1:
            res = reg.split(line[0])
        if len(res) > 1:
            line[0] = res.join('\n'.join(arg))
        else:
            raise ValueError(
                f"Unable to replace code on line {line[0]}")

    elif cmd[0].upper() == 'C':
        # comment this line
        line[0] = '//' + line[0]

    return True


if ns.source:

    # backup
    if os.path.exists(ns.target):
        shutil.move(ns.target, free_backup_name(ns.target))

    # simple copy
    shutil.copy(ns.source, ns.target)

else:

    otarget = free_backup_name(ns.target)
    shutil.move(ns.target, otarget)
    with open(otarget, 'r') as f0:
        lines = []
        for l in f0.readlines():
            lines.append([l.rstrip()])

    with open(ns.mods, 'r') as mods:

        empty = True
        cmd = None
        arg = []

        for i, l in enumerate(mods):

            # an empty or comment line
            if not l.split('#')[0].strip():
                # run command if command and argument are given
                if cmd and arg:
                    run_command(lines, cmd, arg)
                    cmd = None
                    arg = []

            else:
                if cmd is None:
                    # must be a command
                    l = l.strip()
                    if l[0] not in 'ARCarc':
                        raise ValueError(f'Unknown command at line {i}\n{l}')
                    cmd = (l[0], l[1:].strip())

                    # immediate
                    if cmd[0] in 'cC':
                        run_command(lines, cmd, arg)
                        cmd = None

                else:
                    # must be argument
                    arg.append(l)

    with open(ns.target, 'w') as f1:
        for line in lines:
            for l in line:
                f1.write(l + '\n')
