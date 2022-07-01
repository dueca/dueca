
# -*- coding: utf-8 -*-
"""
Created on Tue Jun  8 12:10:00 2021

@author: repa
"""

from pyparsing import Literal, Regex, QuotedString, ZeroOrMore, \
    Optional, nestedExpr, Combine, OneOrMore, CharsNotIn, ParserElement, \
    Or, Forward, NoMatch
import argparse   
import sys
import subprocess
import os

_number = Regex(r'[-]?[0-9]+(\.[0-9]*)?')
_comment = Regex(r';+') + \
    Regex(r'(.*)\n').setWhitespaceChars('')
_closing = Literal(r')')

cvtInt = lambda s, l, toks: int(toks[0])
_number.setParseAction(cvtInt)
_someName = Regex(r'[a-zA-Z0-9][-a-zA-Z0-9]*[?]?')
_someValue = Regex(r'[^)]+')

def extract(name, call, available):
    """ Extract a value from a named define call
    
    """
    
    # convert to an integer or float value
    def pres(s, loc, toks):
        #print(toks)
        try:
            call(name, int(toks[0]))
        except ValueError:
            call(name, float(toks[0]))
            
    # extract a string value from a double quoted
    def pstr(s, loc, toks):
        #print(toks)
        call(name, toks[0][1:-1])
        
    # common case, multiplication of two numbers
    def twopart(s, loc, toks):
        p1 = int(toks[1])
        p2 = int(toks[2])
        call(name, p1*p2)
        #print(toks)
        
    # other common case, reference to another variable
    def pcopy(s, loc, toks):
        try:
            call(name, available[toks[0]])
        except IndexError:
            print("Cannot find previous variable", toks[0], 
                  "for copying to", name)
            
    # true value
    def ptrue(s, loc, toks):
        #print(toks)
        call(name, True)
        
    # false value
    def pfalse(s, loc, toks):
        #print(toks)
        call(name, False)

    # analyse a define statement
    return Literal('(define') + Literal(name) + \
        (_number.copy().setParseAction(pres) ^     # number
         ( Literal('(*') + _number.copy() +        # multiplication
          _number.copy() + Literal(')')).setParseAction(twopart) ^
         Regex(r'"[^"]*"').setParseAction(pstr) ^  # string
         _someName.copy().setParseAction(pcopy) ^  
         Literal('#t').setParseAction(ptrue) ^
         Literal('#f').setParseAction(pfalse)
         ) + \
         _closing
   
# fallback define, unkown variable
def extractUnknownDefine():
    def pname(s, loc, toks):
        print("unknown label", toks[0])
    def pval(s, loc, toks):
        print("unknown value", toks[0])
    return Literal('(define') + _someName.copy().setParseAction(pname) \
        + _someValue.copy().setParseAction(pval) + Literal(')')

# dueca.cnf parsing class. Gets the common/relevant values
class ParseCnf:
    
    def result(self, name, res):

        
        if name in self.params:
            print("Overwriting", name, "with", res)
        else:
            print("Setting", name, "to", res)
        self.params[name] = res
        
    def __init__(self):
        
        self.params = { }
        node_id = extract('this-node-id', self.result, self.params)
        no_of_nodes = extract('no-of-nodes', self.result, self.params)
        send_order = extract('send-order', self.result, self.params)
        highest_manager = extract('highest-manager', self.result, self.params)
        multithread = extract('run-in-multiple-threads?', self.result, self.params)
        syncmode = extract('rt-sync-mode', self.result, self.params)
        graphic_interface = extract('graphic-interface', self.result, self.params)
        tick_base = extract('tick-base-increment', self.result, self.params)
        tick_compat = extract('tick-compatible-increment', self.result, self.params)
        tick_step = extract('tick-time-step', self.result, self.params)
        if_address = extract('if-address', self.result, self.params)
        mc_address = extract('mc-address', self.result, self.params)
        mc_port = extract('mc-port', self.result, self.params)
        packet_size = extract('packet-size', self.result, self.params)
        comm_prio = extract('comm-prio-level', self.result, self.params)
        unpack_prio = extract('unpack-prio-level', self.result, self.params)
        bulk_unpack_prio = extract('bulk-unpack-prio-level', self.result, self.params)
        bulk_size = extract('bulk-max-size', self.result, self.params) 
        out_size = extract('out-packet-size', self.result, self.params)
        unknown = extractUnknownDefine()
        
        self.doc = OneOrMore(node_id ^ no_of_nodes ^ send_order ^ 
                             highest_manager ^ multithread ^
                             syncmode ^ graphic_interface ^ tick_base ^ 
                             tick_compat ^ tick_step ^ if_address ^ mc_address ^ 
                             mc_port ^ packet_size ^ comm_prio ^ unpack_prio ^
                             bulk_unpack_prio ^ bulk_size ^ out_size ^ 
                             _comment ^ unknown)

