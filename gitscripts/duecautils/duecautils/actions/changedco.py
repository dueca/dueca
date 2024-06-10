#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun May  2 16:09:22 2021

@author: repa
"""

from .policyaction import PolicyAction
import sys

class ActionChangeDco(PolicyAction):

    xmlname = 'change-dco'

    # parameter strip options
    default_strip = dict(inputvar='both', old_dco='both', old_project='both',
                         new_dco='both', new_project='both')

    def __init__(self, inputvar, old_project=None, old_dco=None,
                 new_project = None, new_dco=None, **kwargs):

        super().__init__(**kwargs)

        self.dcofiles = inputvar
        self.old_dco, self.new_dco = None, None
        if old_project is not None and old_dco is not None:
            self.old_dco = old_project, old_dco
        if new_project is not None and new_dco is not None:
            self.new_dco = new_project, new_dco

        self.mode = 'noop'
        if self.new_dco:
            if self.old_dco:
                self.mode = 'replace'
            else:
                self.mode = 'add'
        elif self.old_dco:
            self.mode = 'delete'

    def enact(self, p_commobjects, p_policy, p_polid, p_modules, **kwargs):

        try:
            # figure out which dco files have been changed
            dcolists = [l for l in kwargs[self.dcofiles] if l.value ]
        except KeyError as e:
            print(f"Cannot find variable {self.dcofiles}, in {kwargs.keys()}",
                  file=sys.stderr)
            raise e

        res = []
        for dcl in dcolists:

            res = [ f'Policy {p_polid}, on file {dcl.filename}']
            if self.mode == 'add' or self.mode == 'replace':
                dcl.commobjects.add(self.new_dco[0], self.new_dco[1],
                               f"Added for Policy: '{p_policy}':'{p_polid}'")
                res.append(
                    f'Added {self.new_dco[0]}/comm-objects/{self.new_dco[1]}.dco')

            if self.mode == 'delete' or self.mode == 'replace':
                try:
                    dcl.commobjects.delete(
                        self.old_dco[0], self.old_dco[1],
                        f"Removed for Policy: '{p_policy}':'{p_polid}'")
                    res.append(
                        f'Deleted {self.old_dco[0]}/{self.old_dco[1]}.dco')
                except ValueError as e:
                    res.append(
                        f'Failed deletion {self.old_dco[0]}/{self.old_dco[1]}.dco: {e}')
            if self.mode != 'noop':
                dcl.commobjects._sync()
            else:
                res.append('No changes applied.')

        return '\n'.join(res), [ dcl.filename for dcl in dcolists if dcl.value]

PolicyAction.register(ActionChangeDco.xmlname, ActionChangeDco)
