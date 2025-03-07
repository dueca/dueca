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
from .param import Param

_dcoline = re.compile(
    r'^\s*(([^ \t/]+)/)?([^ \t/]+)/([^ \t/.]+)\.dco\s*(#.*)?$')

_commentline = re.compile(r'^\s*#.*$')
'''
res = _dcoline.fullmatch(' Base/comm-objects/MyObject.dco  # comment')
print(res.group(0), res.group(1), res.group(2))
res = _commentline.fullmatch("blabla #comment")
'''

def noMatch(project, dco):
    return False

class CommObjectDef:
    """ Data from a single dco reference in a comm-objects.lst

        Can be either an empty or comment line, or a DCO reference
    """
    def __init__(self, line=None, project=None,
                 module=None, dco=None, comment='', ownproject=None):
        """Parse a line, or compose a new line

        Parameters
        ----------
        line : str, optional
            single line in comm-objects.lst, to be parsed. If none, a new
            line is composed from the project, dco and comment values
        project : str, optional
            project name, can be None, indicating home project
        dco : str, optional
            base name for dco object (without extension), by default None
        comment : str, optional
            comment string for the line, by default ''

        Raises
        ------
        ValueError
            If a dco line cannot be parsed
        """

        if line is None:
            if dco is not None:
                if module is None:
                    module = 'comm-objects'
                if project is None:
                    line = f'{module}/{dco}.dco{comment and "  # " + comment}\n'
                else:
                    line = f'{project}/{module}/{dco}.dco{comment and "  # " + comment}\n'
            else:
                line = f'{comment and "# " + comment}\n'
        self._line = line
        self.base_project = None
        self.module = 'comm-objects'
        self.dco = None
        if line.strip() == '':
            return
        if _commentline.match(line):
            # dprint(f"DCO decode comment line {line}")
            return
        try:
            res = _dcoline.match(line)
            self.base_project = res.group(2) or ownproject
            self.module = res.group(3)
            self.dco = res.group(4)
            # dprint(f"DCO line {line} decoded as {self}")
        except:
            raise ValueError(
                f'cannot decode {line} as dco specification or comment')

    def __eq__(self, other):
        return self.base_project == other.base_project and \
            self.module == other.module and \
            self.dco == other.dco

    def __lt__(self, other):
        if (self.base_project or '') < (other.base_project or ''):
            return True
        if (self.module or '') < (other.module or ''):
            return True
        return (self.dco or '') < (other.dco or '')

    def __hash__(self):
        return f'{self.base_project}/{self.module}/{self.dco}'

    def __str__(self):
        return f'{self.base_project}/{self.module}/{self.dco}'

    def line(self):
        return self._line

    def exists(self, base):
        return os.path.isfile(
            f'{base}/{self.base_project}/{self.module}/{self.dco}.dco')

    def remove(self, comment):
        self.base_project = None
        self.module = None
        self.dco = None
        self._line = f'#{self._line.rstrip()} #  {comment}\n'


class CommObjectsListIterator:
    def __init__(self, colist):
        self._iter = iter(colist.dco)

    def __iter__(self):
        return self

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

    def contains(self, project, dco, module='comm-objects'):
        # dprint(list(map(str, self.dco)), CommObjectDef(None, project, dco))
        return CommObjectDef(None, project, module, dco) in self.dco

    def matches(self, matchFunction):
        return [ d for d in self.dco
            if matchFunction(d.base_project, d.module, d.dco)]

    def doubles(self):
        found = set()
        for d in self.dco:
            if (d.base_project, d.dco) in found:
                return True
            found.add((d.base_project, d.module, d.dco))
        return False

    def add(self, project, module, dco, comment):
        if self.contains(project, module, dco):
            print(f'Not adding, {self.fname} already contains '
                  f'{project}/{module}/{dco}.dco', file=sys.stderr)
            return
        self.dco.append(CommObjectDef(None, project, module, dco, comment))
        self.clean = False

    def delete(self, project, module, dco, comment):
        searchfor = CommObjectDef(None, project, module, dco)
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
                self.dco = [CommObjectDef(line=l, ownproject=self.project)
                for l in cf]
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
