#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun May  2 20:02:33 2021

@author: repa
"""

from .policycondition import PolicyCondition, checkAndSet
from ..xmlutil import XML_interpret_bool
from ..matchreference import MatchReferenceModule
import itertools as it

class MatchFunctionModule:
    """ Function object class indicating a project/module match

    """
    matchon = set(('project', 'module'))
    forall = set(('machine',))

    def __init__(self, project, module):
        """Create a match check

        Parameters
        ----------
        project : Param
            name/regex of the project to match
        module : Param
            name/regex of the module
        """
        self.projectref = project
        self.moduleref = module

    def __call__(self, project, module, **kwargs):
        """Test whether project/module matches the required

        Parameters
        ----------
        project : str
            Name of project hosting the module
        module : str
            Name of module

        Returns
        -------
        Bool
            True if project/module matches
        """
        return self.projectref.match(project) and self.moduleref.match(module)

    def explain(self, project=None, module=None, **kwargs):
        """Return a readable explanation of match or no match

        Parameters
        ----------
        project : str, optional
            Name of project, by default None
        module : str, optional
            Name of module, by default None

        Returns
        -------
        str
            Explanation
        """
        if project is None and module is None:
            return f'FALSE, no match on {self.projectref.val} / {self.moduleref.val}'
        return f"Match: project '{project}' ~ '{self.projectref.val}'" + \
            f" module '{module}' ~ '{self.moduleref.val}'"


class HasModule(PolicyCondition):

    matchresult = MatchReferenceModule

    # Determine how param arguments need to be stripped
    default_strip = dict(project='both', module='both', all_machines='both',
                         resultvar='both')

    def __init__(self, project='', module=None, all_machines="false",
                 resultvar='', **kwargs):
        """
        Test whether a module is used for the current or any machine class.

        Parameters
        ----------
        project : str
            Name of the project part of the module.
        module : str
            Name of the module.
        all_machines : bool, optional
            Check for all machine classes, or only the current one. The
            default is False.
        resultvar : str
            Result variable name. Details of the check Will be passed on
            to remaining checks and actions.
        **kwargs : dict
            Remaining, unused variables.

        Returns
        -------
        None.

        """
        # project and module may be Param objects
        self.project, self.module = project, module
        self.resultvar = str(resultvar)
        self.all_machines = XML_interpret_bool(str(all_machines))

    def holds(self, p_modules, p_project, p_machine, **kwargs):

        if not self.pproject == 0:
            self.pproject = p_project

        if self.all_machines:
            machines = p_modules.keys()
        else:
            machines = [ p_machine ]

        res = []
        newvars = dict()

        for m in machines:

            res.append(MatchReferenceModule(
                matchFunction=MatchFunctionModule(self.pproject, self.module),
                modules=p_modules[m]))
        
        checkAndSet(self.resultvar, newvars, res)
        result = [r for r in res if r.value ]
        return (result, map(self.__class__.matchresult.explain, result), newvars)




PolicyCondition.register("has-module", HasModule)
