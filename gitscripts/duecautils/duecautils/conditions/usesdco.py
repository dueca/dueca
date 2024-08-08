#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun May  2 20:02:33 2021

@author: repa
"""

from .policycondition import PolicyCondition, checkAndSet
from ..matchreference import MatchReferenceDco
from ..param import Param
from .homedco import MatchFunctionDCO

class UsesDco(PolicyCondition):
    """ Check whether a certain project/dco combination is being used.
    """

    matchresult = MatchReferenceDco

    # Determine how param arguments need to be stripped
    default_strip = dict(project='both', dco='both', resultvar='both')

    def __init__(self, project: Param, dco: Param, resultvar=None, **kwargs):
        """
        Test whether a dco object is used in any of the project's own
        modules.

        Parameters
        ----------
        project : Param
            Name of the project supplying the dco object.
        dco : Param
            Dco objects (without .dco suffix).
        resultvar : str, optional
            Result variable name. Details of the check Will be passed on
            to remaining checks and actions.
        **kwargs : dict
            Remaining, unused variables.

        Returns
        -------
        None.

        """
        self.pproject, self.dco = project, dco
        self.resultvar = resultvar

    def holds(self, p_commobjects, p_project, **kwargs):
        """Check whether a specific module uses a DCO file

        Arguments:
            p_commobjects -- dictionary, keyed with module names, lists
                             of the parsed contents of different dco files
            p_project     -- project hosting the module

        Returns:
            Tuple (bool: true if match found,
            list explaining all maching dcos,
            dict with new variables for further processing
            )
        """

        res = list()
        newvars = dict()

        for m, commobj in p_commobjects.items():
            res.append(MatchReferenceDco(
                MatchFunctionDCO(self.pproject, self.dco),
                commobjects = commobj))

        checkAndSet(self.resultvar, newvars, res)
        result = [r for r in res if r.value ]

        return (result, map(self.__class__.matchresult.explain, result), newvars)


PolicyCondition.register("uses-dco", UsesDco)