def dprint(*args, **argv):
    print(*args, **argv)
    pass

class Value:
    
    def __init__(self, val=None):
        self.val = val
        
    def __str__(self):
        return f'{self.val}'
    
    @classmethod
    def parser(cls):
        return ValBool.parser() ^ ValInt.parser() ^ ValFloat.parser() ^ \
            ValStr.parser() ^ TimeSpec.parser() ^ PrioritySpec.parser()
            
class ValBool(Value):

    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return ValBool({ '#t': True, '#f': False }[toks[0]])

    @classmethod    
    def parser(cls):
        return (Literal('#t') ^ Literal('#f')).setParseAction(ValBool.new)
        
    
class ValInt(Value):
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return ValInt(int(toks[0]))

    @classmethod
    def parser(cls):
        return Regex(r'[-]?[0-9]+').setParseAction(ValInt.new)

class ValFloat(Value):
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return ValFloat(float(toks[0]))

    @classmethod    
    def parser(cls):
        return Regex(r'[-]?[0-9]+\.[0-9]*([eE][-+]?[0-9]+)?'
                     ).setParseAction(ValFloat.new)
    

class ValStr(Value):
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return ValStr(toks[0][1:-1])
        
    @classmethod
    def parser(cls):
        return Regex(r'".*"').setParseAction(ValStr.new)
        
        
class ValLiteral(ValStr):
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return ValStr(toks[0][1:])
        
    @classmethod
    def parser(cls):
        return Regex(r"'[-a-zA-Z0-9]+").setParseAction(ValLiteral.new)
      
