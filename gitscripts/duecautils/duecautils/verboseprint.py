#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Mar  5 14:52:42 2021

@author: repa
"""

_verbose_print = False
def dprint(*arg, **kwarg):
    global _verbose_print
    if _verbose_print:
        print(*arg, **kwarg)
