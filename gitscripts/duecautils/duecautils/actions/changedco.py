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
    default_strip = dict(inputvar='both', mode='both',
                         new_dco='both', new_project='both')

    def __init__(self, inputvar, mode='noop',
                 new_project = None, new_dco=None, **kwargs):

        super().__init__(**kwargs)

        self.dcofiles = inputvar
        self.new_dco = new_dco
        self.new_prj = new_project
        self.mode = str(mode)

    def enact(self, p_commobjects, p_policy, p_polid, p_modules, **kwargs):

        try:
            # figure out which dco files have been changed
            dcolists = [l for l in kwargs[str(self.dcofiles)] if l.value ]
        except KeyError as e:
            print(f"Cannot find variable {self.dcofiles}, in {kwargs.keys()}",
                  file=sys.stderr)
            raise e

        res = []
        for dcl in dcolists:

            res = [ f'Policy {p_polid}, on file {dcl.filename}']
            toadd = []
            todelete = []

            for co in dcl.commobjects:
                if dcl.matchFunction(co.base_project, co.dco):
                    ndco, nprj = None, None
                    if self.new_dco is not None:
                        ndco = self.new_dco.getString(vars=dict(dco=co.dco))
                    if self.new_prj is not None:
                        nprj = self.new_prj.getString(vars=dict(project=co.base_project))

                    if self.mode == 'add' or self.mode == 'replace':
                        toadd.append((nprj, ndco, f"Added for Policy: '{p_policy}':'{p_polid}'"))

                    if self.mode == 'delete' or self.mode == 'replace':
                        todelete.append((co.base_project, co.dco,  f"Removed for Policy: '{p_policy}':'{p_polid}'"))

            for nproj, ndco, comment in todelete:
                try:
                    dcl.commobjects.delete(nproj, 'comm-objects', ndco, comment)
                    res.append(
                        f'Deleted {nproj}/comm-objects/{ndco}.dco')
                except ValueError:
                    res.append(
                        f'Fail delete {nproj}/comm-objects/{ndco}.dco')

            for nproj, ndco, comment in toadd:
                dcl.commobjects.add(nproj, 'comm-objects', ndco, comment)
                res.append(
                            f'Added {nproj}/comm-objects/{ndco}.dco')

            if self.mode != 'noop':
                dcl.commobjects._sync()
            else:
                res.append('No changes applied.')

        return '\n'.join(res), [ dcl.filename for dcl in dcolists if dcl.value]

PolicyAction.register(ActionChangeDco.xmlname, ActionChangeDco)
