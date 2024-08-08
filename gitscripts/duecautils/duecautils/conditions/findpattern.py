#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jun 30 20:56:53 2021

@author: repa
"""

from .policycondition import PolicyCondition, checkAndSet
from ..matchreference import MatchReferenceFile, MatchSpan
from ..verboseprint import dprint
import glob
import re
import os
from collections import defaultdict

def _empty_list():
    return list()

class MatchFunctionPattern:

    def __init__(self, pattern):

        self.pattern = re.compile(pattern)

    def __call__(self, text: str, span=None, fpath='unknown file'):

        if span is None:
            offset = 0
        else:
            offset = span[1]
        res = self.pattern.search(text, offset)
        return res and res.span(), res


class FindPattern(PolicyCondition):

    matchresult = MatchReferenceFile

    # Determine how param arguments need to be stripped
    default_strip = dict(fileglob='both', pattern='both', resultvar='both',
                         limit='both')

    def __init__(self, fileglob: str, pattern: str,
                 resultvar=None, limit=0, **kwargs):
        """
        Check for a pattern in the indicated files.

        Parameters
        ----------
        fileglob : str
            Glob pattern indicating which files should be checked.
        pattern : str
            Regular expression pattern.
        resultvar : str
            Result variable name. Details of the check Will be passed on
            to remaining checks and actions.
        **kwargs : dict
            Remaining, unused variables.

        Returns
        -------
        None.

        """
        self.fileglob, self.pattern = str(fileglob), str(pattern)
        self.resultvar = str(resultvar)
        try:
            self.limit = int(str(limit))
        except ValueError:
            raise ValueError(
                f"{self.__class__.__name__}, cannont interpret 'limit' "
                f" from '{limit}'")

    def holds(self, p_path, **kwargs):

        # run and test
        result = []
        newvars = defaultdict(_empty_list)

        testp = re.compile(self.pattern)
        matching = glob.glob(self.fileglob, recursive=False)

        dprint(f"Testing {matching}")
        for fn in matching:
            res = MatchReferenceFile(MatchFunctionPattern(testp), fn, self.limit)
            if res.value:
                result.append(res)


        checkAndSet(self.resultvar, newvars, result)
        dprint(f"pattern setting {self.resultvar}, files: {len(result)}")
        return (result, map(self.__class__.matchresult.explain, result), newvars)

PolicyCondition.register('find-pattern', FindPattern)


"""
test = re.compile('^find this')

res = test.search('there is a string with find this in it')
print(res)
"""