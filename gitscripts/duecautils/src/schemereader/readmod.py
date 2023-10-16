#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Mar 23 09:32:14 2023

@author: repa
"""

import os
try:
    from .readscheme import contents, _values, Expression, _debprint, _evaluate, \
        Comment, convert, Identifier, ALiteral, String
except:
    from readscheme import contents, _values, Expression, _debprint, _evaluate, \
        Comment, convert, Identifier, ALiteral, String


_entities = set()
_ename = 'anon'

def safeName(x):
    return ''.join(
        [((i in r'-.*/<=>!?:$%&~^') and '_') or i for i in x])

def doEval(x):
    pass

def makeEntity(args):
    global _entities
    _entities.add(args[0])

    res = []
    for a in args:
        if isinstance(a, Expression):
            res.append(a.run())
        elif isinstance(a, Comment):
            res.append(f'# {str(a)}')
        else:
            print(f"Cannot process {a}")

def equalTest(args):
    return f'({str(args[0])} == {str(args[1])})'

def ignoreDef(args):
    # print(f"Ignoring {args}")
    return None

_evaluate['make-entity'] = makeEntity
_evaluate['equal?'] = equalTest
_evaluate['dueca-list'] = ignoreDef

def convertLiteralList(level, pool, data):
    pre = [ ]
    data2 = [ ]
    cmnts = [ ]
    for d in data:
        if isinstance(d, ALiteral):
            data2.append([d])
        elif isinstance(d, Comment):
            cmnts.append(str(convert(level, pool, d)))
        elif len(data2):
            data2[-1].append(d)
        else:
            pre.append(d)
    if cmnts:
        cmnts = '\n' + '\n'.join(cmnts) + '\n'
    else:
        cmnts = ''
    res = []
    for d in pre:
        res.append(str(convert(level, pool, d)))

    for d in data2:
        r2 = [f'"{str(convert(level, pool, d[0]))}"']
        if len(d) > 2:
            r2[-1] = r2[-1] + ', (' + str(convert(level, pool, d[1]))
            for d2 in d[2:]:
                r2.append(str(convert(level, pool, d2)))
            r2[-1] += ')'
        else:
            r2.append(str(convert(level, pool, d[1])))
        res.append(f'({", ".join(r2)})')
    if len(data) <= 3:
        return f"{', '.join(res)}{cmnts}"
    else:
        return (',\n'+' '*level).join(res)+cmnts

class List:
    def __init__(self, level, pool, *data):
        if level == 0 and len(data) == 0:
            # special case, scheme empty list
            self.line = ''
        elif len(data) == 0:
            self.line = '[ ]'
        else:
            self.line = '[ ' + convertLiteralList(level, pool, data) + ' ]'
    def __str__(self):
        return self.line

class Define:
    def __init__(self, level, pool, varname, value):
        vn = convert(level, pool, varname)
        if isinstance(value, Expression) and value.fname() == 'make-entity':
            self.line = str(value.convert(0, pool))
        else:
            vv = convert(level, pool, value)
            self.line = ' '*level + f"{str(vn)} = {str(vv)}"
    def __str__(self):
        return self.line

class Equal:
    def __init__(self, level, pool, v1, v2):
        vl = convert(level, pool, v1)
        vr = convert(level, pool, v2)
        self.line = f"{str(vl)} == {str(vr)}"
    def __str__(self):
        return self.line

class And:
    def __init__(self, level, pool, *args):
        self.line = " and ".join(
            [ f"( {str(convert(level, pool, v))} )" for v in args ])
    def __str__(self):
        return self.line

class Or:
    def __init__(self, level, pool, *args):
        self.line = " or ".join(
            [ f"( {str(convert(level, pool, v))} )" for v in args ])
    def __str__(self):
        return self.line

class Multiplication:
    def __init__(self, level, pool, *args):
        self.line = " * ".join(
            [ str(convert(level, pool, v)) for v in args ])
    def __str__(self):
        return self.line

class If:
    def __init__(self, level, pool, test, cmdtrue, cmdfalse=None):
        self.lines = [
            ' '*level + f"if {str(convert(level, pool, test))}:"]
        if isinstance(cmdtrue, Expression) and cmdtrue.fname() == 'list':
            for c in cmdtrue.arguments[1:]:
                self.lines.append(str(convert(level+4, pool, c)))
        else:
            self.lines.append(str(convert(level+4, pool, cmdtrue)))
        if cmdfalse is not None:
            self.lines.append(
                ' '*level + 'else:')
            if isinstance(cmdfalse, Expression) and cmdfalse.fname == 'list':
                for c in cmdfalse.arguments[1:]:
                    self.lines.append(str(convert(level+4, pool, c)))
            else:
                self.lines.append(str(convert(level+4, pool, cmdfalse)))

    def __str__(self):
        return '\n'.join(self.lines)

class PrioritySpec:
    def __init__(self, level, pool, *a):
        try:
            self.line = f"dueca.PrioritySpec({int(a[0])}, {int(a[1])})"
        except ValueError:
            self.line = "dueca.PrioritySpec(please correct this)"
    def __str__(self):
        return self.line

class TimeSpec:
     def __init__(self, level, pool, *a):
         try:
             self.line = f"dueca.TimeSpec({int(a[0])}, {int(a[1])})"
         except ValueError:
             self.line = "dueca.TimeSpec(please correct this)"
     def __str__(self):
         return self.line

class Entity:
    def __init__(self, level, pool, *arg):
        global _ename
        self.name = str(arg[0])
        _ename = safeName(self.name)
        self.lines = [ f'mods_{_ename} = []' ]
        for a in arg[1:]:
            self.lines.append(str(convert(0, pool, a)))
        self.lines.append("# Create the entity")
        self.lines.append(
            f'{_ename} = dueca.Entity("{self.name}", mods_{_ename})')
    def __str__(self):
        return '\n'.join(self.lines)

class Module:
    def __init__(self, level, pool, name, part, prio, *args):
        global _ename
        self.lines = [' '*level + f'mods_{_ename}.append(\n' + ' '*level +
            f'    dueca.Module("{name.name}", "{str(part)}", {str(convert(level, pool, prio))})' ]
        if len(args):
            self.lines[-1] += '.param('
            self.lines.append(' '*(level+8) +
                              convertLiteralList(level+8, pool, args))
            self.lines.append(' '*(level+8) + '))')
        else:
            self.lines.append(' '*(level+8) + ')')
    def __str__(self):
        return '\n'.join(self.lines)

class Creatable:
    def __init__(self, name, level, pool, *args):
        self.lines = [
            f'dueca.{name}().param(' ]
        self.lines.append(' '*(level+8) + convertLiteralList(level+8, pool, args))
        self.lines.append(' '*(level+8) + ')')
    def __str__(self):
        return '\n'.join(self.lines)

class Apply:
    def __init__(self, level, pool, cmd, append):
        global _ename
        assert cmd.name == 'make-module'
        assert isinstance(append, Expression)
        assert append.arguments[0].name == 'append'
        name = append.arguments[1].arguments[1]
        part = append.arguments[1].arguments[2]
        prio = append.arguments[1].arguments[3]
        append.arguments[1].arguments.pop(3)
        append.arguments[1].arguments.pop(2)
        append.arguments[1].arguments.pop(1)
        self.lines = [' '*level + f'mods_{_ename}.append(\n' + ' '*level +
            f'    dueca.Module("{name.name}", "{str(part)}", {str(convert(level, pool, prio))}).param(' ]
        sub = []
        for a in append.arguments[1:]:
            sub.append(' '*(level+8) + '*' + str(convert(level+8, pool, a)))
        self.lines.append(',\n'.join(sub))
        self.lines.append(' '*(level+8) + '))')
    def __str__(self):
        return '\n'.join(self.lines)

class LoadExternal:
    def __init__(self, level, pool, fname):
        self.line = f"import {fname.split('.')[0]}"
    def __str__(self):
        return self.line

class DuecaBlurp:
    def __str__(self):
        return """
