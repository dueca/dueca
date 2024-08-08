#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Aug 10 17:01:46 2021

@author: repa
"""
from .policycondition import ComplexCondition, PolicyCondition
from ..xmlutil import XML_interpret_bool
from ..verboseprint import dprint
import copy

def _combine_not(kwargs, inputvar, trim=False):
    """
    And-combination of condition test results. It currently always trims to
    the true values.

    Parameters
    ----------
    kwargs : dict
        Dict with test results.
    inputvar : str
        Name of the variable from the kwargs dict that need to be negated.

    Returns
    -------
    res : list of MatchReference
        Resulting combined variable indicating the "true" matches.

    """
    res = []
    dprint(f"Negation {inputvar}")

    if inputvar not in kwargs:
        raise ValueError(f"Required variable {inputvar} is not available,"
                         " fault in policy XML")

    # this tests the combinations of all inputvars values
    for inputs in kwargs.get(inputvar):

        mr = copy.copy(inputs)
        mr.value = not inputs.value

        if (not trim) or mr.value:
            res.append(mr)
    dprint(f"result negation {res}")
    return res

class ConditionNot(ComplexCondition):

    # Determine how param arguments need to be stripped
    default_strip = dict(trim='both', resultvar='both', inputvar='both')

    def __init__(self, **kwargs):
        super(ConditionNot, self).__init__(**kwargs)
        if len(self.subconditions) != 1:
            raise ValueError("NOT condition needs 1 subcondition")

        if 'resultvar' in kwargs:
            self.resultvar = str(kwargs['resultvar'])
        else:
            self.resultvar = None

        if (self.resultvar is not None) and \
            (len(self.inputvars) != 1):
            raise ValueError("NOT condition with output needs one inputvar")
        self.trim = XML_interpret_bool(str(kwargs.get('trim', "false")))


    def holds(self, **kwargs):
        motivation = ['NOT(']
        res, mot, newvars = self.subconditions[0].holds(**kwargs)
        motivation.extend(mot)
        motivation.append(')')

        if self.resultvar is not None:
            newvars[self.resultvar] = _combine_not(
                    newvars, self.inputvars[0], self.trim)

        # print('not', newvars)
        return (not res, motivation, newvars)

PolicyCondition.register("not", ConditionNot)
