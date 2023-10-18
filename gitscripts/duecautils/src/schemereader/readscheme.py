#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Mar 23 09:32:14 2023

@author: repa
"""

from pyparsing import ZeroOrMore, \
    Literal, Regex, LineEnd, SkipTo, Forward
from pyparsing import pyparsing_common as ppcom
import os
import sys
from itertools import accumulate
import numpy as np
import re

_debprint = False
_values = {}

def defineValue(x):
    global _values
    _values[x[0].name] = lookup(x[1])
    return None

def setValue(x):
    global _values
    # ignored for now

def lookup(x):
    if isinstance(x, String):
        return x.value
    try:
        return _values[x.name]
    except (TypeError, KeyError, AttributeError):
        return x
    except Exception as e:
        print(f"lookup fails on {x}: {e}", file=sys.stderr)
        raise e

def lookupValues(x):
    global _values
    res = []
    for v in x:
        try:
            res.append(_values[v.name])
        except (TypeError, KeyError, AttributeError):
            res.append(v)
        except Exception as e:
            print(f"lookup fails on {x}: {e}", file=sys.stderr)
            raise e
    return res

def getValues():
    global _values
    return _values

def clearValues():
    global _values
    _values.clear()

def ifCondition(x):
    if lookup(x[0]):
        return x[1]
    if len(x) == 3:
        return x[2]

def greaterTest(x):
    return lookup(x[0]) > lookup(x[1])

def smallerTest(x):
    return lookup(x[0]) < lookup(x[1])

_evaluate = {
    '*': lambda x: np.prod(lookupValues(x)),
    '+': lambda x: sum(lookupValues(x)),
    'list': lambda x: list(lookupValues(x)),
    'define': defineValue,
    'set!': setValue,
    'if': ifCondition,
    '>': greaterTest,
    '<': smallerTest
}

# debug result
def pParse(name):
    def _pParse(s, loc, toks):
        if _debprint:
            print(f"{name} at {loc}: {toks}")
    return _pParse

def convert(level, pool, arg):
    try:
        return arg.convert(level, pool)
    except AttributeError:
        return str(arg)
    except Exception as e:
        print(f"Unhandled error {e}\n"
            f"{level}, {arg}")
        raise

class ALiteral:
    def __init__(self, v):
        self.name = v[1:]
    def convert(self, level, pool):
        return pool[ALiteral](level, pool, self.name)
    def __str__(self):
        return self.name

class Identifier:
    def __init__(self, v):
        self.name = v

    def __repr__(self):
        return f'ID({self.name})'

    def __str__(self):
        return self.name

    def convert(self, level, pool):
        return pool[Identifier](level, pool, self.name)

class String:
    def __init__(self, v):
        self.value = v
    def __str__(self):
        return self.value
    def convert(self, level, pool):
        return pool[String](level, pool, self.value)


class Comment:

    def __init__(self, v):
        self.comment = v

    def __str__(self):
        return self.comment

    def __repr__(self):
        return f'CM({self.comment})'

    def convert(self, level, pool):
        try:
            return pool[Comment](level, pool, self.comment)
        except KeyError:
            return str(self)

class Expression:

    def __init__(self, args):
        self.arguments = args # [ a for a in args if not isinstance(a, Comment) ]

    def fname(self):
        return self.arguments[0].name

    def run(self):
        global _evaluate
        a2 = []
        for a in self.arguments:
            if isinstance(a, Expression):
                res = a.run()
                if res is not None:
                    a2.append(res)
            elif isinstance(a, Comment):
                pass
            else:
                a2.append(a)
        try:
            if _debprint:
                print(f"eval of {a2}")
            return _evaluate[a2[0].name](a2[1:])
        except Exception as e:
            if _debprint:
                print(f"Cannot evaluate {a2[0]}: {e}")

    def convert(self, level, pool):
        if self.arguments[0].name in pool:
            #print(f"Calling {self.arguments[0].name} from pool")
            return pool[self.arguments[0].name](
                level, pool, *self.arguments[1:])
        for k, e in pool.items():
            if isinstance(k, re.Pattern):
                m = k.fullmatch(self.arguments[0].name)
                if m:
                    return e(m.group(1), level, pool, *self.arguments[1:])

# token parsing
comment = (Literal(';') + SkipTo(LineEnd(), include=True)).setParseAction(
    pParse('comment')).addParseAction(lambda t: [Comment(t[1])])
mixcomment = comment.copy().setParseAction(
    pParse('mixcomment')).addParseAction(lambda t: [Comment(t[1])])
literal = Regex(r"'[a-zA-Z0-9+-\.\*/<=>!?:$%_&~^]+").setParseAction(
    pParse('literal')).addParseAction(lambda t: [ALiteral(t[0])])
identifier = Regex(r"[a-zA-Z0-9+-\.\*/<=>!?:$%_&~^]+").setParseAction(
    pParse('identifier')).addParseAction(lambda t: [Identifier(t[0])])
integer = ppcom.integer.copy().setParseAction(
    pParse('integer')).addParseAction(lambda t: [int(t[0])])
fnumber = ppcom.real.copy().setParseAction(
    pParse('float')).addParseAction(lambda t: [float(t[0])])
booltrue = Literal("#t").addParseAction(lambda t: [True])
boolfalse = Literal("#f").addParseAction(lambda t: [False])
string = Regex(r'"[^"\\]*(?:\\.[^"\\]*)*"').setParseAction(
    pParse('string')).addParseAction(lambda t: [String(t[0][1:-1])])
emptylist = Literal("'()").addParseAction(lambda t: [[]])
value = emptylist | booltrue | boolfalse | fnumber | integer | string

expression = Forward()
argument = value | literal | expression | identifier | mixcomment
expression << Literal('(') + identifier + ZeroOrMore(argument) + Literal(')')
expression.setParseAction(pParse('expression')).addParseAction(
    lambda t: [Expression(t[1:-1])])
contents = ZeroOrMore(expression | comment)

if __name__ == '__main__':

    tryme = ('gdapps/JNDexperiment/JNDexperiment/run/solo/solo',)
    tryme = ('cssoft/cv/new/SenecaAutomationTraining/SenecaAutomationTraining/run/SRS/srsecs',)
    # define print for debug
    _debprint = True

    for l in tryme:
        with open(f"{os.environ['HOME']}/{l}/dueca.mod", 'r') as f:
            res = contents.parseFile(f)

        for r in res:
            if isinstance(r, Expression):
                r.run()
