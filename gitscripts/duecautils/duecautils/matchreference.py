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

def _empty_list():
    return list()

class MatchSpan:

    def __init__(self, line: int=0,
                 span=None, count=0, matchre=None):
        self.line = line
        self.span = span
        self.count = count
        self.matchre = matchre

    def explain(self, label='default'):
        return f"Match in category {label}, l: {self.line}, {self.span[0]}-{self.span[1]} on {self.matchre}"


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

    @property
    def filename(self):
        return self.commobjects.fname


class MatchReferenceModule(MatchReference):

    def __init__(self, matchFunction, modules: Modules):
        """Create a reference match of matchine modules

        Parameters
        ----------
        matchFunction : function
            Function to filter project/module combinations from a list
        modules : Modules
            Modules object with the projec/module combinations
        """
        self.matchFunction = matchFunction
        self.modules = modules
        value = bool([m for m in modules if matchFunction(m['project'], m['module'])])
        super(MatchReferenceModule, self).__init__(value)

    def explain(self):
        if self.value:
            res = [ f"For {self.modules.fname}:" ]
            res.extend([ self.matchFunction.explain(m['project'], m['module'])
                for m in self.modules
                if self.matchFunction(m['project'], m['module']) ])
            return '\n'.join(res)
        return f'No match found in {self.modules.fname}'

    @property
    def filename(self):
        return modules.fname


class MatchReferenceFile(MatchReference):
    """Match on a file (typically given by a pattern)
    """
    matchon = set(('filename',))

    def __init__(self, matchFunction, fname: str, limit = 0):
        """_summary_

        Parameters
        ----------
        fname : str
            _description_
        value : bool, optional
            _description_, by default True

        Returns
        -------
        _type_
            _description_
        """

        self.matches = list()
        self.filename = fname
        with open(fname, 'r') as tf:
            text = tf.read()
            span, match = matchFunction(text, fpath=fname)
            while match and (limit == 0 or len(self.matches) < limit):
                self.matches.append(
                    MatchSpan(span=span, matchre=match))
                span, match = matchFunction(text, span, fpath=fname)

        super(MatchReferenceFile, self).__init__(bool(self.matches))

    def explain(self):
        res = [ f"For {self.filename}:" ]
        for m in self.matches:
            res.append(m.explain())
        return '\n'.join(res)


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
