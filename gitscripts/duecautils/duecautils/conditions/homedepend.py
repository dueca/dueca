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

class HomeDepend(PolicyCondition):

    # type of result of a holds test
    matchresult = MatchReferenceFile

    # Determine how param arguments need to be stripped
    default_strip = dict(resultvar='both', label='both')

    def __init__(self, label: str='default',
                 resultvar=None, limit=0, **kwargs):
        """
        Check for a pattern in the indicated files.

        Parameters
        ----------
        resultvar : str
            Result variable name. Details of the check Will be passed on
            to remaining checks and actions.
        **kwargs : dict
            Remaining, unused variables.

        Returns
        -------
        None.

        """
        self.resultvar = str(resultvar)
        self.label = str(label)
        try:
            self.limit = int(str(limit))
        except ValueError:
            raise ValueError(
                f"{self.__class__.__name__}, cannont interpret 'limit' "
                f" from '{limit}'")

    def holds(self, p_path, **kwargs):

        # run and test
        result = dict()
        newvars = dict()

        startp = re.compile('USEMODULES')
        stopp = re.compile(r'SOURCES|INCLUDEDIRS|DUECA_COMPONENTS|LIBRARIES|COMPILEOPTIONS|COMPILEOPTIONS_PUBLIC|INCLUDEDIRS_PUBLIC|\)')
        pattern = re.compile(r'^\s*([a-zA-Z0-9-_]+/)?([a-zA-Z0-9-_]+)\s*(#.*)?$')

        matching = glob.glob('*/CMakeLists.txt', recursive=False)


        dprint(f"Testing {matching}")
        res = []
        for fn in matching:
            res.append(MatchReferenceFile(fname=f'{p_path}/{fn})))
            with open(fn, 'r') as tf:
                text = tf.read()
                tstart = startp.search(text)
                if tstart:
                    tend = stopp.search(text, tstart.span()[1])
                istart = tstart.span()[1]
                m = True
                while istart < tend.span()[0] and m:
                    m = pattern.search(text, istart, tend.span()[0])
                    if m:
                        res.append(Match)
                dprint(f"pattern testing {fn}, result {mres}")

                # also record a "negative" result; this may be converted
                # by a not condition
                res.append(MatchReferenceFile(
                    module=fn.split(os.sep)[0],
                    fname=f'{p_path}/{fn}',
                    value=(mres is not None)))

                if mres is not None:

                    offset, count = 0, 0
                    while mres is not None:
                        result[-1].addSpan(
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
        return (result, map(self.__class__.matchresult.explain, result), newvars)

PolicyCondition.register('find-pattern', FindPattern)


"""
test = re.compile('^find this')

res = test.search('there is a string with find this in it')
print(res)

pattern = re.compile(r'([a-zA-Z0-9-_]+/)?([a-zA-Z0-9-_]+)(\ws)?(#.*)?(\ws)?')

res = pattern.match('  Aproject/a-module')
res = pattern.search('  Aproject/a-module # and comment ')

"""