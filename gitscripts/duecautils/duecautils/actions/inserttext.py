#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jun 30 21:31:46 2021

@author: repa
"""

from .policyaction import PolicyAction
from ..verboseprint import dprint
from ..xmlutil import XML_interpret_bool
from ..matchreference import doubleFile
import os

class ActionInsertText(PolicyAction):
    """
    Insert text at given places
    """

    xmlname = "insert-text"

    # parameter strip options
    default_strip = dict(inputvar='both',
                         mode='both', text='none',
                         substitute='both', substitutevar='both',
                         matchvar='both')

    def __init__(self, text, label='default', inputvar=None, mode="before",
                 substitute='', **kwargs):
        """
        Insert text at a given position in a file. Either use the
        file names and line numbers produced by a match condition,
        or a fixed file name and line number.

        Parameters
        ----------
        text : str
            Text to be inserted.
        inputvar : list, optional
            List with MatchReference objects as e.g., produced by
            FindPattern.
        mode : str, default 'before'
            Insert before, after or replace match

        Returns
        -------
        None.

        """
        super().__init__(**kwargs)

        if (inputvar is None):
            raise ValueError("InsertText, specify inputvar and label")

        self.text = text
        self.matchvar = inputvar
        self.label = label
        self.substitute = XML_interpret_bool(substitute)

        if str(mode) not in set(("replace", "before", "after")):
            raise ValueError(f"InsertText, cannot do mode {mode}")
        self.mode = str(mode)


    def enact(self, p_path, **kwargs):

        todo = [ td for td in kwargs[str(self.matchvar)] if td.value ]
        doubleFile(kwargs[str(self.matchvar)], self.matchvar)
        res = []
        files = []
        text = self.text
        for it in todo:

            if not it.value:
                continue
            try:
                dprint(f"Renaming {it.filename}")
                os.rename(it.filename, it.filename+'~')
                idxw = 0
                f0 = open(it.filename+'~', 'r')
                with open(it.filename, 'w') as f1:
                    txt0 = f0.read()
                    for rep in it.matches:
                        if self.mode =='replace':
                            f1.write(txt0[idxw:rep.span[0]])
                            f1.write(text.getString(reg=rep.matchre))
                            idxw = rep.span[1]
                        elif self.mode == 'before':
                            f1.write(txt0[idxw:rep.span[0]])
                            f1.write(text.getString(reg=rep.matchre))
                            idxw = rep.span[0]
                        elif self.mode == 'after':
                            f1.write(txt0[idxw:rep.span[1]])
                            f1.write(text.getString(reg=rep.matchre))
                            idxw = rep.span[1]
                        dprint(f'{it.filename} modification {self.mode} at'
                               f' {rep.span}')
                    # write the remaining
                    f1.write(txt0[idxw:])
                f0.close()
                files.append(it.filename)
                res.append(f'Modified {it.filename}')
            except Exception as e:
                raise e
                res.append(f'Failed modification of {it.filename}, error {e}')

        return '\n'.join(res), files

PolicyAction.register(ActionInsertText.xmlname, ActionInsertText)