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
    module -> module where file was found (if applicable)
    spans where the pattern was found

- dco used in a dco list:
    filename -> the dco list
    module -> module that uses the dco
    dco -> dco name
    dco_project -> dco parent project

- module borrowed or owned in a platform:
    filename -> modules.xml file
    machine_class -> machine class
    module -> module name
    module_project -> module parent project

"""
from collections import defaultdict


class MatchSpan:

    def __init__(self, line: int=0,
                 span=None, count=0, label=''):
        self.line = line
        self.span = span
        self.count = count

class MatchReference:

    def __init__(self, fname: str='', value=True):

        self.filename = fname
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

    def __init__(self, dco: str, dco_project: str,
                 module: str, module_project: str,
                 fname: str, value: bool=False):
        self.dco = dco
        self.dco_project = dco_project
        self.module = module
        self.module_project = module_project
        super(MatchReferenceDco, self).__init__(fname, value)

"""
    def __repr__(self):
        return f"MatchReferenceDCO(m:{self.module}, f:{self.filename}, " \
            f"p:{self.module_project}, d:{self.dco}, dp:{self.dco_project})"""

class MatchReferenceModule(MatchReference):

    def __init__(self, module: str, module_project: str,
                 machine_class: str, fname: str, value: bool=False):
        self.module = module
        self.module_project = module_project
        self.machine_class = machine_class
        super(MatchReferenceModule, self).__init__(fname, value)

"""
    def __repr__(self):
        return f"MatchReferenceModule(m:{self.module}, f:{self.filename}, " \
            f"p:{self.module_project}, mc:{self.machine_class}, v:{self.value})"
"""

class MatchReferenceFile(MatchReference):

    def __init__(self, module: str, fname: str, value: bool=True):
        self.module = module

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
