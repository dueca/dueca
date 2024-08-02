#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun May  2 15:58:35 2021

@author: repa
"""

from ..xmlutil import XML_tag, XML_comment
from ..param import Param

class PolicyAction:
    """ Interface class for actions/edits for implementing a policy.

    """

    _actions = {}

    def __init__(self, **kwargs):
        """
        Create a policy action object.

        For this base class, the initialisation is a noop. For derived
        classes, the kwargs contents can be used.

        Parameters
        ----------
        **kwargs : TYPE
            Contents for creation; follows the naming from the <param> tags
            in the xml file.

        Returns
        -------
        None.

        """
        pass

    def enact(self, **kwargs):
        """
        Implement a policy by editing the project.

        Parameters
        ----------
        **kwargs : dict
            Available arguments in the dict:
                p_path : str    - path to the project folder.
                p_project : str - project name
                p_machine : str - machine class current checkout
                p_modules : dict(str, ModuleList) - dict with module lists
                                  for all machine classes
                p_policy : str   - currently considered/tested policy
                p_polid : str    - policy ID
                p_commobjects : dict(str, CommObjectsList) - CommObjects
                                   lists defined

                Additional arguments may be inserted by the policy condition
                checks

        Returns
        -------
        message.

        """
        raise(Exception("Cannot enact, derive "))

    @classmethod
    def register(cls, name, action):
        """
        Record a link between name/xml tag and policy code.

        Parameters
        ----------
        cls : object
            current class.
        name : str
            Name of the class.
        action : object
            Derived class.

        Returns
        -------
        None.

        """
        if name in cls._actions:
            raise(IndexError(
                    f"Attempting double registration for action {name}"))
        cls._actions[name] = action

    @classmethod
    def create(cls, node, **kwargs):
        """
        Create a policy action from the given xml node, and parameters in
        kwargs

        Parameters
        ----------
        cls : TYPE
            This class
        node : lxml.Element
            XML node with creation information.
        **kwargs : dict
            Interpreted arguments from the XML node.

        Returns
        -------
        PolicyAction
            Derived class that can implement the given policy action.

        """

        name = node.get('type')
        params = {}
        for par in node:
            if XML_comment(par):
                continue
            elif XML_tag(par, 'param'):
                p = Param(par, cls._actions[name].default_strip.get(
                          par.get('name'), ''))
                params[p.name] = p

        try:
            return cls._actions[name](_node=node, **params)
        except TypeError as e:
            print(f"failing to create action of type {name} with {params}: {e}")
            raise e
