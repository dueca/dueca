#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Mar 23 09:32:14 2023

@author: repa
"""
import re
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

class ToInfix:
    def __init__(self, sym, level, pool, *args):
        if len(args) < 2:
            raise ValueError(f"Cannot create infix {sym} from {args}")
        self.line = f" {sym} ".join(
            [ str(convert(level, pool, v)) for v in args ])
    def __str__(self):
        return f"({self.line})"

class ToUnitary:
    def __init__(self, sym, level, pool, *args):
        self.line = f"({sym} {str(convert(level, pool, args[0]))})"
        if len(args) > 1:
            raise ValueError(f"Too many arguments for unitary {sym}")
    def __str__(self):
        return f"({self.line})"

def splitargs(n, args):
    res = []
    cmnt = []
    for a in args:
        if isinstance(a, Comment):
            cmnt.append(a)
        else:
            res.append(a)
    while len(res) < n:
        res.append(None)
    return res, cmnt

class If:
    def __init__(self, level, pool, *args):
        (test, cmdtrue, cmdfalse), comments = splitargs(3, args)
        self.lines = [ str(convert(level, pool, cm)) for cm in comments ]
        self.lines.append(
            ' '*level + f"if {str(convert(level, pool, test))}:")
        if isinstance(cmdtrue, Expression) and cmdtrue.fname() == 'list':
            for c in cmdtrue.arguments[1:]:
                self.lines.append(str(convert(level+4, pool, c)))
        else:
            self.lines.append(str(convert(level+4, pool, cmdtrue)))
        if (cmdfalse is not None) and not (
            isinstance(cmdfalse, Expression) and cmdfalse.fname() == 'list' and
            len(cmdfalse.arguments) == 1):
            self.lines.append(
                ' '*level + 'else:')
            if isinstance(cmdfalse, Expression) and cmdfalse.fname() == 'list':
                for c in cmdfalse.arguments[1:]:
                    self.lines.append(str(convert(level+4, pool, c)))
            else:
                self.lines.append(str(convert(level+4, pool, cmdfalse)))

    def __str__(self):
        return '\n'.join(self.lines)

class PrioritySpec:
    def __init__(self, level, pool, *a):
        try:
            vals = dict(priority=0, order=0)
            try:
            # if isinstance(a[0], str) and isinstance(a[1], str):
                vals['priority'] = int(a[0])
                vals['order'] = int(a[1])

            except:
            #else:
                idx = 0
                while idx < len(a)-1:
                    if not isinstance(a[idx], ALiteral):
                        raise ValueError(f"Cannot get PrioritySpec from {a}")
                    vals[a[idx].name] = int(a[idx+1])
                    idx = idx + 2

            self.line = f"dueca.PrioritySpec({vals['priority']}, {vals['order']})"
        except ValueError:
            self.line = "dueca.PrioritySpec(please correct this)"
    def __str__(self):
        return self.line

class TimeSpec:
    def __init__(self, level, pool, *a):
        try:
            vals = {'validity-start': 0, 'period': 100}
            #if isinstance(a[0], str) and isinstance(a[1], str):
            try:
                vals['validity-start'] = int(a[0])
                vals['period'] = int(a[1])
            #else:
            except:
                idx = 0
                while idx < len(a)-1:
                    if not isinstance(a[idx], ALiteral):
                        raise ValueError(f"Cannot get PrioritySpec from {a}")
                    vals[a[idx].name] = int(a[idx+1])
                    idx = idx + 2

            self.line = f"dueca.TimeSpec({vals['validity-start']}, {vals['period']})"
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

class Font:
    def __init__(self, level, pool, f1, *args):
        allf = [ f'"{str(f1)}"' ]
        if len(args) > 0 and not isinstance(args[0], ALiteral):
            allf.append(f'"{str(args.pop(0))}"')
            if len(args) > 0 and not isinstance(args[0], ALiteral):
                allf.append(f'"{str(args.pop(0))}"')

        self.lines = [
            f'''dueca.Font({', '.join(allf)})''' ]
        if len(args):
            self.lines[-1] += '.param('
            self.lines.append(' '*(level+8) +
                              convertLiteralList(level+8, pool, args))
            self.lines[-1] += ')'
    def __str__(self):
        return '\n'.join(self.lines)

class FontManager:
    def __init__(self, level, pool, name, *args):
        self.lines = [
            f'dueca.FontManager("{str(name)}")' ]
        if len(args):
            self.lines[-1] += '.param('
            self.lines.append(' '*(level+4) +
                              convertLiteralList(level+8, pool, args))
            self.lines.append(' '*(level+4) + ').complete()')
        else:
            self.lines[-1] += '.complete()'
    def __str__(self):
        return '\n'.join(self.lines)

class Creatable:
    def __init__(self, name, level, pool, *args):
        # correct the name
        if '-' in name:
            name = ''.join(map(str.capitalize, name.split('-')))
        self.lines = [
            f'dueca.{name}().param(' ]
        self.lines.append(' '*(level+8) + convertLiteralList(level+8, pool, args))
        self.lines.append(' '*(level+8) + ')')
    def __str__(self):
        return '\n'.join(self.lines)

class Apply:
    def __init__(self, level, pool, cmd, append):
        global _ename
        # assert cmd.name == 'make-module'
        if cmd.name == 'make-module':

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

        else:
            print('Unsure about conversion of "apply" expression', cmd.name)
            cmdname = ''.join(map(str.capitalize, cmd.name.split('-')[1:]))

            self.lines = [' '* level + f'dueca.{cmdname}().param(' ]
            sub = []
            if isinstance(append, Expression):
                for a in append.arguments[1:]:
                    sub.append(' '*(level+8) + '*' + str(convert(level+8, pool, a)))
            elif isinstance(append, Identifier):
                sub.append(' '*(level+8) + '*' + str(convert(level+8, pool, append)))
            self.lines.append(',\n'.join(sub))
            self.lines.append(' '*(level+8) + ')')
            
    def __str__(self):
        return '\n'.join(self.lines)

class LoadExternal:
    def __init__(self, level, pool, fname):
        self.line = " "*level + \
            f"from {str(convert(level, pool, fname))[1:-1]} import *"
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
    'equal?': lambda l, p, *c: ToInfix("==", l, p, *c),
    'and': lambda l, p, *c: ToInfix("and", l, p, *c),
    'or': lambda l, p, *c: ToInfix("or", l, p, *c),
    'not': lambda l, p, *c: ToUnitary("not", l, p, *c),
    '*': lambda l, p, *c: ToInfix('*', l, p, *c),
    '+': lambda l, p, *c: ToInfix('+', l, p, *c),
    '/': lambda l, p, *c: ToInfix('/', l, p, *c),
    '-': lambda l, p, *c: ToInfix('-', l, p, *c),
    'make-entity': lambda l, p, *c: Entity(l, p, *c),
    'make-module': lambda l, p, *c: Module(l, p, *c),
    'make-stick-value': lambda l, p, *c: Creatable('MultiStickValue', l, p, *c),
    'make-font': lambda l, p, *c: Font(l, p, *c),
    'make-font-manager': lambda l, p, *c: FontManager(l, p, *c),
    'apply': lambda l, p, *c: Apply(l, p, *c),
    'dueca-list': lambda l, p, *c: DuecaBlurp(),
    'load': lambda l, p, *c: LoadExternal(l, p, *c),
    Comment: lambda l, p, c: ' '*l+f"# {c}",
    Identifier: lambda l, p, c: safeName(c),
    ALiteral: lambda l, p, c: c,
    String: lambda l, p, c: f'"{c}"',
    re.compile(r"make-(.+)"): lambda n, l, p, *c: Creatable(n, l, p, *c),
    }


if __name__ == '__main__':

    tryme = ('cssoft/cv/new/SenecaAutomationTraining/SenecaAutomationTraining/run/SRS/srsecs/dueca.mod',)
    tryme = ('cssoft/cv/new/SenecaAutomationTraining/SenecaAutomationTraining/run/run-data/andy_motion_filt.cnf',)
    tryme = ('cssoft/cv/new/SenecaAutomationTraining/SenecaAutomationTraining/run/solo/solo/dueca.mod',)
    tryme = ('cssoft/convert/new/MotionSenseOptimalAccelerations/MotionSenseOptimalAccelerations/run/solo/solo/dueca.mod',)
    tryme = ('cssoft/convert/new/MotionSenseBoris/MotionSenseBoris/run/solo/solo/dueca.mod',)
    for l in tryme:
        with open(f"{os.environ['HOME']}/{l}", 'r') as f:
            res = contents.parseFile(f)

        level = 0
        for r in res:
            print(r.convert(level, _pool))
    print(_values)
