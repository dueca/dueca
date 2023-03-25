#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Mar 23 09:32:14 2023

@author: repa
"""

import os
from readscheme import contents, _values, Expression

if __name__ == '__main__':
    for l in ('gdapps/JNDexperiment/JNDexperiment/run/solo/solo',):
        with open(f"{os.environ['HOME']}/{l}/dueca.cnf", 'r') as f:
            res = contents.parseFile(f)

        for r in res:
            if isinstance(r, Expression):
                r.run()
    print(_values)

