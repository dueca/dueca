#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun May  2 16:09:22 2021

@author: repa
"""

from .policyaction import PolicyAction
import sys

class ActionChangeModule(PolicyAction):
    """
    Add, remove or exchange modules.
    """

    xmlname = 'change-module'

    # parameter strip options
    default_strip = dict(inputvar='both',
                         mode='both',
                         new_project='both', new_module='both',
                         url='both', version='both')

    def __init__(self, inputvar: str, mode='noop',
                 new_project = None, new_module=None,
                 url=None, version='', **kwargs):
        """
        Create an action to add a module to a project, remove one or
        do both.

        Arguments are determined by the parameters given in the xml file.

        Parameters
        ----------
        modulelists : MatchReference
            details on modules.xml files to be changed
        new_project : TYPE, optional
            Project of the module to be added.
        new_module : TYPE, optional
            Module to be added.
        url : TYPE, optional
            Full URL of the new project, needed if project is not yet
            part of the project.
        version : TYPE, optional
            Version to be borrowed.
        **kwargs : TYPE
            Filling for excess arguments.

        Returns
        -------
        None.

        """


        super().__init__(**kwargs)

        self.modulelists = inputvar
        self.new_module = new_module
        self.new_project = new_project
        if url is None and new_project:
                url = f'dgr:///{new_project}.git'
        self.url = ((url is not None) and url) or url
        self.version = version
        self.mode = mode

    def enact(self, p_policy: str, p_polid, **kwargs):
        """
        Run the module swap, deletion or addition.

        Parameters
        ----------
        p_modules : Modules
            Representation of the modules.xml file.
        p_policy : str
            Name of the policy driving this change.
        p_polid : str
            ID of the policy driving this change.
        **kwargs : dict
            Remaining unused arguments.

        Returns
        -------
        tuple(str, list-of-str)
            Description of implemented changes and list of files affected.

        """
        try:
            modulelists = [ ml for ml in kwargs[self.modulelists]
                            if ml.value ]
        except KeyError as e:
            print(f"Cannot find variable {self.modulelists}, in {kwargs.keys()}",
                  file=sys.stderr)
            raise e

        modified = []
        for ml in modulelists:

            if not ml.value:
                continue

            res = [ f'Policy {p_polid}, on file {ml.filename}']
            if self.mode == 'add' or self.mode == 'replace':
                try:
                    ml.modules.addModule(
                        self.new_project, self.new_module,
                        self.version, self.url)
                    res.append(
                        f'Added {self.new_module[0]}/{self.new_module[1]}')
                except Exception as e:
                    res.append(
                        f'Failure adding {self.new_module[0]}/{self.new_module[1]}: {str(e)}')

            if self.mode == 'delete' or self.mode == 'replace':
                for m in ml.modules:
                    if ml.matchFunction(m['project'], m['module']):
                        ml.modules.deleteModule(m['project'], m['module'])
                res.append(f'Deleted {m["project"]}/{m["module"]}')
            if self.mode != 'noop':
                ml.modules._sync()
                modified.append(ml.filename)
            else:
                res.append('No changes applied.')

        return '\n'.join(res), modified

PolicyAction.register(ActionChangeModule.xmlname, ActionChangeModule)
