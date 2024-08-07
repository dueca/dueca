#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Aug 10 15:47:33 2021

@author: repa
"""

from .policycondition import ComplexCondition, PolicyCondition
from ..xmlutil import XML_interpret_bool
from ..verboseprint import dprint
from ..matchreference import MatchReference
import itertools
import copy
from collections import defaultdict


def _combine_iterable(l1, l2):
    return l1 + l2

def _combine_dict(d1, d2):
    res = copy.copy(d1)
    res.update(d2)
    return res

def _combine_first():
    def _combine_first(r1, r2):
        return r1
    return _combine_first

_funmapping = defaultdict(_combine_first)
_funmapping.update({  str: _combine_iterable,
                    list: _combine_iterable,
                    dict: _combine_dict,
                    int: _combine_iterable })


def _combine_elts(inputvars, selection, ekey, inputs):

    try:
        # combine from multiple?
        eitlist = list(map(str.strip, selection.split(',')))
        idx = inputvars.index(eitlist[0])
        res = copy.copy(inputs[idx].__dict__[ekey])
        cfun = _funmapping[inputs[idx].__dict__[ekey].__class__]

        for eit in eitlist[1:]:
            idx = inputvars.index(eit)
            res = cfun(res, inputs[idx].__dict__[ekey])
            # res.update(inputs[idx].__dict__[ekey])
        return res
    except Exception as e:
        raise ValueError(f"Cannot transfer/combine property {ekey}, error {e}")

def _combine_and(kwargs, inputvars, matchelts, resultelts, trim):
    """
    And-combination of condition test results. It currently always trims to
    the true values.

    Parameters
    ----------
    kwargs : dict
        Dict with test results.
    inputvars : list of str
        All variables from the kwargs dict that need to be combined.
    matchelts : list of str
        What should be matched in the combination; consists of the possible
        members in a MatchReference object; typically "module"
        (same module name), "module_project" (project donating the module),
        "dco" (same dco object), "dco_project" (project donating the dco)
    resultelts : dict of str
        Keys in the resultelts dict give the variables in the combined result,
        the associated values indicate which inputvariable from the kwargs
        supplies that value.
    trim : bool
        If true, produce a result with only "true" valued matches, otherwise
        produce all.

    Raises
    ------
    e
        Exception, typically when data members are not correctly specified.

    Returns
    -------
    res : list of MatchReference
        Resulting combined variable indicating the "true" matches.

    """
    res = []
    dprint(f"And combining {inputvars}")

    # this tests the combinations of all inputvars values
    for inputs in itertools.product(*map(kwargs.get, inputvars)):

        # inputs is now a tuple of elements from the input variables.
        # check for a match
        matching = True
        value = True
        matchresult = {}
        for elt in matchelts:
            i0 = inputs[0]
            eltval = i0.__dict__.get(elt, None)
            for i in inputs[1:]:

                if eltval is None:
                    eltval = i.__dict__.get(elt, None)

                if (i.__dict__.get(elt, None) is not None) and \
                    eltval != i.__dict__[elt]:
                    matching = False
                    dprint(f"No match between {i0} and {i} on {elt}"
                           f" {eltval} != {i.__dict__[elt]}")
                else:
                    value = value and i.value
            matchresult[elt] = eltval

        if matching and (value or (not trim)):
            mr = MatchReference(value=value)

            # the matching keys are inserted by default
            for k, v in matchresult.items():
                mr.__dict__[k] = v
                dprint(f"matched all {k} to {v}")

            # add other results as defined in result-.... values
            for ekey, eit in resultelts.items():
                mr.__dict__[ekey] = _combine_elts(
                    inputvars, eit, ekey, inputs)
                dprint(f"Setting {ekey} on new match from {eit}")

            '''
            try:
                for ekey, eit in resultelts.items():
                    idx = inputvars.index(eit)
                    mr.__dict__[ekey] = inputs[idx].__dict__[ekey]
                    dprint(f"Setting {ekey} on new match from {eit}")
            except IndexError as e:
                print(f"index {idx}, on {inputs}")
                raise e
            except Exception as e:
                print(f"trying to set values, member {ekey}, from var {eit} with vars {kwargs}: {e}")
                raise e
            '''
            res.append(mr)

    dprint(f"result and combination {res}")
    return res

class ConditionAnd(ComplexCondition):

    # Determine how param arguments need to be stripped
    default_strip = dict(trim='both', match='both',
                         resultvar='both', inputvar='both')

    def __init__(self, _match='', **kwargs):
        """
        Create an 'and' combination of conditions

        Parameters
        ----------
        match : str, optional
            Comma-separated list of elements to create the 'and' match on.
            Common choices are
            'module': the file matched or condition matched refer to the
                      same module
            'module_project': the module match is from the same parent project
            'dco': match on the same DCO object
            'dco_project': match on the DCO's parent project
            'filename': refers to the same file
            The default is ''.
        **kwargs : dict of str
            parameter arguments for the combination. Parameters interpreted
            by the And combination are:
            'trim': in a result variable, only pass matches with true result
            'result-*': indicate where elements of the result variable come
            from. E.g., an entry with kwargs['result-module'] = 'pattermatch'
            will set the module value from the combined result to the
            module value found in the 'patternmatch' variable produced by
            one of the condition's components.
            The entries 'resultvar' and 'inputvar' will be interpreted by
            the ComplexCondition parent.

        Returns
        -------
        None.

        """
        _match = str(_match)
        self.matchelts = list(map(str.strip, _match.split(',')))
        self.resultelts = {}
        if 'resultvar' in kwargs:
            self.resultvar = str(kwargs['resultvar'])
        else:
            self.resultvar = None
        for key, val in kwargs.items():
            if key.startswith('result-'):
                dprint(f"result element {key}, value {val}")
                self.resultelts[key[len('result-'):]] = str(val).strip()
        self.trim = XML_interpret_bool(str(kwargs.get('trim', 'false')))
        super(ConditionAnd, self).__init__(**kwargs)


    def holds(self, **kwargs):
        motivation = ['AND(']
        newvars = dict()
        _res = True

        for c in self.subconditions:
            res, mot, _nv = c.holds(**kwargs, **newvars)
            _res = _res and bool(res)

            newvars.update(_nv)
            motivation.extend(mot)

        if self.resultvar is not None:
            dprint(f'inputvars: {self.inputvars}')
            if _res :
                newvars[self.resultvar] = _combine_and(
                    newvars, self.inputvars, self.matchelts,
                    self.resultelts, self.trim)

                # update result from match
                _res = len([nv for nv in newvars[self.resultvar]
                            if nv.value])
            else:
                newvars[self.resultvar] = []

        motivation.append(')')
        dprint('and', _res, newvars)
        return (_res, motivation, newvars)

PolicyCondition.register("and", ConditionAnd)
