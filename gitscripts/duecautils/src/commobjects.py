#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Apr 30 19:59:02 2021

@author: repa
"""

import os
import re
from .verboseprint import dprint
import sys

_dcoline = re.compile(
    r'^[ \t]*([^/]+)/comm-objects/(.*).dco[ \t]*#*.*[ \t\n]*')
_commentline = re.compile(
    r'^[ \t]*#.*[ \t\n]*$')
'''
res = _dcoline.fullmatch(' Base/comm-objects/MyObject.dco  # comment')
print(res.group(0), res.group(1), res.group(2))
res = _commentline.fullmatch("blabla #comment")
'''



class CommObjectDef:

    def __init__(self, line=None, project=None, dco=None, comment=''):

        if line is None and project is not None and dco is not None:
            line = f'\n{project}/comm-objects/{dco}.dco{comment and "  # " + comment}\n'
        self._line = line
        self.base_project = None
        self.dco = None
        if line.strip() == '':
            return
        if _commentline.fullmatch(line):
            # dprint(f"DCO decode comment line {line}")
            return
        try:
            res = _dcoline.fullmatch(line)
            self.base_project = res.group(1).strip()
            self.dco = res.group(2).strip()
            # dprint(f"DCO line {line} decoded as {self}")
        except:
            raise Exception(
                f'cannot decode {line} as dco specification or comment')

    def __eq__(self, other):
        return self.base_project == other.base_project and \
            self.dco == other.dco

    def __lt__(self, other):
        if self.base_project < other.base_project:
            return True
        elif self.base_project > other.base_project:
            return False
        return self.dco < other.dco

    def __hash__(self):
        return f'{self.base_project}/{self.dco}'

    def __str__(self):
        return f'{self.base_project}/{self.dco}'

    def line(self):
        return self._line

    def exists(self, base):
        return os.path.isfile(
            f'{base}/{self.base_project}/comm-objects/{self.dco}.dco')

    def remove(self, comment):
        self.base_project = None
        self.dco = None
        self._line = f'#{self._line.rstrip()} #  {comment}\n'


class CommObjectsListIterator:
    def __init__(self, colist):
        self._iter = iter(colist.dco)

    def __next__(self):
        elem = next(self._iter)
        while elem.base_project is None:
            elem = next(self._iter)
        return elem


class CommObjectsList:

    def __init__(self, path:str):

        elts = path.split(os.sep)
        self.project = elts[-2]
        self.module = elts[-1]
        self.fname = f'{path}/comm-objects.lst'
        self.clean = None
        self._sync()

    def contains(self, project, dco):
        # dprint(list(map(str, self.dco)), CommObjectDef(None, project, dco))
        return CommObjectDef(None, project, dco) in self.dco


    def doubles(self):
        found = set()
        for d in self.dco:
            if (d.base_project, d.dco) in found:
                return True
            found.add((d.base_project, d.dco))
        return False

    def add(self, project, dco, comment):
        if self.contains(project, dco):
            print(f'Not adding, {self.fname} already contains '
                  f'{project}/comm-objects/{dco}.dco', file=sys.stderr)
            return
        self.dco.append(CommObjectDef(None, project, dco, comment))
        self.clean = False

    def delete(self, project, dco, comment):
        searchfor = CommObjectDef(None, project, dco)
        try:
            todelete = self.dco.index(searchfor)
            self.dco[todelete].remove(comment)
            self.clean = False
        except ValueError:
            print(f'No delete, {self.fname} does not contain {str(searchfor)}',
                  file=sys.stderr)
        return

    def listProjects(self):
        res = set()
        for dco in self.dco:
            if dco.base_project is not None:
                res.add(dco.base_project)
        return list(res)

    def _sync(self):

        if self.clean:
            return

        if self.clean is None:
            # dprint("Reading", self.fname)
            with open(self.fname, 'r') as cf:
                self.dco = [CommObjectDef(line=l) for l in cf]
            self.clean = True
            # dprint("after reading", list(map(str, self.dco)))
            return

        # dirty, write now
        try:
            os.rename(self.fname, self.fname + '~')
        except FileNotFoundError:
            pass

        with open(self.fname, 'w') as cf:
            for l in self.dco:
                cf.write(l.line())
        self.clean = True

    def __iter__(self):
        return CommObjectsListIterator(self)
