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

class FindPattern(PolicyCondition):
    
    # Determine how param arguments need to be stripped
    default_strip = dict(fileglob='both', pattern='both', resultvar='both',
                         label='both', limit='both')

    def __init__(self, fileglob: str, pattern: str, label: str='default', 
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
        self.fileglob, self.pattern = fileglob, pattern
        self.resultvar = resultvar
        self.label = label
        try:
            self.limit = int(limit)
        except ValueError:
            raise ValueError(
                f"{self.__class__.__name__}, cannont interpret 'limit' "
                f" from '{limit}'")

    def holds(self, p_path, **kwargs):

        # run and test
        result = dict()
        newvars = dict()

        testp = re.compile(self.pattern)
        matching = glob.glob(self.fileglob, recursive=False)

        dprint(f"Testing {matching}")
        for fn in matching:
            with open(fn, 'r') as tf:
                text = tf.read()
                mres = testp.search(text)
                dprint(f"pattern testing {fn}, result {mres}")

                # also record a "negative" result; this may be converted
                # by a not condition
                result[fn] = MatchReferenceFile(
                    module=fn.split(os.sep)[0],
                    fname=f'{p_path}/{fn}',
                    value=(mres is not None))

                if mres is not None:

                    offset, count = 0, 0
                    while mres is not None:
                        result[fn].addSpan(
                            MatchSpan(
                                span=(offset+mres.span()[0],
                                      offset+mres.span()[1]), 
                                count=count), self.label)
                        offset += mres.span()[-1]
                        count += 1
                        dprint(f'Found pattern {self.pattern} at {offset}')
                        mres = testp.search(text[offset:])

        checkAndSet(self.resultvar, newvars, list(result.values()))
        dprint(f"pattern setting {self.resultvar}, files: {len(result)}")
        return (len(result) > 0, (len(result) > 0 and [
             f'Found pattern {self.pattern} {len(val.matches)} times in file {fn}'
             for fn, val in result.items() ]) or
            [ 'FALSE: No pattern {self.pattern} found in {self.fileglob}'],
            newvars)

PolicyCondition.register('find-pattern', FindPattern)


"""
test = re.compile('^find this')

res = test.search('there is a string with find this in it')
print(res)
"""