#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Jul  1 13:33:02 2021

@author: repa
"""

"""
Match results, to be returned

- pattern found in a file:
    filename -> matched file
    value -> true if any match was found
    module -> module where file was found (if applicable)
    matches -> spans where the pattern was found

- dco used in a dco list:
    filename -> the dco comm-objects.lst
    value -> true if any match was found
    module -> module that uses the list (where located)
    module_project -> project where module located (always home project)
    dco -> list of lines in dco list, either empty, comment or dco definition
           if value true, is a match to a criterion (uses-dco or home-dco)

- module borrowed or owned in a platform:
    filename -> modules.xml file
    value -> true if any match was found
    machine_class -> machine class
    module -> module name
    module_project -> module parent project

"""
from collections import defaultdict
from .commobjects import CommObjectsList
from .modules import Modules


class MatchSpan:

    def __init__(self, line: int=0,
                 span=None, count=0, label=''):
        self.line = line
        self.span = span
        self.count = count

class MatchReference:

    def __init__(self, value=True):

        self.value = value

    def __repr__(self):
        res = [ f'{self.__class__.__name__}' ]
        #for k, mem in dict(m='module', mp='module_project', d='dco',
        #                   dp='dco_project', mc='machine_class').items():
        for k, val in self.__dict__.items():
            #if mem in self.__dict__:
            #    res.append(f', {k}:{self.__dict__[mem]}')
            res.append(f", {k}:{val}")
        #if 'match' in self.__dict__:
        #    res.append(f", n:{len(self.matches)}")
        res.append(')')
        return ''.join(res)

class MatchReferenceDco(MatchReference):

    def __init__(self, matchFunction,
                 commobjects: CommObjectsList):
        self.matchFunction = matchFunction
        self.commobjects = commobjects
        value = bool([c for c in commobjects if matchFunction(c.base_project, c.dco)])

        super(MatchReferenceDco, self).__init__(value)

    def explain(self):
        res = [ f'For {self.commobjects.fname}:' ]
        if self.value:
            res.extend([ self.matchFunction.explain(c.base_project, c.dco)
                         for c in self.commobjects
                         if self.matchFunction(c.base_project, c.dco) ])
        else:
            res.append(self.matchFunction.explain())

        return '\n'.join(res)


class MatchReferenceModule(MatchReference):

    def __init__(self, matchFunction, modules: Modules):
        self.matchFunction = matchFunction
        self.modules = module
        self.machine_class = machine_class
        super(MatchReferenceModule, self).__init__(value)

"""
    def __repr__(self):
        return f"MatchReferenceModule(m:{self.module}, f:{self.filename}, " \
            f"p:{self.module_project}, mc:{self.machine_class}, v:{self.value})"
"""

class MatchReferenceFile(MatchReference):
    """Match on a file (typically given by a pattern)

    Parameters
    ----------
    MatchReference : _type_
        _description_
    """
    matchon = set(('filename',))

    def __init__(self, fname: str, value: bool=True):

        # function for dict
        def _empty_list():
            return list()

        self.matches = defaultdict(_empty_list)
        super(MatchReferenceFile, self).__init__(fname, value)

    def addSpan(self, det: MatchSpan, label: str):
        self.matches[label].append(det)

"""
    def __repr__(self):
        return f"MatchReferenceFile(m:{self.module}, f:{self.filename}, " \
            f"n:{len(self.matches)} v:{self.value})"
"""

def trimList(l: list):
    return [ e for e in l if e.value ]

def anyTrue(l: list):
    res = False
    for e in l:
        res = res or l.value
    return res

def doubleFile(l: list, varname):
    files = set()
    for e in l:
        if e.value and e.filename in files:
            raise ValueError(f'file {e.filename} multiple occurrence in {varname}')
    return None
