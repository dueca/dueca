#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun May  2 20:02:33 2021

@author: repa
"""

from .policycondition import PolicyCondition, checkAndSet
from ..xmlutil import XML_interpret_bool
from ..matchreference import MatchReferenceModule

class HasModule(PolicyCondition):

    # Determine how param arguments need to be stripped
    default_strip = dict(project='both', module='both', all_machines='both',
                         resultvar='both')

    def __init__(self, project='', module=None, all_machines=False,
                 resultvar=None, **kwargs):
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
        self.pproject, self.module = project, module
        self.resultvar = resultvar
        self.all_machines = XML_interpret_bool(all_machines)

    def holds(self, p_modules, p_project, p_machine, **kwargs):

        if len(self.pproject) == 0:
            self.pproject = p_project

        if self.all_machines:
            machines = p_modules.keys()
        else:
            machines = [ p_machine ]

        res = []
        newvars = dict()

        for m in machines:
            res.append(MatchReferenceModule(
                module_project=self.pproject, module=self.module,
                machine_class=m,
                fname=f'{p_project}/.config/class/{m}/modules.xml'))
            res[-1].modules = p_modules[m]
            if p_modules[m].hasModule(
                project=self.pproject, module=self.module):
                res[-1].value = True

        checkAndSet(self.resultvar, newvars, res)
        haves = [ r.filename for r in res if r.value ]

        return (
            len(haves) > 0,
            (len(haves) > 0 and
             [ f'Have module {self.pproject}/{self.module} in {fn}'
                  for fn in haves ]) or
            [ f'FALSE: No module {self.pproject}/{self.module} found' ],
             newvars)

PolicyCondition.register("has-module", HasModule)