## ---------------------------------------------------------------------
### the modules needed for dueca itself
if this_node_id == ecs_node:

    # create a list of modules:
    DUECA_mods = []
    DUECA_mods.append(dueca.Module("dusime", "", admin_priority))
    DUECA_mods.append(dueca.Module("dueca-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("activity-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("timing-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("log-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("channel-view", "", admin_priority))
    # uncomment for web-based graph, see DUECA documentation
    # DUECA_mods.append(dueca.Module("config-storage", "", admin_priority))

    if no_of_nodes > 1 and not classic_ip:
        DUECA_mods.append(dueca.Module("net-view", "", admin_priority))

    # remove the quotes to enable DUSIME initial condition recording and
    # setting, and simulation recording and replay, indicate the right
    # entity names
    '''
    for e in ("PHLAB",):
        DUECA_mods.append(
            dueca.Module("initials-inventory", e, admin_priority).param(
                reference_file=f"initials-{e}.toml",
                store_file=f"initials-{e}-%Y%m%d_%H%M.toml"))
        DUECA_mods.append(
            dueca.Module("replay-master", e, admin_priority).param(
                reference_files=f"recordings-{e}.ddff",
                store_files=f"recordings-{e}-%Y%m%d_%H%M%S.ddff"))
    '''

    # create the DUECA entity with that list
    DUECA_entity = dueca.Entity("dueca", DUECA_mods)
"""

new_mod_header = \
"""## -*-python-*-
## This is a dueca_mod.py file, converted from an existing dueca.mod
## (scheme script) file.
## Please revise the conversion, as it might not be perfect.
## Note that the comments may be shifted from their proper positions.

"""

_pool = {
    'set!': lambda l, p, *c: Define(l, p, *c),
    'list': lambda l, p, *c: List(l, p, *c),
    'define': lambda l, p, *c: Define(l, p, *c),
    'if': lambda l, p, *c: If(l, p, *c),
    'make-priority-spec': lambda l, p, *c: PrioritySpec(l, p, *c),
    'make-time-spec': lambda l, p, *c: TimeSpec(l, p, *c),
    'equal?': lambda l, p, *c: Equal(l, p, *c),
    'and': lambda l, p, *c: And(l, p, *c),
    'or': lambda l, p, *c: Or(l, p, *c),
    '*': lambda l, p, *c: Multiplication(l, p, *c),
    'make-entity': lambda l, p, *c: Entity(l, p, *c),
    'make-module': lambda l, p, *c: Module(l, p, *c),
    'make-stick-value': lambda l, p, *c: Creatable('MultiStickValue', l, p, *c),
    'apply': lambda l, p, *c: Apply(l, p, *c),
    'dueca-list': lambda l, p, *c: DuecaBlurp(),
    'load': lambda l, p, *c: LoadExternal(l, p, *c),
    Comment: lambda l, p, c: ' '*l+f"# {c}",
    Identifier: lambda l, p, c: safeName(c),
    ALiteral: lambda l, p, c: c,
    String: lambda l, p, c: f'"{c}"'
    }


if __name__ == '__main__':

    tryme = ('cssoft/cv/new/SenecaAutomationTraining/SenecaAutomationTraining/run/SRS/srsecs',)

    for l in tryme:
        with open(f"{os.environ['HOME']}/{l}/dueca.mod", 'r') as f:
            res = contents.parseFile(f)

        level = 0
        for r in res:
            print(r.convert(level, _pool))
    print(_values)