class ValList(Value):
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return ValList(toks[1:-1])
    
    @classmethod
    def newEmpty(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        l = []
        return ValList(l)

    @classmethod       
    def parser(cls):
        global _value
        return Literal("'()").setParseAction(ValList.newEmpty) ^ \
                (Literal('(list') + (ZeroOrMore(_value ^ _statement)) + 
                 Literal(')')).setParseAction(ValList.new)

#ValList_parser = Literal('()').setParseAction(ValList.new) ^ \
#        (   Literal('(list') + ZeroOrMore(_value) + Literal(')')
#                 ).setParseAction(ValList.new)

class Comment(Value):

    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return Comment(toks[-1])
    
    @classmethod
    def parser(cls):
        return (OneOrMore(Literal(';')) +
            Regex(r'[^\n]*')).setParseAction(Comment.new)
            
    def __str__(self):
        return f"# {self.val}"
      


_variable = Regex('[-_a-zA-Z]+[-_?a-zA-Z0-9]*')

class BoundVariable:
    
    _store = {}
    
    def __init__(self, name, var):
        self.name = name
        self.var = var
        dprint(f"new BoundVariable {name}, {var}")
        BoundVariable._store[name] = self
        
    def __str__(self):
        return self.name.replace('-','_').replace('?','_')

    @classmethod
    def retrieve(cls, s, lok, toks):
        return BoundVariable._store[toks[0]]

    @classmethod
    def parser(cls):
        return _variable.copy().setParseAction(BoundVariable.retrieve)


class TimeSpec:
    
    def __init__(self, t0, t1):
        self.t0 = t0
        self.t1 = t1
        
    @classmethod
    def new(cls, s, lok, toks):
        return TimeSpec(toks[1], toks[2])

    def __str__(self):
        return f'dueca.TimeSpec({self.t0}, {self.t1})'
        
    @classmethod
    def parser(cls):
        return (Literal('(make-time-spec') + 
            (ValInt.parser() ^ BoundVariable.parser()) + 
            (ValInt.parser() ^ BoundVariable.parser()) +
                 Literal(')')).setParseAction(TimeSpec.new)

class PrioritySpec:
    
    def __init__(self, t0, t1):
        self.t0 = t0
        self.t1 = t1
        
    @classmethod
    def new(cls, s, lok, toks):
        return PrioritySpec(toks[1], toks[2])

    def __str__(self):
        return f'dueca.PrioritySpec({self.t0}, {self.t1})'
        
    @classmethod 
    def parser(cls):
        return (Literal('(make-priority-spec') +
            (ValInt.parser() ^ BoundVariable.parser()) + 
            (ValInt.parser() ^ BoundVariable.parser()) +
                 Literal(')')).setParseAction(PrioritySpec.new)
           
    @classmethod
    def test(cls):
        res = PrioritySpec.parser().parseString(
            """
            (make-priority-spec 3 0)""")
        print(res)

class Condition:
    pass
                
class ConditionEqual(Condition):
    
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def __str__(self):
        return f'{self.left} == {self.right}'
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return ConditionEqual(toks[1], toks[2])
    
    @classmethod
    def parser(cls):
        return (Literal('(equal?') + _value + _value + 
                Literal(')')
                ).setParseAction(ConditionEqual.new)
    
    
class ConditionAnd(Condition):
    
    def __init__(self, part1, part2):
        self.part1 = part1
        self.part2 = part2
                        
    def __str__(self):
        return f'({self.part1} and {self.part2})'
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return ConditionAnd(toks[1], toks[2])
    
    @classmethod
    def parser(cls):
        return (Literal('(and') + 
                (BoundVariable.parser() ^ _condition) +
                (BoundVariable.parser() ^ _condition) + 
                Literal(')')).setParseAction(ConditionAnd.new)

_condition = Forward()
_statement = Forward()
_value = Forward()

class IfFunction:
    
    def __init__(self, condition, casetrue, casefalse):
        self.condition = condition
        self.casetrue = casetrue
        self.casefalse = casefalse
        
    @classmethod
    def new(cls, s, lok, toks): 
        dprint(cls.__name__, lok, toks)
        return IfFunction(toks[1], toks[2], (len(toks) > 4 and toks[3]) or None)
    
    @classmethod
    def parser(cls):
        return (Literal('(if') + _condition + 
                _statement + Optional(_statement) + 
                Literal(')')).setParseAction(IfFunction.new)
    
    @classmethod
    def test(cls):
        res = IfFunction.parser().parseString(
            """(if (equal? 1 1) (set! a 3) )""")
        res = IfFunction.parser().parseString(
            """(if (equal? 1 1) (set! a 3) (list (set! b 5) (set! d "t") ))""")
        print(cls.__name__, res)

class VarLiteral:
    
    def __init__(self, name, args):
        self.name = name
        self.args = args
        
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return VarLiteral(toks[0][1:], toks[1:-1])
    
    @classmethod
    def parser(cls):
        return (Regex("'[-a-zA-Z0-9]+") + OneOrMore(_value)
                ).setParseAction(VarLiteral.new)
                
    @classmethod
    def test(cls):
        txt = [ """'someval 1""",
               """'other-val 1 2 "x" """]
        for t in txt:
            print("testing with", t)
            print(cls.parser().parseString(t))
         
        
        
        
class Entity:
    
    def __init__(self, name, contents):
        self.name = name
        self.contents = contents
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return Entity(toks[1], toks[2])
    
    @classmethod
    def parser(cls):    
        return (Literal('(make-entity') + 
                ValStr.parser() + 
                (OneOrMore(IfFunction.parser() ^ ValList.parser())) +
                Literal(')')
                ).setParseAction(Entity.new)

class Append:
    
    def __init__(self, parts):
        self.parts = parts
        self.all = []
        for p in self.parts:
            self.all.extend(p.val)
                
        
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return Append(toks[1:-1])
    
    @classmethod
    def parser(cls):
        return (Literal('(append') + 
                ZeroOrMore(ValList.parser() ^ BoundVariable.parser()) +
                Literal(')')).setParseAction(Append.new)


class Module:
    
    def __init__(self, name, part, priority, contents):
        self.name = name
        self.part = part
        self.priority = priority
        self.contents = contents

    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return Module(toks[1], toks[2], toks[3], toks[4:])

    @classmethod
    def newAppend(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        return Module(toks[2].all[0], toks[2].all[1], 
                      toks[2].all[3], toks[2].all[4:])
    
    @classmethod
    def parser(cls):
        return (Literal('(make-module') + ValLiteral.parser() + 
                ValStr.parser() + _value +
                #Or(PrioritySpec.parser(), BoundVariable.parser()) +
                ZeroOrMore(VarLiteral.parser()) +
                Literal(')')).setParseAction(Module.new) ^ \
            (Literal('(apply') + Literal("make-module") + Append.parser() +
             Literal(')')).setParseAction(Module.newAppend)
    
    @classmethod
    def test(cls):
        txt = [
            """(make-module 'tstmod "part" (make-priority-spec 1 0))
            """,
            """(make-module 'tstmod "part" (make-priority-spec 1 0)
            'some-param 2)
            """,
            """(apply make-module (append
                (list 'tstmod "" (make-priority-spec 0 0)
                 'set-timing (make-time-spec 4 5)
                 ) ) )"""
            ]
        for t in txt:
            print("testing with", t)
            print(Module.parser().parseString(t))
            
            
        
    
class DuecaList:
    
    @classmethod
    def new(cls, s, lok, toks):
        dprint(cls.__name__, lok, toks)
        
    @classmethod
    def parser(cls):
        return (Literal('(dueca-list') + 
                Entity.parser() + Literal(')'))

#PrioritySpec.test()

# test this
# print("parse result 1", _value.parseString('1')[0])
# print("parse result 2", _value.parseString('3.4')[0])
# print("parse result 3", _value.parseString(' "4,33"')[0])


class DefineSet:
    
    def __init__(self, name, var):
        self.name = name.replace('-','_').replace('?','_')
        self.var = var
        BoundVariable(name, var)
    
    @classmethod
    def new(cls, s, lok, toks):
        return DefineSet(toks[1], toks[2])

    def __str__(self):
        return f'{self.name} = {self.var}'
    
    @classmethod
    def parser(cls):
        return ( (Literal('(define ') ^ Literal('(set!')) + \
            _variable + _value + Literal(')')).setParseAction(DefineSet.new)
            
    @classmethod
    def test(cls):
        for tcase in (
                "(define a 1.4)",
                "(set! b 5)",
                "(define x-x-? #f)", 
                "(set! l (list 1 2 3))"):
            print(f"case {tcase}, -> {str(DefineSet.parser().parseString(tcase)[0])}")

_value << ( ValStr.parser() ^ ValFloat.parser() ^ \
    ValInt.parser() ^ ValBool.parser() ^ \
    TimeSpec.parser() ^ PrioritySpec.parser() ^ ValList.parser() ^ 
    ValLiteral.parser() ^ BoundVariable.parser() )

_condition << (ValBool.parser() ^ 
               ConditionEqual.parser() ^ 
               ConditionAnd.parser())

# baselevel statements
_statement << (DefineSet.parser() ^ ValList.parser() ^ 
               DuecaList.parser() ^ IfFunction.parser() ^ Module.parser() ^
               _comment)

"""
DefineSet.test()
VarLiteral.test()        
Module.test()          
IfFunction.test()
"""


class ParseMod:
    
    def addLine(self, s, lok, toks):
        dprint('ParseMod', lok, toks)
        self.lines.append([s, toks])
    
    def __init__(self, txt):
    
        self.lines = []
        parser = OneOrMore(
            (_statement).setParseAction(
                self.addLine))
        parser.parseString(txt)
    

# argument parser
parser = argparse.ArgumentParser(
        description=
            "Convert dueca projects from scheme to python scripting")
parser.add_argument(
    '--verbose', action='store_true',
    help="Verbose run with information output")
parser.add_argument(
    '--cnf', action='store_true',
    help="Only the dueca.cnf file")
parser.add_argument(
    '--mod', action='store_true',
    help="Only the dueca.mod file")
parser.add_argument('file', metavar='FILE', type=str, nargs='+',
                    help="file to be converted")


#runargs = parser.parse_args(['--mod',
#    "/home/repa/dapps/ActiveStickTest/ActiveStickTest/run/solo/solo"])
runargs = parser.parse_args(sys.argv[1:]) 

# check which types of file to convert
if not runargs.cnf and not runargs.mod:
    suffixes = ['.cnf', '.mod']
else:
    suffixes = []
    if runargs.mod:
        suffixes.append('.mod')
    if runargs.cnf:
        suffixes.append('.cnf')

# get a list of files
files = []
for f in runargs.file:
    dprint("testing", f)
    if os.path.isdir(f):
        files.extend([f'{f}/{f2}' for f2 in os.listdir(f) 
                      if len(f2) > 4 and f2[-4:] in suffixes ])
    elif os.path.isfile(f):
        files.extend(f)
        
# find template files
dc = subprocess.run(("dueca-config", "--path-datafiles"),
                    stdout=subprocess.PIPE)
dpath = dc.stdout.decode('ascii')

# .cnf file conversion is simplistic. Replace all occurrences of the following
# strings in the template
fdata = [ 'rt-sync-mode' ]

#print("files", files)

for f in files:
    path = os.sep.join(f.split(os.sep)[:-1]) or '.'
    txt = open(f, 'r').read()
    print("\n\nparsing file", f)        
    if f[-4:] == '.cnf':
        a = ParseCnf()
        
        a.doc.parseString(txt)
        if not os.path.exists(path + os.sep + 'dueca_cnf.py'):
            try:    
                with (open(path + os.sep + 'dueca_cnf.py', 'w'), 
                      open(f'{dpath}/data/default/dueca_cnf.py.inc', 'r')) \
                         as (f2,f0):
                    conffile = f0.read()
                    for k, v in a.params.items():
                        if f'@{k}@' in fdata:
                            conffile = str(v).join(conffile.split(f'@{k}@'))
                    f2.write(conffile)
            except Exception as e:
                print(f"Failed writing '{dpath}/data/default/dueca_cnf.py.in')")
                      
    elif f[-4:] == '.mod':
        a = ParseMod(txt)
                    
                
                

