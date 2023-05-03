#!/usr/bin/env python3
"""     item            : dueca-codegen
        made by         : RvP
        date            : 2014
        category        : python program
        description     : Code generation of DUECA Communication Objects (DCO)
        language        : python
        changes         : 161123 RvP Add a define test (__DCO_STANDALONE), that
                                     if defined creates a non-dueca object
                          161123 RvP Add a second define test (__DCO_NOPACK),
                                     that excludes packing&unpacking code
                          1704xx RvP Added a plugin system, to enable
                                     extension of code generation
        copyright       : 2014 TUDelft-AE-C&S
        copyright       : 2022 Rene van Paassen
        license         : EUPL-1.2
"""

from __future__ import print_function

from pyparsing import Literal, Regex, QuotedString, ZeroOrMore, Word, \
    Optional, nestedExpr, Combine, OneOrMore, CharsNotIn, ParserElement, Or, \
    White
import sys
import copy
import traceback
import os
import pwd
import datetime
import getpass
import subprocess
from argparse import ArgumentParser
import importlib

ParserElement.enablePackrat()

helptext = \
    """Generate DCO object header and body code from the standard input"""
epilog = "copyright DUECA authors, 2022"

# some global flags
in_dueca = False
do_debug = False
compilerarg = []
nextinclude = False
objectnamespace = []
pack_alignment = None
currentobject = None

# pre strip compiler arguments - oldstyle
if '--' in sys.argv[1:]:
    idx = sys.argv.index('--')
    compilerarg = sys.argv[idx+1:]
    del sys.argv[idx:]
    del idx

aparser = ArgumentParser(description=helptext, epilog=epilog)
aparser.add_argument(
    '-d', '--dueca',  action='store_true',
    help="create an object for DUECA internally, in dueca namespace")
aparser.add_argument(
    '-D', '--debug', action='store_true',
    help="produce additional debug output on the parsing")
aparser.add_argument(
    '-n', '--namespace', action='append',
    default=[], help="namespace for the object")
aparser.add_argument(
    'dcofile', type=str, nargs='*',
    help="Input file")

pvals = aparser.parse_args(sys.argv[1:])

in_dueca, do_debug, objectnamespace = (
    pvals.dueca, pvals.debug, pvals.namespace)

if in_dueca:
    objectnamespace.insert(0, 'dueca')


class CodegenException(Exception):
    """Identified problem, to be turned into simple message."""


class TypeNotDefined(CodegenException):
    """The specified/wanted type cannot be found."""
    pass


class CombinationNotPossible(CodegenException):
    """This combination of options is not possible."""
    pass


class ParserProblem(CodegenException):
    """Cannot parse the input"""
    pass


class AvoidTheseTypes(CodegenException):
    """Avoid int, unsigned, etc. type names, since their size can vary"""
    pass


class CodegenImproperConfig(CodegenException):
    """The configuration (path to DUECA files) is not correct"""
    pass


codegen_version = None

# extend the search path if not in dueca
def setup_vars():
    global codegen_version, plugins, enum_plugins
    global summarise_member

    # check that we are not called from dueca build
    filepath = os.path.dirname(os.path.abspath(__file__))
    if not os.path.isfile(filepath + os.sep + 'generation.py.in'):
        try:
            fsdp = os.popen('dueca-config --path-datafiles 2>/dev/null')
            sys.path.append(fsdp.readline().strip())
            fsdp.close()
            # add support files
            from pycodegen import codegen_version
            # insert dueca installed plugins
            from DCOplugins import plugins
            from EnumPlugins import plugins as enum_plugins

            # check home dir?
            for plist, pdir in ((plugins, 'DCOplugins'),
                                (enum_plugins, 'EnumPlugins')):

                hplugs = os.path.expanduser("~")+f"/.config/dueca/{pdir}"
                if os.path.isdir(hplugs):
                    for m in os.listdir(hplugs):
                        if m == '__init__.py' or m[-3:] != '.py':
                            continue
                        #print(m)
                        mname = m[:-3]
                        #print(mname)
                        spec = importlib.util.spec_from_file_location(
                            mname, f'{hplugs}/{mname}.py')
                        plist[mname] = importlib.util.module_from_spec(spec)

            from duecautils.codegen import summarise_member
            # print(plugins)
            return
        except Exception as e:
            print("cannot find helper files")
            raise e

    # called from dueca build
    sys.path.append(filepath)   # for fixedhash.py
    sys.path.insert(0, filepath+'/../gitscripts/duecautils')
    # print(sys.path)
    # path for fresh generated generation.py
    pathelts = os.getcwd().split(os.sep)[:-1]
    while pathelts and not \
        os.path.isfile(os.sep.join(pathelts)+os.sep + 'pycodegen' +
                       os.sep+'generation.py'):
        pathelts = pathelts[:-1]
    genpath = os.sep.join(pathelts + ['pycodegen'])
    sys.path.append(genpath)
    from generation import codegen_version
    from DCOplugins import plugins
    from EnumPlugins import plugins as enum_plugins
    from src.codegen import summarise_member


# holder of the current item
current = None
# current list of comments
comments = []
# comments for the general header
headercommentstring = ''
# array dimension
arraydims = {}
# declared types
types = {}
# in dueca or not?
# for printing all kinds of messages


def debugprint(*args):
    global do_debug
    if do_debug:
        print(*args)
    return


def simple_decorator(decorator):
    '''This decorator can be used to turn simple functions
    into well-behaved decorators, so long as the decorators
    are fairly simple. If a decorator expects a function and
    returns a function (no descriptors), and if it doesn't
    modify function attributes or docstring, then it is
    eligible to use this. Simply apply @simple_decorator to
    your decorator and it will automatically preserve the
    docstring and function attributes of functions to which
    it is applied.'''
    def new_decorator(f):
        g = decorator(f)
        g.__name__ = f.__name__
        g.__doc__ = f.__doc__
        g.__dict__.update(f.__dict__)
        return g
    # Now a few lines needed to make simple_decorator itself
    # be a well-behaved decorator.
    new_decorator.__name__ = decorator.__name__
    new_decorator.__doc__ = decorator.__doc__
    new_decorator.__dict__.update(decorator.__dict__)
    return new_decorator


@simple_decorator
def catchguard(func):
    def inner(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except Exception as e:
            print("%s; %s" % (sys._getframe(1).f_code.co_name,
                              traceback.format_exc()))
            raise e
    return inner


@catchguard
def addComment(upto, line, s):
    global comments
    debugprint("comment", s)
    comments.append(s[0])


@catchguard
def headerComments(upto, line, c):
    global headercommentstring, comments
    headercommentstring = c[0]
    comments = []


@catchguard
def addContentComment(upto, line, c):
    global current
    debugprint("content comment", c)
    current.addContentComment(c[0])


@catchguard
def addInherits(upto, line, c):
    global current
    debugprint("inherits", c)
    if c[0] == 'ScriptCreatable':
        # special case, now handled through option
        print("""Converting ScriptCreatable inherit to option, check the
documentation for ScriptCreatableDataHolder to see how you should update
your code. Use (Option ScriptCreatable) instead of inheritance to update
the .dco file""")
        current.addOption(c[0])
    else:
        current.addInherits(c[0])


@catchguard
def addOption(upto, line, c):
    global current
    debugprint("option", c)
    current.addOption(c[0])


@catchguard
def addInclude(upto, line, c):
    global current
    debugprint("include", c)
    current.addInclude(c[0].replace('n\n', '\n'))


@catchguard
def addToHeader(upto, line, c):
    global current
    debugprint("add to header", c)
    current.addToHeader(c[0])


@catchguard
def addConstructorCode(upto, line, c):
    global current

    debugprint("constructorcode", c)
    # the following replace is a quick hack, to stay compatible with
    # old .dco files where a return has to be entered with \n
    current.addConstructorCode(c[0].replace('\\n\n', '\n'))


@catchguard
def addFullArgsConstructorCode(upto, line, c):
    global current
    debugprint("full args constructorcode", c)
    current.addFullArgsConstructorCode(c[0])


@catchguard
def addDimension(upto, line, c):
    global current
    debugprint("dimension", c)
    current.addDimension(c[0])


@catchguard
def addClass(upto, line, c):
    global current
    debugprint("addclass", c)
    current.addClass(c[0])


@catchguard
def addMember(upto, line, c):
    global current
    debugprint("addmember", c)
    current.addMember(c[0])


@catchguard
def setDefault(s, l, t):
    global current

    def flatlist(l):
        ln = []
        for e in l:
            if not isinstance(e, str):
                ln.extend(['(', flatlist(e), ')'])
            else:
                ln.append(e)
        return ''.join(ln)
    debugprint("default", t, flatlist(t))
    current.setDefault(flatlist(t))


@catchguard
def setDefaultArg(s, l, t):
    global current

    def flatlist(l):
        ln = []
        for e in l:
            if not isinstance(e, str):
                ln.extend(['(', flatlist(e), ')'])
            else:
                ln.append(e)
        return ''.join(ln)
    debugprint("defaultarg", t, flatlist(t))
    current.setDefaultArg(flatlist(t))


@catchguard
def setDefaultSize(s, l, t):
    global current

    def flatlist(l):
        ln = []
        for e in l:
            if not isinstance(e, str):
                ln.extend(['(', flatlist(e), ')'])
            else:
                ln.append(e)
        return ''.join(ln)
    debugprint("defaultsize", t, flatlist(t))
    current.setDefaultSize(flatlist(t))


@catchguard
def setName(upto, line, c):
    global current
    debugprint("set name", c)
    current.setName(c[0])

@catchguard
def setTypeName(upto, line, c):
    global current
    debugprint("name", c)
    try:
        if current and current[0] and \
           isinstance(current, Type) and current.isIterable() and \
           c[0].startswith('varvector<') or \
           c[0].startswith['dueca::varvector<'] or \
           c[0].startswith('limvector<') or \
           c[0].startswith['dueca::limvector<']:
            print("Found a varvector, assuming variable")
            current.fixed_size = False
    except Exception as e:
        #print(f"Trying to decode as string {current}, error{e}")
        pass
    current.setName(c[0])


@catchguard
def complete(upto, line, c):
    global current
    debugprint("complete", c)
    current.complete()


@catchguard
def completeMember(upto, line, c):
    global current
    debugprint("completemember", c)
    current.completeMember()


@catchguard
def addEnumMember(upto, line, c):
    global current
    debugprint("enum member", c)
    current.addEnumMember(c[0])

@catchguard
def addEnumValue(upto, line, c):
    global current
    debugprint("enum value", c)
    current.addEnumValue(c[0])


@catchguard
def setEnumType(upto, line, c):
    global current
    debugprint("enum type", c)
    current.setEnumType(c[0])


@catchguard
def startEnum(*args):
    global current, comments
    debugprint("startenum")
    current = Enum(comments)
    comments = []

@catchguard
def startClassEnum(*args):
    global current, comments
    debugprint("startenum (class)")
    current = Enum(comments, classenum=True)
    comments = []

@catchguard
def startType(*args):
    global current, comments
    debugprint("starttype")
    current = Type(comments)
    comments = []

@catchguard
def startIType(*args):
    global current, comments
    debugprint("startitype")
    current = Type(comments, True, True)
    comments = []

@catchguard
def startIVarType(*args):
    global current, comments
    debugprint("startitype")
    current = Type(comments, True, False)
    comments = []

@catchguard
def startEnumerator(*args):
    global current, comments
    debugprint("startEnumerator")
    current = StandaloneEnum(comments)
    comments = []

@catchguard
def startEventAndStream(*args):
    global current, comments
    debugprint("starteventandstream")
    current = EventAndStream(comments)
    comments = []

@catchguard
def startArraySize(*args):
    global current, comments
    debugprint("startarraysize")
    current = ArraySize(comments)
    comments = []


@catchguard
def startArraySizeEnum(*args):
    global current, comments
    debugprint("startarraysizeenum")
    current = ArraySize(comments, True)
    comments = []

def cnize(pref):
    if pref:
        return pref + '::'
    return ''

@catchguard
def appendNameSpace(upto, line, c):
    global objectnamespace
    debugprint("addnamespace", c)
    objectnamespace.append(c[-1])

@catchguard
def setPackAlignment(upto, line, c):
    global pack_alignment
    debugprint("setPackAlignment", c)
    if c[-1] == '0':
        pack_alignment is None
    else:
        pack_alignment = int(c[-1])

class MemberInfo:

    def __init__(self, name, klass, master, namespace):
        self.name = name
        self.klass = klass
        self.mklass = mklass
        self.namespace = namespace

    def getClass(self, fromwhere='global'):
        if fromwhere == 'global':
            return f'{cnize(self.namespace)}{self.mclass}::{self.klass}'
        elif fromwhere == 'namespace':
            return f'{self.mclass}::{self.klass}'
        else:
            return self.klass

class BuildObject(object):
    def __init__(self, c):
        global codegen_version
        self.name = None
        debugprint("comments argument %s" % c)
        self.comments = [r for r in c]
        self.include = None
        self.codegenversion = codegen_version

        # custom cpp defines, guarding override
        self.customdefines = [
            '__CUSTOM_DEFAULT_CONSTRUCTOR',
            '__CUSTOM_FULL_CONSTRUCTOR',
            '__CUSTOM_FULLSINGLES_CONSTRUCTOR',
            '__CUSTOM_COPY_CONSTRUCTOR',
            '__CUSTOM_AMORPHRESTORE_CONSTRUCTOR',
            '__CUSTOM_ARRAYS_SIZE_INIT_CONSTRUCTOR',
            '__CUSTOM_DESTRUCTOR',
            '__CUSTOM_FUNCTION_PACKDATADIFF',
            '__CUSTOM_FUNCTION_UNPACKDATA',
            '__CUSTOM_FUNCTION_UNPACKDATADIFF',
            '__CUSTOM_FUNCTION_PACKDATA' ]
        pass

    def setName(self, n):
        # debugprint("setting name %s " % n)
        self.name = n
        pass

    def addInclude(self, c):
        self.include = c
        pass

    def complete(self):

        # global variables
        global headercommentstring, in_dueca, types, currentobject

        if currentobject and self.name != currentobject:
            raise ValueError(
                f"Produced objects {self.name}.hxx and {self.name}.cxx"
                f" do not match input file name {currentobject}")

        # specific addition to header
        self.headercomments = headercommentstring

        # debug fixes
        self.debugcmd = (
            do_debug and "#define DOBS(A) std::cerr << (A) << std::endl") or \
            "#define DOBS(A)"

        # data for the header
        try:
            username = pwd.getpwuid(os.getuid())[4].split(',')[0]
            self.maker = "%s (%s)" % (username, getpass.getuser())
        except:
            self.maker = getpass.getuser()
        self.date = datetime.date.today().strftime('%a %d %b %Y')

        # assert include / in dueca or not
        if in_dueca:
            self.assertinclude = "#include <dassert.h>"
        else:
            self.assertinclude = "#include <cassert>"

        # additional include files
        self.extrainclude = (self.include and '''

// Additional custom code
#include "%s.hxx"''' % self.include) or ''

        # transfer the custom defines
        customdefines = r""") || \
    defined(""".join(self.customdefines)

        self.extraincludebody = (self.include and '''

// Additional custom code
#include "%(include)s.cxx"

// if any custom implementations have been defined, for packing,
// unpacking, construction or deletion, verify that the
// custom code is compatible with the current generation
#if defined(%(customdefines)s)
#ifndef __CUSTOM_COMPATLEVEL_%(cg_version)s
#error "Verify that your custom code is compatible with version %(cg_version)s. Then define __CUSTOM_COMPATLEVEL_%(cg_version)s"
#endif
#endif%(plug_body_check)s
''' % dict(include=self.include,cg_version=codegen_version,
           customdefines=customdefines,
           plug_body_check=self.plug_body_check) or '')


class Type(BuildObject):
    """Create a Type: base class for different declarable types.

    a type represent a c++ type that can be used as member variable in
    an object that can be transmitted over a channel.
    """
    avoid = ('long', "size_t", "unsigned long", "long int",
             "unsigned long int", "long unsigned int")

    def __init__(self, c, it=False, fixed=False, vector=False):
        super(Type, self).__init__(c)
        self.master = None
        self.is_iterable = it
        self.is_vector = vector
        self.fixed_size = fixed
        if self.name in Type.avoid:
            raise AvoidTheseTypes()

    def getType(self):
        if self.isIterable():
            return 'IterableType'
        else:
            return 'Type'

    def getCType(self):
        return None

    def getEnums(self):
        return []

    def complete(self):
        # check for problematic types
        if self.name in Type.avoid:
            print(
"""Cannot accept the name {name}, use instead int32_t, uint32_t, etc.
Explanation: long type variables have 4 byte versus 8 byte
             sizes on 32 respectively 64 bit platforms. Transmission of
             these variables among these platforms is problematic""" \
                .format(**self.__dict__))
            sys.exit(1)
        debugprint("adding type", self)
        types[self.name] = self

    def isIterable(self):
        return self.is_iterable

    def isFixed(self):
        return self.fixed_size

    def isVector(self):
        return self.is_vector

    def __str__(self):
        return "Type %(name)s, include %(include)s" % self.__dict__

    def amorphConstructorList(self, dname):
        return """
        %(dname)s(s)""" % locals()

    def amorphPackBody(self, dname):
        return """
  ::packData(s, this->%(dname)s);""" % locals()

    def amorphUnPackBody(self, dname):
        return """
  ::unPackData(s, this->%(dname)s);""" % locals()

    def subTypeCommand(self, depth=''):
        return ''

    def includeCommand(self):
        return self.include

    def publicFunctionsDec(self, master):
        return None

    def publicFunctionsImp(self, master):
        return None

    def enumTraits(self, master):
        return None

    def staticAccessObject(self, mastername, dname):
        return """
static ::dueca::CommObjectMemberAccess
  <%(mastername)s,%(klass)s >
  %(mastername)s_member_%(name)s(&%(mastername)s::%(name)s, "%(name)s");""" \
            % dict(mastername=mastername, klass=self.name,
                   name=dname)

class ArraySize(BuildObject):
    """ArraySize: specification, through define or enum, of an array size.

    after specification of an ArraySize, its name may be used instead of a
    numeric arrray size. Note that this is deprecated; it is preferred to
    use an IterableType"""

    def __init__(self, c, fromenum=False):
        super(ArraySize, self).__init__(c)
        debugprint("start arraysize")
        self.fromenum = fromenum
        self.size = None

    def complete(self):
        global arraydims, compilerflags
        if self.include:
            try:
                os.remove(".determine-size-%(name)s.cxx" % self.__dict__)
            except:
                pass
            try:
                os.remove(".determine-size-%(name)s.x" % self.__dict__)
            except:
                pass
            p = open(".determine-size-%(name)s.cxx" % self.__dict__, 'w')
            p.write("""
#include <iostream>
%(include)s
int main() { std::cout << int(%(name)s); return 0;}
""" % self.__dict__)
            p.close()
            # see if we can get DUECA flags, these are needed in some
            # projects
            try:
                dfs = subprocess.Popen(
                    ['dueca-config', '--cflags'], stdout=subprocess.PIPE)
                dflags = dfs.communicate()[0].split()
            except:
                dflags = []

            # 1st try, use make, this might employ the rules in a makefile
            try:
                subprocess.check_call(
                    ["/usr/bin/make",
                     ".determine-size-%(name)s.x" % self.__dict__,
                     "1>/dev/null",
                     "2>/dev/null"])
            except subprocess.CalledProcessError as e:
                # print("failed to use make", str(e))

                # 2nd try, directly compile with c++
                subprocess.check_call(
                    ["/usr/bin/c++",
                     ".determine-size-%(name)s.cxx" % self.__dict__,
                     "-o", ".determine-size-%(name)s.x" % self.__dict__] +
                    compilerarg + dflags)
                pass

            # now run the new program for getting the variable size
            try:
                ds = subprocess.Popen(
                    ["./.determine-size-%(name)s.x" % self.__dict__],
                    stdout=subprocess.PIPE)
                self.size = int(ds.communicate()[0])
            except subprocess.CalledProcessError:
                print("could not run .determine-size-%(name)s.x"
                      % self.__dict__)
                os.exit(1)
            except ValueError:
                print("invalid output from .determine-size-%(name)s.x"
                      % self.__dict__)
                os.exit(1)
        arraydims[self.name] = self
        debugprint("array dimension", self)
        return

    def __int__(self):
        return self.size

    def __str__(self):
        return "ArrayDim %(name)s %(size)s" % self.__dict__

    def body_check(self, master):
        self.master = master
        return """
struct %(master)s_checksize_%(name)s
{
  // A single instance of this class will be created
  // its function is to check that the size given in the enum
  // or define has not been changed
  %(master)s_checksize_%(name)s()
  {
    if (int(%(name)s) != (%(size)i) /* NORMALLY DISABLED CODE */ ) {
      cerr << "enum or define %(name)s changed size" << endl
           << "declared as %(size)i, now " << int(%(name)s) << endl
           << "check class %(master)s.dco and re-generate" << endl;
      assert(0);
    }
  }
};
static %(master)s_checksize_%(name)s check_%(master)s_%(name)s;
""" % self.__dict__

    def header(self):
        if self.include:
            return '''
%(include)s''' % self.__dict__
        return None


class EnumMember:
    """EnumMember: defines names in an enum definition
    """
    def __init__(self):
        self.comments = []
        self.name = None
        self.value = None
        self.depth = ''

    def complete(self, depth='  '):
        self.depth = depth
        if self.comments:
            self.comments = "{depth}  /** {comments} */".format(
                depth=depth,
                comments='\n{depth}      '.format(depth=depth).join(
                    self.comments))

    def __str__(self):
        return "EnumMember %(name)s" % self.__dict__

    def quoted(self):
        return '"%s"' % self.name

    def subTypeCommand(self, depth='  '):
        res = []
        if self.comments:
            res.append("{comments}\n".format(**self.__dict__))
        if self.value is None:
            res.append("{depth}  {name}".format(**self.__dict__))
        else:
            res.append("{depth}  {name} = {value}".format(**self.__dict__))
        return ''.join(res)


class Enum(Type):
    """Enum: type based on enum definition in c.

    Enum is generated either through an include file or by listing the
    members. in the latter case the enum definition is done in the
    generated class
    """
    def __init__(self, c, classenum=False):
        super(Enum, self).__init__(c)
        self.ctype = None
        self.classenum = classenum
        self.members = [ EnumMember() ]

    def getType(self):
        return 'Enum'

    def getCType(self):
        return self.ctype

    def getEnums(self):
        return [ m.name for m in self.members ]

    def addEnumMember(self, c):
        self.members[-1].name = c
        self.members.append(EnumMember())
        debugprint(self.members)
        pass

    def addEnumValue(self, c):
        self.members[-2].value = c
        pass

    def addContentComment(self, c):
        self.members[-1].comments.append(c)

    def setEnumType(self, c):
        self.ctype = c
        pass

    def complete(self, depth=''):
        debugprint("adding type", self, self.name)
        types[self.name] = self
        self.comments = "{depth}/**{comments} */".format(
            depth=depth,
            comments='\n{depth}   '.format(depth=depth).join(self.comments) or
             ' Enumerated type for an automatically generated object class')
        self.members.pop()
        for m in self.members:
            m.complete(depth+'  ')
        pass

    def __str__(self):
        return "Enum %(name)s, include %(include)s" % self.__dict__

    def amorphConstructorList(self, dname):
        return """
        %(dname)s(%(name)s(%(ctype)s(s)))""" % joindict(self.__dict__, locals())

    def subTypeCommand(self, depth='  '):
        debugprint(self.members)
        if not self.members:
            # external type
            return ''

        #elts = [ (m.comments and "%(comments)s\n    %(name)s" % m.__dict__) or
        #         "    %(name)s" % m.__dict__ for m in self.members ]
        #        elts = [ "    %s" % m.name for m in self.members ]
        elts = [ m.subTypeCommand(depth) for m in self.members ]
        if self.classenum:
            return """
%s
  enum class %s {
%s
  };
""" % (self.comments, self.name, ',\n'.join(elts) )
        else:
            return """
%s
  enum %s {
%s
  };
""" % (self.comments, self.name, ',\n'.join(elts) )

    def typeCommand(self, master):
        elts = [ m.subTypeCommand(depth='') for m in self.members ]
        if self.classenum:
            return r"""
{comments}
enum class {name} {{
{elements}
}};""".format(comments=self.comments, name=self.name,
              elements=',\n'.join(elts))
        else:
            return r"""
{comments}
enum {name} {{
{elements}
}};""".format(comments=self.comments, name=self.name,
              elements=',\n'.join(elts))

    def publicFunctionsDec(self, master):
        if not self.members:
            # this might have to move to publicFunctionsImp???
            return None
        res = [ """
%(namespacecmd0)sconst char* const getString(const %(objprefix)s%(masterprefix)s%(name)s &o);
void readFromString(%(objprefix)s%(masterprefix)s%(name)s &o, const std::string& s);
void getFirst(%(objprefix)s%(masterprefix)s%(name)s &o);
bool getNext(%(objprefix)s%(masterprefix)s%(name)s &o);%(namespacecmd1)s
#if !defined(__DCO_NOPACK)
void packData(::dueca::AmorphStore& s,
              const %(objprefix)s%(masterprefix)s%(name)s &o);
void unPackData(::dueca::AmorphReStore& s,
                %(objprefix)s%(masterprefix)s%(name)s &o);
#endif
namespace dueca {
  /** This function retrieves the classname of an %(masterprefix)s%(name)s */
  template <>
  const char* getclassname<%(masterprefix)s%(name)s>();
};

PRINT_NS_START;
/** Print an object of type %(masterprefix)s%(name)s */
inline std::ostream&
operator << (std::ostream& s, const %(objprefix)s%(masterprefix)s%(name)s& o)
{ return s << getString(o); }
/** Read an object of type %(masterprefix)s%(name)s */
inline std::istream&
/** Read an enum of type %(masterprefix)s%(name)s */
operator >> (std::istream& s, %(objprefix)s%(masterprefix)s%(name)s& o)
{ std::string tmp; s >> tmp; readFromString(o, tmp); return s; }
PRINT_NS_END;
""" % joindict(self.__dict__, { 'mastername' : master.name,
                                'masterprefix' :
                                (master.name and f'{master.name}::') or '',
                                'objprefix' : master.objprefix,
                                'namespacecmd0' : master.namespacecmd0,
                                'namespacecmd1' : master.namespacecmd1 }) ]
        return ''.join(res)

    def publicFunctionsImp(self, master):
        if not self.members:
            return None
        anyval = [ 1 for i in self.members if i.value is not None]
        if anyval:
            res = [ """
{namespacecmd0}#if !defined(__CUSTOM_ENUMNAMES_{name})
struct NameMatch_{mastername}_{name} {{
  const char* mname;
  {objprefix}{masterprefix}{name}      enumval;
}};

static const NameMatch_{mastername}_{name} __{name}_names [] = {{\n""".format(
    name=self.name, namespacecmd0=master.namespacecmd0,
    objprefix=master.objprefix, mastername=master.name,
    masterprefix=(master.name and f'{master.name}::') or ''
)]

            # append all values
            for m in self.members:
                res.append('  {{ "{m.name}", {objprefix}{masterprefix}{prefix}{m.name} }},\n'.format(
                    m=m,
                    prefix=(self.classenum and f'{self.name}::') or '',
                    objprefix=master.objprefix,
                    masterprefix=(master.name and f'{master.name}::') or ''))

            # complete and add the functions
            res.append("""  {{ NULL }}
}};
#endif

#ifndef __CUSTOM_GETSTRING_{name}
const char* const getString(const {objprefix}{masterprefix}{name} &o)
{{
  for (auto ii = __{name}_names; ii->mname; ii++) {{
    if (o == ii->enumval) {{
      return ii->mname;
    }}
  }}
  throw({inclassprefix}ConversionNotDefined());
}}
#endif

#ifndef __CUSTOM_READFROMSTRING_{name}
void readFromString({objprefix}{masterprefix}{name} &o, const std::string& s)
{{
  for (auto ii = __{name}_names; ii->mname; ii++) {{
    if (std::string(ii->mname) == s) {{
      o = ii->enumval;
      return;
    }}
  }}
  throw({inclassprefix}ConversionNotDefined());
}}
#endif

#ifndef __CUSTOM_ITERATE_{name}
void getFirst({objprefix}{masterprefix}{name} &o)
{{
  o = __{name}_names[0].enumval;
}}

bool getNext({objprefix}{masterprefix}{name} &o)
{{
  bool next = false;
  for (const auto &pair: __{name}_names) {{
    if (pair.mname == NULL) {{ return false; }}
    if (next) {{
      o = pair.enumval; return true;
    }}
    if (pair.enumval == o) {{ next = true; }}
  }}
  return false;
}}
#endif{namespacecmd1}

#if !defined(__CUSTOM_PACKDATA_{name}) && !defined(__DCO_NOPACK)
void packData(::dueca::AmorphStore& s,
              const {objprefix}{masterprefix}{name} &o)
{{ packData(s, {ctype}(o)); }}
#endif

#if !defined(__CUSTOM_UNPACKDATA_{name}) && !defined(__DCO_NOPACK)
void unPackData(::dueca::AmorphReStore& s,
                {objprefix}{masterprefix}{name} &o)
{{ {ctype} tmp(s); o = {objprefix}{masterprefix}{name}(tmp); }}
#endif

namespace dueca {{
template <>
const char* getclassname<{masterprefix}{name}>()
{{ return "{masterprefix}{name}"; }}
}};
""".format(name=self.name, objprefix=master.objprefix,
           masterprefix=(master.name and f'{master.name}::') or '',
           inclassprefix=master.inclassprefix,
           namespacecmd1=master.namespacecmd1,
           ctype=self.ctype))
        else:
            res = ["""
#include <map>
{namespacecmd0}#ifndef __CUSTOM_ENUMNAMES_{name}
struct NameMatch_{name} {{
  const char* mname;
  {objprefix}{masterprefix}{name}      enumval;
}};

static const NameMatch_{name} __{name}_names [] = {{""".format(
    name=self.name, namespacecmd0=master.namespacecmd0,
    objprefix=master.objprefix,
    masterprefix=(master.name and f'{master.name}::') or ''
)]
            for m in self.members:
                res.append('''
  {{ "{m.name}", {objprefix}{masterprefix}{prefix}{m.name} }},'''.format(
                    m=m,
                    prefix=(self.classenum and f'{self.name}::') or '',
                    objprefix=master.objprefix,
                    masterprefix=(master.name and f'{master.name}::') or ''))

            res.append('''
  {{ NULL }}
}};
#endif

#ifndef __CUSTOM_GETSTRING_{name}
const char* const getString(const {objprefix}{masterprefix}{name} &o)
{{
  for (auto ii = __{name}_names; ii->mname; ii++) {{
    if (o == ii->enumval) {{
      return ii->mname;
    }}
  }}
  throw({inclassprefix}ConversionNotDefined());
}}
#endif

#ifndef __CUSTOM_READFROMSTRING_{name}
void readFromString({objprefix}{masterprefix}{name} &o, const std::string& s)
{{
  for (auto ii = __{name}_names; ii->mname; ii++) {{
    if (std::string(ii->mname) == s) {{
      o = ii->enumval;
      return;
    }}
  }}
  throw({inclassprefix}ConversionNotDefined());
}}
#endif

#ifndef __CUSTOM_ITERATE_{name}
void getFirst({objprefix}{masterprefix}{name} &o)
{{
  o = __{name}_names[0].enumval;
}}

bool getNext({objprefix}{masterprefix}{name} &o)
{{
  bool next = false;
  for (const auto &pair: __{name}_names) {{
    if (pair.mname == NULL) {{ return false; }}
    if (next) {{
      o = pair.enumval; return true;
    }}
    if (pair.enumval == o) {{ next = true; }}
  }}
  return false;
}}
#endif{namespacecmd1}

#if !defined(__CUSTOM_PACKDATA_{name}) && !defined(__DCO_NOPACK)
void packData(::dueca::AmorphStore& s,
              const {objprefix}{masterprefix}{name} &o)
{{ packData(s, {ctype}(o)); }}
#endif

#if !defined(__CUSTOM_UNPACKDATA_{name}) && !defined(__DCO_NOPACK)
void unPackData(::dueca::AmorphReStore& s,
                {objprefix}{masterprefix}{name} &o)
{{ {ctype} tmp(s); o = {objprefix}{masterprefix}{name}(tmp); }}
#endif

namespace dueca {{
template <>
const char* getclassname<{masterprefix}{name}>()
{{ return "{masterprefix}{name}"; }}
}};
'''.format(name=self.name,
           mastername = master.name,
           masterprefix = (master.name and f'{master.name}::') or '',
           objprefix = master.objprefix,
           namespacecmd0=master.namespacecmd0,
           namespacecmd1=master.namespacecmd1,
           inclassprefix=master.inclassprefix,
           ctype=self.ctype))
        return ''.join(res)

    def enumTraits(self, mastername):
        if not self.members:
            return None
        return """
template <>
struct dco_nested< %(masterprefix)s%(name)s > : public dco_isenum { };""" % \
    joindict(self.__dict__, {'masterprefix' :
                             (mastername and f'{mastername}::') or ''})

    def staticAccessObject(self, mastername, dname):
        if self.members:
            return """
static ::dueca::CommObjectMemberAccess
  <%(mastername)s,%(klass)s >
  %(mastername)s_member_%(name)s(&%(mastername)s::%(name)s, "%(name)s");""" \
                % dict(mastername=mastername,
                       klass= '::'.join((mastername, self.name)), name=dname)

        # otherwise take the default
        return Type.staticAccessObject(self, mastername, dname)

def joindict(a, b):
    res = a.copy()
    res.update(b)
    return res

class MemberBase(object):
    def __init__(self, master, comments, name, klassref,
                 default=None, defaultarg=''):
        self.mastername = master.name
        self.comments = '\n      '.join(comments or ['A class member'])
        self.name = name
        self.klass = klassref.name
        self.klassref = klassref
        self.default = default
        self.defaultarg = defaultarg

    def calculateMagic(self, magic):
        for c in self.klass:
            magic = ROR(magic) ^ ord(c)
        for c in self.name:
            magic = ROR(magic) ^ ord(c)
        return magic

    # default arguments for the "empty" constructor
    def defaultConstructorArgumentDec(self):
        return None

    # same arguments, but without the = part
    def defaultConstructorArgumentImp(self):
        return None

    # initialization list for the default constructor
    def defaultConstructorList(self):
        return None

    # in-body initialization default constructor
    def defaultConstructorBody(self):
        return None

    # create the declaration of the member
    def declareMemberInClass(self):
        return """
  /** %(comments)s */
  %(klass)s %(name)s;
""" % self.__dict__

    # initialization list for the copy constructor
    def copyConstructorList(self):
        return """
    %(name)s(other.%(name)s)""" % self.__dict__

    # in-body actions copy constructor
    def copyConstructorBody(self):
        return None

    # argument in the complete (all arguments) constructor at declaration
    def completeConstructorArgumentDec(self):
        return None

    # argument in the complete (all arguments) constructor at implementation
    def completeConstructorArgumentImp(self):
        return None

    # initialization list complete constructor
    def completeConstructorList(self):
        return None

    # argument in the complete (single arguments) constructor at declaration
    def completeConstructorSingleArgumentDec(self):
        return None

    # argument in the complete (single arguments) constructor at impl
    def completeConstructorSingleArgumentImp(self):
        return None

    # initialization list complete constructor
    def completeConstructorSingleList(self):
        return None

    # initialization list array size init
    def arraySizeInitConstructorArgumentDec(self):
        return None

    # initialization list array size init
    def arraySizeInitConstructorArgumentImp(self):
        return None

    # initialization list array size init
    def arraySizeInitConstructorList(self):
        return None

    # in-body actions complete constructor
    def completeConstructorBody(self):
        return None

    # initialization list amorph constructor
    def amorphConstructorList(self):
        return None

    # in-body actions amorph constructor
    def amorphConstructorBody(self):
        return None

    # in-body actions destructor
    def destructorBody(self):
        return None

    # 1st series actions packData function
    def amorphPackFirst(self):
        return self.klassref.amorphPackBody(self.name)

    # 1st series actions unPackData function
    def amorphUnpackFirst(self):
        return self.klassref.amorphUnPackBody(self.name)

    # 2nd series actions packData function
    def amorphPackSecond(self):
        return None

    # 2nd series actions unPackData function
    def amorphUnpackSecond(self):
        return None

    # actions diff pack function
    def amorphPackDiff(self):
        return None

    # actions diff unpack function
    def amorphUnpackDiff(self):
        return None

    # operator equal test
    def operatorEqual(self):
        return """
  if (this->%(name)s != other.%(name)s) return false;""" % self.__dict__

    # assign action
    def operatorAssign(self):
        return """
  this->%(name)s = other.%(name)s;""" % self.__dict__

    # print action
    def operatorPrint(self, islast):
        return None

    # call for creating a generic access object
    def staticAccessObject(self):
        return self.klassref.staticAccessObject(self.mastername, self.name)

    # entry in the generic access table
    def accessTableEntry(self):
        return """
  { &%(mastername)s_member_%(name)s }""" % self.__dict__

    # entry in the parameter table, for scheme-creatables
    def parameterTable(self):
        self.schemename = self.name.replace('_', '-')

        self.scomments = r'''\n"
      "'''.join(self.comments.split('\n'))
        if self.klassref.getType() == 'Enum':
            return '''
    { TABNAME("set-%(schemename)s"),
      new MemberCall<%(mastername)s,std::string>
        (&%(mastername)s::doFromParameterTableSet_%(name)s),
      "%(scomments)s" }''' % self.__dict__

        return '''
    { TABNAME("set-%(schemename)s"),
      new VarProbe<%(mastername)s,%(klass)s >(&%(mastername)s::%(name)s),
      "%(scomments)s" }''' % self.__dict__

    def setFunctionDec(self):
        if self.klassref.getType() == 'Enum':
            return '''

  /** Set the parameter "%(name)s" from a string */
  bool doFromParameterTableSet_%(name)s(const std::string& n);''' \
        % self.__dict__
        return ''

    def setFunctionImp(self):
        if self.klassref.getType() == 'Enum':
            return '''

bool %(mastername)s::doFromParameterTableSet_%(name)s(const std::string& n)
{
  try {
    readFromString(%(name)s, n);
  }
  catch(const dueca::ConversionNotDefined& e) {
    std::cerr << "Cannot interpret " << n << std::endl;
    return false;
  }
  return true;
}''' % self.__dict__
        return ''

class SingleMember(MemberBase):
    def __init__(self, master, comments, name, klassref, default, defaultarg):
        super(SingleMember, self).__init__(
            master, comments, name, klassref, default, defaultarg)

    def defaultConstructorArgumentDec(self):
        if self.defaultarg:
            return self.completeConstructorSingleArgumentDec()
        return None

    def defaultConstructorArgumentImp(self):
        if self.defaultarg:
            return self.completeConstructorArgumentImp()
        return None

    def defaultConstructorList(self):
        if self.defaultarg:
            return self.completeConstructorList()
        elif self.default:
            return """
    %(name)s(%(default)s)""" % self.__dict__
        return None

    # initialization list array size init
    def arraySizeInitConstructorList(self):
        if self.default:
            return """
    %(name)s(%(default)s)""" % self.__dict__
        elif self.defaultarg:
            return """
    %(name)s(%(defaultarg)s)""" % self.__dict__
        return None

    # def defaultConstructorBody(self):
    # def declareMemberInClass(self):
    # def copyConstructorList(self):
    # def copyConstructorBody(self):

    def completeConstructorArgumentDec(self):
        return """
        const %(klass)s& %(name)s""" % self.__dict__

    def completeConstructorArgumentImp(self):
        return """
        const %(klass)s& %(name)s""" % self.__dict__

    def completeConstructorList(self):
        return """
    %(name)s(%(name)s)""" % self.__dict__

    def completeConstructorSingleArgumentDec(self):
        return """
        const %(klass)s& %(name)s%(defaultarg)s""" % self.__dict__

    def completeConstructorSingleArgumentImp(self):
        return self.completeConstructorArgumentImp()

    def completeConstructorSingleList(self):
        return self.completeConstructorList()

    # completeConstructorBody(self):

    def amorphConstructorList(self):
        return self.klassref.amorphConstructorList(self.name)

    # def amorphConstructorBody(self):
    # def destructorBody(self):
    # def amorphPackFirst(self):
    # def amorphUnpackFirst(self):
    # def amorphPackSecond(self):
    # def amorphUnpackSecond(self):

    def amorphPackDiff(self):
        return  """
  ::dueca::checkandpackdiffsingle(this->%(name)s, ref.%(name)s,
                         s, im);""" % self.__dict__

    def amorphUnpackDiff(self):
        return """
  ::dueca::checkandunpackdiffsingle(this->%(name)s, s, im);""" % self.__dict__

    # def operatorEqual(self):
    # def operatorAssign(self):

    def operatorPrint(self, islast):
        return """
    << "%(name)s=" << this->%(name)s%(comma)s""" % \
            dict(name=self.name, comma={False: " << ','", True: ""}[islast])

    # def staticAccessObject(self):
    # def accessTableEntry(self):
    # def parameterTable(self):

class IterableMember(MemberBase):
    def __init__(self, master, comments, name, klassref,
                 default, defaultsize, defaultarg):
        self.defaultsize = defaultsize
        super(IterableMember, self).__init__(
            master, comments, name, klassref, default, defaultarg)

    def defaultConstructorArgumentDec(self):
        if self.defaultarg:
            return self.completeConstructorArgumentDec()
        elif self.defaultsize is not None:
            return """
    size_t %(name)s_size = %(defaultsize)s""" % self.__dict__
        return None

    def defaultConstructorArgumentImp(self):
        if self.defaultarg:
            return self.completeConstructorArgumentImp()
        elif self.defaultsize is not None:
            return """
    size_t %(name)s_size""" % self.__dict__
        return None

    def defaultConstructorList(self):
        if self.defaultarg:
            return """
    %(name)s(%(name)s)""" % self.__dict__
        elif self.defaultsize is not None and not self.default:
            return """
    %(name)s(%(name)s_size)""" % self.__dict__
        elif self.defaultsize is not None and self.default:
            return """
    %(name)s(%(name)s_size, %(default)s)""" % self.__dict__
        elif self.default:
            return """
    %(name)s(%(default)s)""" % self.__dict__

    # def defaultConstructorBody(self):
    # def declareMemberInClass(self):
    # def copyConstructorList(self):
    # def copyConstructorBody(self):

    def completeConstructorArgumentDec(self):
        if self.defaultarg:
            return """
        const %(klass)s& %(name)s%(defaultarg)s""" % self.__dict__
        return """
        const %(klass)s& %(name)s""" % self.__dict__

    # argument in the complete (all arguments) constructor at implementation
    def completeConstructorArgumentImp(self):
        return """
        const %(klass)s& %(name)s""" % self.__dict__

    # initialization list complete constructor
    def completeConstructorList(self):
        return """
    %(name)s(%(name)s)""" % self.__dict__

    # def completeConstructorBody(self):
    # def amorphConstructorList(self):
    # initialization list array size init

    def arraySizeInitConstructorArgumentDec(self):
        if not self.klassref.fixed_size:
            return """
            const dueca::DataWriterArraySize& %(name)s_size"""  % self.__dict__
        return None

    # initialization list array size init
    def arraySizeInitConstructorArgumentImp(self):
        if not self.klassref.fixed_size:
            return """
            const dueca::DataWriterArraySize& %(name)s_size""" % self.__dict__
        return None

    # initialization list array size init
    def arraySizeInitConstructorList(self):
        if not self.klassref.fixed_size:
            if self.default:
                return """
            %(name)s(%(name)s_size.size, %(default)s)""" % self.__dict__
            else:
                return """
            %(name)s(%(name)s_size.size)""" % self.__dict__
        return None

    def amorphConstructorBody(self):
        return self.amorphUnpackSecond();

    def amorphPackFirst(self):
        return None

    def amorphUnpackFirst(self):
        return None

    def amorphPackSecond(self):
        return """
  ::dueca::packiterable(s, this->%(name)s,
                        dueca::pack_traits<%(klass)s >()); """ % \
            self.__dict__

    def amorphUnpackSecond(self):
        return """
  ::dueca::unpackiterable(s, this->%(name)s,
                          dueca::pack_traits<%(klass)s >()); """ % \
            self.__dict__

    def amorphPackDiff(self):
        return """
  ::dueca::checkandpackdiffiterable(this->%(name)s, ref.%(name)s, s, im,
                          dueca::diffpack_traits<%(klass)s >());""" % \
            self.__dict__

    def amorphUnpackDiff(self):
        return """
  ::dueca::checkandunpackdiffiterable(this->%(name)s, s, im,
                          dueca::diffpack_traits<%(klass)s >());""" % \
            self.__dict__

    # def operatorEqual(self):
    # def operatorAssign(self):

    def operatorPrint(self, islast):
        return '''
    << "%(name)s={";
  for (%(klass)s::const_iterator ii = this->%(name)s.begin();
       ii != this->%(name)s.end(); ii++) {
    if (ii != this->%(name)s.begin()) s << ',';
    s << (*ii);
  }
  s << "}%(comma)s"''' % dict(name=self.name, klass=self.klass,
                              comma={False: ",", True: ""}[islast])

    # def staticAccessObject(self):
    # def accessTableEntry(self):
    # def parameterTable(self):

class InheritanceMember(MemberBase):
    def __init__(self, master, klassref):
        super(InheritanceMember, self).__init__(
            master, None, '', klassref)
        self.safeklass = self.klass.replace(':','_')

    def calculateMagic(self, magic):
        magic = super(InheritanceMember, self).calculateMagic(magic)
        magic = ROR(magic) ^ ord('I')
        return magic

    def defaultConstructorArgumentDec(self):
        if self.defaultarg:
            return """
        const %(klass)s& parent_%(safeklass)s%(defaultarg)s""" % self.__dict__
        return None

    def defaultConstructorArgumentImp(self):
        if self.defaultarg:
            return """
        const %(klass)s& parent_%(safeklass)s""" % self.__dict__
        return None

    def defaultConstructorList(self):
        if self.defaultarg:
            return """
        %(klass)s(parent_%(safeklass)s)""" % self.__dict__
        elif self.default:
            return """
        %(klass)s(%(default)s)""" % self.__dict__
        return None

    # def defaultConstructorBody(self):

    def declareMemberInClass(self):
        return None

    def copyConstructorList(self):
        return """
        %(klass)s(other)""" % self.__dict__

    # def copyConstructorBody(self):

    def completeConstructorArgumentDec(self):
        return """
        const %(klass)s& parent_%(safeklass)s%(defaultarg)s""" % self.__dict__

    def completeConstructorArgumentImp(self):
        return """
        const %(klass)s& parent_%(safeklass)s""" % self.__dict__

    def completeConstructorList(self):
        return """
        %(klass)s(parent_%(safeklass)s)""" % self.__dict__

    # def completeConstructorBody(self):

    def amorphConstructorList(self):
        return """
        %(klass)s(s)""" % self.__dict__

    # def amorphConstructorBody(self):
    # def destructorBody(self):

    def amorphPackFirst(self):
        return """
        this->%(klass)s::packData(s);""" % self.__dict__

    # def amorphPackSecond(self):

    def amorphUnpackFirst(self):
        return """
        this->%(klass)s::unPackData(s);""" % self.__dict__

    # def amorphUnpackSecond(self):

    def amorphPackDiff(self):
        return """
        this->%(klass)s::packDataDiff(s, ref);""" % self.__dict__

    def amorphUnpackDiff(self):
        return """
        this->%(klass)s::unPackDataDiff(s);""" % self.__dict__

    def operatorEqual(self):
        return """
  if (this->%(klass)s::operator != (other)) return false;""" % \
            self.__dict__

    def operatorAssign(self):
        return """
        this->%(klass)s::operator=(other);""" % self.__dict__

    def operatorPrint(self, islast):
        return ''';
  this->%(klass)s::print(s);
  s''' % dict(klass=self.klass, comma={False: " << ','", True: ""}[islast])

    def staticAccessObject(self):
        return None

    def accessTableEntry(self):
        return None

class Member:
    def __init__(self):
        self.comments = []
        self.name = None
        self.klass = None
        self.size = None
        self.default = None
        self.defaultsize = None
        self.defaultarg = ''

    def createMember(self, master):
        try:
            klassref = types[self.klass]
        except:
            raise TypeNotDefined(
                "Creating member '%(name)s': undefined type '%(klass)s'" %
                self.__dict__)

        if self.size != None:
            klass = self.klass
            size =  self.size
            space = (self.klass[-1] == '>' and ' ') or ''
            if size > 0:
                # convert the old size spec to a class that uses a
                # fixediterable:
                nclass = "dueca::fixvector<%(size)i,%(klass)s%(space)s>" % \
                    locals()
                if not nclass in types:
                    nt = Type(klassref.comments, it=True, fixed=True)
                    nt.setName(nclass)
                    nt.addInclude((klassref.include and
                        '\n'.join([klassref.include,
                                   '#include <dueca/fixvector.hxx>'])) or
                                   '#include <dueca/fixvector.hxx>')
                    nt.complete()
                print("Converting an old c-style fixed-length array "
                      "to use %(nclass)s\n"
                      "Please convert the dco file declaring an IterableType"
                      % locals())
                # add the type to the new DCO class types
                if master.types.get(klass, None) is None:
                    master.types[klass] = klassref

            elif size < 0:
                # convert the old size spec to a class that uses a
                # variterable:
                nclass = "dueca::varvector<%(klass)s%(space)s>" % locals()
                if not nclass in types:
                    nt = Type(klassref.comments, it=True, fixed=False)
                    nt.setName(nclass)
                    nt.addInclude((klassref.include and
                        '\n'.join([klassref.include,
                                   '#include <dueca/varvector.hxx>'])) or
                                   '#include <dueca/varvector.hxx>')
                    nt.complete()
                print("Converting an old c-style variable-length array "
                      "to use %(nclass)s\n"
              "Please convert the dco file declaring an IterableVarSizeType"
                      % locals())
                # add the type to the new DCO class types
                if master.types.get(klass, None) is None:
                    master.types[klass] = klassref

            # modify the original class reference to the fixed or var
            # vector
            self.klass = nclass
            klassref = types[self.klass]

        if klassref.isIterable():

            # if possible, simple check whether enum members are included
            # in template
            if self.klass[-1] == '>':
                try:
                    parts = self.klass[:-1].split('<')[1].split(',')
                    for p in parts:
                        if p in types and not p in master.types:
                            master.types[p] = types[p]
                except Exception as e:
                    print("analysing class template %s, encountered %s" %
                          (self.klass, str(e)))

            return IterableMember(
                master, self.comments, self.name, klassref,
                self.default, self.defaultsize, self.defaultarg)
        else:
            return SingleMember(
                master, self.comments, self.name, klassref,
                self.default, self.defaultarg)

def ROR(c):
    return (c >> 1) | ((c & 0x1) << 31)

def scriptNameFormat(name):
    newname = []
    prevupper = 1
    for c in list(name)[-1::-1]:
        if c.lower() == c and prevupper > 1:
            newname.insert(0, '-')
        if c == '_':
            newname.insert(0, '-')
        else:
            newname.insert(0, c.lower())
        if c.lower() != c and not prevupper:
            newname.insert(0, '-')
        if c.lower() != c:
            prevupper += 1
        else:
            prevupper = 0
    if newname[0] == '-':
        newname.pop(0)
    return ''.join(newname)

def constructorindent(c):
    l = c.split('\n')
    if l[0] != '':
        l.insert(0, '')
    return '\n  '.join(l)

class Channel(BuildObject):
    def __init__(self, c):
        global objectnamespace
        super(Channel, self).__init__(c)
        self.memberprotos = [ Member() ]
        self.parent = None
        self.constructorcode = ''
        self.fullargsconstructorcode = ''
        self.extraheader = ''
        self.options = []
        # defaults for printing
        if objectnamespace:
            # namespace commands surrounding the generated code
            self.namespace = '::'.join(objectnamespace)
            self.namespacecmd0 = ''.join(
                ['namespace %s {\n' % ns for ns in objectnamespace])
            self.namespacecmd1 = '\n' + '} '*len(objectnamespace) + '\n'
            # prefix references to generated class with namespace
            self.objprefix = self.namespace + "::"
            # reference other dueca namespace classes with this prefix
            self.inclassprefix = "::dueca::"
        else:
            self.namespace = ''
            self.namespacecmd0 = ''
            self.namespacecmd1 = ''
            self.objprefix = ''
            self.inclassprefix = "dueca::"
        pass

    def addContentComment(self, c):
        self.memberprotos[-1].comments.append(c)
        pass

    def addInherits(self, c):
        self.parent = c
        pass

    def addToHeader(self, c):
        self.extraheader = c
        pass

    def addOption(self, c):
        self.options.append(c)
        pass

    def addConstructorCode(self, c):
        self.constructorcode = constructorindent(c)
        pass

    def addFullArgsConstructorCode(self, c):
        self.fullargsconstructorcode = constructorindent(c)
        pass

    def addClass(self, c):
        self.memberprotos[-1].klass = c
        pass

    def addMember(self, c):
        self.memberprotos[-1].name = c
        #self.memberprotos.append(Member())
        pass

    def completeMember(self):
        self.memberprotos.append(Member())
        pass

    def setDefault(self, c):
        self.memberprotos[-1].default = c

    def setDefaultArg(self, c):
        self.memberprotos[-1].defaultarg = ' = ' + c

    def setDefaultSize(self, c):
        self.memberprotos[-1].defaultsize = c

    def addDimension(self, c):
        try:
            self.memberprotos[-1].size = int(c)
            return
        except:
            print("dimension %s is not int" % c)

        try:
            self.memberprotos[-1].size = int(arraydims[c])
            return
        except:
            print("dimension '%s' was not defined")

    def complete(self):

        global headercommentstring, in_dueca

        self.memberprotos.pop()
        self.types = {}
        self.members = [ r.createMember(self) for r in self.memberprotos ]
        self.idx = 0
        for r in self.members:
            self.types[r.klass] = r.klassref
        if self.parent:
            try:
                self.members.insert(
                    0, InheritanceMember(self, types[self.parent]))
            except KeyError:
                print("Cannot find class %(parent)s, please define the type" %
                      self.__dict__)

            self.types[self.parent] = self.members[0].klassref
            self.registrarparent = '%(parent)s::classname' % self.__dict__
        else:
            self.registrarparent = 'NULL'

        if not self.comments:
            self.comments = [ ' An automatically generated class\n' ]

        # array sizes
        self.arraydimsdec = ''.join(
            [r.header() for r in iter(arraydims.values()) if
             r.header()])


        # implementation of the array dimension checks
        self.arraydimsimp = ''.join(
            [r.body_check(self.name) for r in iter(arraydims.values()) if
             r.body_check(self.name)])

        # includes for the used types
        self.typeincludes = '\n'.join(
            [r.includeCommand() for r in iter(self.types.values()) if
             r.includeCommand()])


        # prepare a memberlist for plugin use
        memberlist = [
            dict(name=m.name, klass=m.klassref.name,
                 mtype=m.klassref.getType(), ctype=m.klassref.getCType(),
                 enums=m.klassref.getEnums() )
            for m in self.members if not isinstance(m, InheritanceMember)]

        memberlist = [
            summarise_member(m, self.name, self.namespace)
            for m in self.members if not isinstance(m, InheritanceMember)
            ]


        # Plugin options
        self.plug_header_include = []
        self.plug_header_classcode = []
        self.plug_header_code = []
        self.plug_body_include = []
        self.plug_body_check = []
        self.plug_body_code = []
        for p in (set(self.options) & set(plugins.keys())):
            pcode = plugins[p].AddOn(
                self.namespace, self.name, self.parent, memberlist)
            self.plug_header_include.append(pcode.printHeaderInclude())
            self.plug_header_classcode.append(pcode.printHeaderClassCode())
            self.plug_header_code.append(pcode.printHeaderCode())
            self.plug_body_include.append(pcode.printBodyInclude())
            self.plug_body_check.append(pcode.printBodyCheck())
            self.plug_body_code.append(pcode.printBodyCode())
            self.customdefines.extend(pcode.getCustomDefines())

        # convert to single strings
        self.plug_header_include = ''.join(self.plug_header_include)
        self.plug_header_classcode = ''.join(self.plug_header_classcode)
        self.plug_header_code = ''.join(self.plug_header_code)
        self.plug_body_include = ''.join(self.plug_body_include)
        self.plug_body_check = ''.join(self.plug_body_check)
        self.plug_body_code = ''.join(self.plug_body_code)

        # common parts
        super(Channel, self).complete()

        # scriptcreatable is currently the second option
        if 'ScriptCreatable' in self.options:
            if self.parent:
                raise CombinationNotPossible(
                    '''Cannot combine ScriptCreatable with parent class''')
            '''
            self.inherits = """
#if !defined(__DCO_STANDALONE)
: public dueca::ScriptCreatable
#endif"""
            '''

            self.lcname = scriptNameFormat(self.name)

            mempardecs = [
                m.setFunctionDec()
                for m in self.members if not isinstance(m, InheritanceMember) ]
            mempardecs.append('''

  /** Obtain a pointer to the parameter table. */
  static const dueca::ParameterTable* getParameterTable();''')

            self.createcodedec = ''.join(mempardecs)

            self.typeincludes = ''.join([self.typeincludes, '''
#if !defined(__DCO_STANDALONE)
#include <dueca/ParameterTable.hxx>
#endif'''])
            self.createcodeimp = '''
#if !defined(__DCO_STANDALONE)
// specialisation of the script class data singleton
#ifdef SCRIPT_SCHEME
#include <dueca/SchemeClassData.hxx>
DUECA_NS_START
template<>
SchemeClassData<ScriptCreatableDataHolder<%(name)s> >*
SchemeClassData<ScriptCreatableDataHolder<%(name)s> >::single()
{
  static SchemeClassData<ScriptCreatableDataHolder<%(name)s> > singleton
    ("%(name)s", SchemeClassData<ScriptCreatable>::single());
  return &singleton;
}
DUECA_NS_END
#endif

#ifdef SCRIPT_PYTHON
DUECA_NS_START
#include <dueca/PythonCorrectedName.hxx>
template<>
const char* core_creator_name<ScriptCreatableDataHolder<%(name)s> >(const char*)
{ return "%(name)s"; }
DUECA_NS_END
#endif

// Make a CoreCreator object for this module, the CoreCreator
// will check in with the scheme-interpreting code, and enable the
// creation of objects of this type
static CoreCreator<ScriptCreatableDataHolder<%(objprefix)s%(name)s>,
                   ScriptCreatable>
a(%(objprefix)s%(name)s::getParameterTable(), "%(name)s");
#endif
''' % self.__dict__

            self.memparimp = ''.join(
                [ m.setFunctionImp()
                  for m in self.members if not isinstance(m, InheritanceMember)])

            self.scomments = r'''\n"
       "'''.join(''.join(self.comments).split('\n'))
            self.parentries = ','.join(
                [r.parameterTable() for r in self.members])
            self.parametertable = '''
#if !defined(__DCO_STANDALONE)

#if defined(SCRIPT_PYTHON)
inline const char* TABNAME(const char* n) { return &(n[4]); }
#else
inline const char* TABNAME(const char* n) { return n; }
#endif

// Parameters to be inserted
const dueca::ParameterTable* %(name)s::getParameterTable()
{
  static const dueca::ParameterTable parameter_table[] = {
%(parentries)s,
    { NULL, NULL,
      "%(scomments)s" } };
  return parameter_table;
}%(memparimp)s
#endif''' % self.__dict__



            self.scriptcreatebodyinc = '''
#define DO_INSTANTIATE
#include <dueca/VarProbe.hxx>
#include <dueca/MemberCall.hxx>
#include <dueca/MemberCall2Way.hxx>
#include <dueca/CoreCreator.hxx>
#include <dueca/ScriptCreatableDataHolder.hxx>'''

        else:
            self.createcodedec = ''
            self.createcodeimp = ''
            self.parametertable = ''
            self.scriptcreatebodyinc = ''
        self.inherits = \
            (self.parent and " : public %s" % self.parent) or ''

        # debug stuff
        self.debugcmd = (
            do_debug and "#define DOBS(A) std::cerr << (A) << std::endl") or \
            "#define DOBS(A)"

        # comment string
        self.classcomment = '/**' + '   '.join(self.comments) + '    */'

        # types declared within the class
        self.subtypecmd = ''.join(
            [r.subTypeCommand() for r in iter(self.types.values())])

        # members
        self.memberdec = ''.join(
            [r.declareMemberInClass() for r in self.members
             if r.declareMemberInClass()])

        # calculate magic number
        self.magic = 0
        for c in self.name:
            self.magic = ROR(self.magic) ^ ord(c)
        for r in self.members:
            self.magic = r.calculateMagic(self.magic)

        # pack and string declarations for enum defined types
        self.publicfunctionsdec = ''.join(
            [ r.publicFunctionsDec(self) for r in iter(self.types.values())
              if r.publicFunctionsDec(self) ])
        self.publicfunctionsimp = ''.join(
            [ r.publicFunctionsImp(self) for r in iter(self.types.values())
              if r.publicFunctionsImp(self) ])

        # traits definitions for enum defined types
        self.enumtraits = ''.join(
            [ r.enumTraits(self.name) for r in iter(self.types.values())
              if r.enumTraits(self.name) ])

        # access objects to get the relative addresses and names of members
        self.accessstatics = ''.join(
            [r.staticAccessObject() for r in self.members
             if r.staticAccessObject()])

        # member entries (name and object) table
        self.tableentries = ','.join(
            [r.accessTableEntry() for r in self.members
             if r.accessTableEntry()])
        if self.tableentries:
            self.tableentries = self.tableentries + ','

        # default constructor list and body
        self.defaultconstructorarguments = ','.join(
            [r.defaultConstructorArgumentDec() for r in self.members
             if r.defaultConstructorArgumentDec()])
        self.defaultconstructorargumentsimp = ','.join(
            [r.defaultConstructorArgumentImp() for r in self.members
             if r.defaultConstructorArgumentImp()])
        mi = ','.join(
            [r.defaultConstructorList() for r in self.members
             if r.defaultConstructorList()])
        self.defaultconstructorlist = (mi and ''.join([':', mi])) or ''
        self.defaultconstructorbody = ''.join(
            [r.defaultConstructorBody() for r in self.members
             if r.defaultConstructorBody()])
        # copy constructor list and body
        mi = ','.join(
            [r.copyConstructorList() for r in self.members
             if r.copyConstructorList()])
        self.copyconstructorlist = (mi and ''.join([':', mi])) or ''
        self.copyconstructorbody = ''.join(
            [r.copyConstructorBody() for r in self.members
             if r.copyConstructorBody()])

        # amorph constructor list and body
        mi = ','.join(
            [r.amorphConstructorList() for r in self.members
             if r.amorphConstructorList()])
        self.amorphconstructorlist = (mi and ''.join([':', mi])) or ''
        self.amorphconstructorbody = ''.join(
            [r.amorphConstructorBody() for r in self.members
             if r.amorphConstructorBody()])

        # destructor
        self.destructorbody = ''.join(
            [r.destructorBody() for r in self.members
             if r.destructorBody()])

        # amorph pack and unpack strings. First and second to match the
        # packing in and out of constructor list
        self.amorphpackfirst = ''.join(
            [r.amorphPackFirst() for r in self.members
             if r.amorphPackFirst()])
        self.amorphpacksecond = ''.join(
            [r.amorphPackSecond() for r in self.members
             if r.amorphPackSecond()])
        self.amorphunpackfirst = ''.join(
            [r.amorphUnpackFirst() for r in self.members
             if r.amorphUnpackFirst()])
        self.amorphunpacksecond = ''.join(
            [r.amorphUnpackSecond() for r in self.members
             if r.amorphUnpackSecond()])

        # amorph diff pack and unpack strings.
        self.amorphpackdiff = ''.join(
            [r.amorphPackDiff() for r in self.members
             if r.amorphPackDiff()])
        self.amorphunpackdiff = ''.join(
            [r.amorphUnpackDiff() for r in self.members
             if r.amorphUnpackDiff()])

        # body text of different operators
        commamap = { True : ',', False : ''}
        self.operatorequal = ''.join(
            [r.operatorEqual() for r in self.members])
        self.operatorassign = ''.join(
            [r.operatorAssign() for r in self.members])
        self.operatorprint = ''.join(
            [r.operatorPrint(r == self.members[-1]) for r in self.members])

        # constructor with all arguments
        cargs1 = ','.join(
            [r.completeConstructorArgumentDec() for r in self.members
             if r.completeConstructorArgumentDec()])
        cargs2 = ','.join(
            [r.completeConstructorArgumentImp() for r in self.members
             if r.completeConstructorArgumentImp()])

        if cargs2 != self.defaultconstructorargumentsimp:
            clist = ','.join(
                [r.completeConstructorList() for r in self.members
                 if r.completeConstructorList()])
            cbody = ','.join(
                [r.completeConstructorBody() for r in self.members
                 if r.completeConstructorBody()])
            self.completeconstructordec = """
  /** Constructor with arguments */
  %(name)s(%(cargs1)s);
""" % joindict(self.__dict__, locals())
            self.completeconstructorimp = """
%(name)s::%(name)s(%(cargs2)s) :%(clist)s
{%(fullargsconstructorcode)s
  DOBS("complete constructor %(name)s");%(cbody)s
}""" % joindict(self.__dict__, locals())
        else:
            self.completeconstructordec = ''
            self.completeconstructorimp = ''


        # constructor with all single (non-vector) arguments for
        # compatibility with older code
        cargs1s = ','.join(
            [r.completeConstructorSingleArgumentDec() for r in self.members
             if r.completeConstructorSingleArgumentDec()])
        cargs2s = ','.join(
            [r.completeConstructorSingleArgumentImp() for r in self.members
             if r.completeConstructorSingleArgumentImp()])
        if cargs2s != cargs2 and \
           cargs2s != self.defaultconstructorargumentsimp and len(cargs2s):
            clists = ','.join(
                [r.completeConstructorSingleList() for r in self.members
                 if r.completeConstructorSingleList()])
            self.completeconstructordec_s = """
  /** Constructor with all single (non-iterable) arguments */
  %(name)s(%(cargs1s)s);
""" % joindict(self.__dict__, locals())
            self.completeconstructorimp_s = """
%(name)s::%(name)s(%(cargs2s)s) :%(clists)s
{%(fullargsconstructorcode)s
  DOBS("complete single argument constructor %(name)s");
}""" % joindict(self.__dict__, locals())

        else:
            self.completeconstructordec_s = ''
            self.completeconstructorimp_s = ''

        # constructor with array size initialisations
        cargs1a = ','.join(
            [r.arraySizeInitConstructorArgumentDec() for r in self.members
             if r.arraySizeInitConstructorArgumentDec()])
        cargs2a = ','.join(
            [r.arraySizeInitConstructorArgumentImp() for r in self.members
             if r.arraySizeInitConstructorArgumentImp()])
        clista = ','.join(
            [r.arraySizeInitConstructorList() for r in self.members
             if r.arraySizeInitConstructorList()])
        if len(cargs1a) and len(clista):
            self.arraysizeinitconstructordec = """
  /** Constructor with sizes for all arrays that can be empty-initialised */
  %(name)s(%(cargs1a)s);
""" % joindict(self.__dict__, locals())
            self.arraysizeinitconstructorimp = """
%(name)s::%(name)s(%(cargs2a)s) :%(clista)s
{%(fullargsconstructorcode)s
  DOBS("initializing array size constructor %(name)s");
}""" % joindict(self.__dict__, locals())
        else:
            self.arraysizeinitconstructordec = ""
            self.arraysizeinitconstructorimp = ""

class EventAndStream(Channel):

    def __init__(self, c):
        super(EventAndStream, self).__init__(c)
        self.dcovariant = 'event and stream'
        if pack_alignment is not None:
            self.packpragma = f'\n#pragma pack({pack_alignment})'
            self.endpackpragma = '\n#pragma pack()'
        else:
            self.packpragma = ''
            self.endpackpragma = ''
        pass

    def complete(self):
        super(EventAndStream, self).complete()
        header = c_header % self.__dict__
        f = open(self.name+'.hxx', 'w', encoding='utf-8')
        f.write(header)
        f.close()
        body = c_body % self.__dict__
        f = open(self.name+'.cxx', 'w', encoding='utf-8')
        f.write(body)
        f.close()

class StandaloneEnum(BuildObject):

    def __init__(self, comments):
        super(StandaloneEnum, self).__init__(comments)

        # complete myself as type?
        #self.name = ''
        if objectnamespace:
            self.namespace = '::'.join(objectnamespace)
            self.namespacecmd0 = ''.join(
                [f'namespace {ns} {{\n' for ns in objectnamespace])
            self.namespacecmd1 = '\n' + '} '*len(objectnamespace) + '\n'
            self.objprefix = self.namespace + "::"
            self.inclassprefix = "::dueca::"
        else:
            self.namespace = ''
            self.namespacecmd0 = ''
            self.namespacecmd1 = ''
            self.objprefix = ''
            self.inclassprefix = "dueca::"

        self.options = []
        # creator and date
        try:
            username = pwd.getpwuid(os.getuid())[4].split(',')[0]
            self.maker = "%s (%s)" % (username, getpass.getuser())
        except:
            self.maker = getpass.getuser()
        self.date = datetime.date.today().strftime('%a %d %b %Y')

    def addOption(self, c):
        self.options.append(c)
        pass

    def complete(self):

        # global variables
        global headercommentstring, in_dueca, types, pvals

        enum = types[self.name]
        nameless = copy.copy(self)
        nameless.name = ''
        self.extraheader = ''
        #print(enum.comments)
        #enum.complete(depth='')
        #print(enum.comments)
        self.enumdec = enum.typeCommand('')
        self.publicfunctionsdec = enum.publicFunctionsDec(nameless)
        self.publicfunctionsimp = enum.publicFunctionsImp(nameless)
        self.enumtraits = enum.enumTraits('')

        self.plug_header_include = []
        self.plug_header_classcode = []
        self.plug_header_code = []
        self.plug_body_include = []
        self.plug_body_check = []
        self.plug_body_code = []
        member = MemberBase(self, '', 'noname', enum)
        memberlist = [ summarise_member(member, '', self.namespace) ]

        for p in (set(self.options) & set(enum_plugins.keys())):
            try:
                pcode = enum_plugins[p].AddOn(
                    self.namespace, self.name, None, memberlist)
            except AttributeError as e:
                print(f"Cannot find class AddOn in enum module {p}",
                      f"have {dir(enum_plugins[p])}")
                raise e
            self.plug_header_include.append(pcode.printHeaderInclude())
            self.plug_header_classcode.append(pcode.printHeaderClassCode())
            self.plug_header_code.append(pcode.printHeaderCode())
            self.plug_body_include.append(pcode.printBodyInclude())
            self.plug_body_check.append(pcode.printBodyCheck())
            self.plug_body_code.append(pcode.printBodyCode())
            self.customdefines.extend(pcode.getCustomDefines())

        # convert to single strings
        self.plug_header_include = ''.join(self.plug_header_include)
        self.plug_header_classcode = ''.join(self.plug_header_classcode)
        self.plug_header_code = ''.join(self.plug_header_code)
        self.plug_body_include = ''.join(self.plug_body_include)
        self.plug_body_check = ''.join(self.plug_body_check)
        self.plug_body_code = ''.join(self.plug_body_code)

        # common parts
        super(StandaloneEnum, self).complete()

        # type include?

        self.typeincludes = enum.includeCommand() or ''
        header = e_header.format(**self.__dict__)
        body = e_body.format(**self.__dict__)
        with open(self.name+'.hxx', 'w', encoding='utf-8') as f:
            f.write(header)
        with open(self.name+'.cxx', 'w', encoding='utf-8') as f:
            f.write(body)

# load any supporting auto modules
setup_vars()

# parser stuff. First basic, top-level keywords
typekw = Literal('(') + Literal('Type').addParseAction(startType)
itypekw = Literal('(') + Literal('IterableType').addParseAction(startIType)
itypekwvar = Literal('(') + Literal('IterableVarSizeType').addParseAction(startIVarType)
arraysizekw = Literal('(') + Literal('ArraySize').addParseAction(startArraySize)
arraysizeenumkw = Literal('(') + Literal('ArraySizeEnum').addParseAction(startArraySizeEnum)
enumkw =  Literal('(') + Word('Enum').addParseAction(startEnum)
classenumkw = Literal('(') + Word('EnumClass').addParseAction(startClassEnum)

# these are all the same now, but for compatibility retained
eventkw = Literal('(') + Literal('Event').addParseAction(startEventAndStream)
streamkw = Literal('(') + Literal('Stream').addParseAction(startEventAndStream)
eventstreamkw = Literal('(') + \
    Literal('EventAndStream').addParseAction(startEventAndStream)
objectkw = Literal('(') + Literal('Object').addParseAction(startEventAndStream)

# Only enumerator code
enumeratorkw = Literal('(') + \
    Literal('Enumerator').addParseAction(startEnumerator)

comment = Regex(r';+') + \
    Regex(r'(.*)\n').setWhitespaceChars('').addParseAction(addComment).\
    setName('comment')
ccomment = Regex(r';+') + Regex(r'[^\n]*').addParseAction(addContentComment).\
    setName('content comment')
number = Regex(r'[-+]?[0-9]*\.?[0-9]+[fF]?([eE][-+]?[0-9]+)?').setName('number')
inumber = Regex(r'0x[0-9a-fA-F]+|[-+]?[0-9]+').setName('inumber')
identifier = Regex('[a-zA-Z_][a-zA-Z0-9_<>,:]*').setName('identifier')
stringvar = QuotedString(quoteChar='"', escChar='\\', unquoteResults=False,
                         multiline=True).setName('stringvar')
namespace = (Literal('(') + Literal('Namespace') +
             identifier + Literal(')')).addParseAction(appendNameSpace)
alignment = Literal('(') + Literal('PackAlignment') + \
    ( Literal('1') | Literal('2') | Literal('4') |
      Literal('8') | Literal('0') ).addParseAction(setPackAlignment) + \
      Literal(')')


# the unquoteResults flag also removes backslashes elswhere in the string
# (i.e., not

def clnString(t):
    # strip the starting and ending "
    # replace the old-style \n at the end of line (compat old dueca)
    # replace \n in the string by a return
    # replace escaped quotes

    # split on any double \\ (which escape the \)
    r = t[0][1:-1].split(r'\\')

    # then process the parts;
    r = [ ir.replace('\\n\n', '\n').
          replace(r'\n', '\n').replace('\\"', '"') for ir in r ]
    return '\\'.join(r)

stringvar.setParseAction(clnString)


nonTag = Combine(OneOrMore(CharsNotIn(r'\()') | r'\(' | r'\)' ))
defvar = OneOrMore(nonTag | nestedExpr('(', ')', nonTag))

headercomments = Literal('(') + Literal('Header') + \
    (stringvar.copy()).addParseAction(headerComments) + Literal(')')

inherits = Literal('(') + Literal('Inherits') + \
         identifier.copy().addParseAction(addInherits) + \
         Literal(')')

optionvals = [ Literal(p) for p in plugins.keys() ]
optionvals.extend(
    [Literal('ScriptCreatable') | Literal('OmitDefaultConstructor')])
optionvals = Or(optionvals)

option = Literal('(') + Literal('Option') + \
    OneOrMore(optionvals).addParseAction(addOption) + \
    Literal(')')

include = Literal('(') + Literal('IncludeFile') + \
    (identifier.copy()).addParseAction(addInclude) + Literal(')')

includehdr = Literal('(') + Literal('AddToHeader') + \
    (stringvar.copy()).addParseAction(addToHeader) + Literal(')')

constructor = Literal('(') + Literal('ConstructorCode') + \
    (stringvar.copy()).addParseAction(addConstructorCode) + \
    Literal(')')

constructor2 = Literal('(') + Literal('FullArgsConstructorCode') + \
    (stringvar.copy()).addParseAction(addFullArgsConstructorCode) + \
    Literal(')')

dimension = Regex('[^() ]+').addParseAction(addDimension)

defval  = Literal('(') + Literal('Default ') + \
    (defvar.copy()).addParseAction(setDefault) + Literal(')')

defarg  = Literal('(') + Literal('DefaultArg ') + \
    (defvar.copy()).addParseAction(setDefaultArg) + Literal(')')

defsize = Literal('(') + Literal('DefaultSize ') + \
    (defvar.copy()).addParseAction(setDefaultSize) + Literal(')')

contents = ZeroOrMore(ccomment) + Literal('(') + \
    identifier.copy().addParseAction(addClass) + \
    identifier.copy().addParseAction(addMember) + \
    Optional(dimension) + Optional(defval) + Optional(defarg) + \
    Optional(defsize) + \
    Literal(')').addParseAction(completeMember)

ctype = Or((typekw, itypekw, itypekwvar) ) + \
    identifier.copy().addParseAction(setTypeName) + \
    Optional(stringvar.copy().addParseAction(addInclude)) + \
    Literal(')').addParseAction(complete)

eventandstream = (Or((eventstreamkw, streamkw, eventkw, objectkw)) +
      identifier.copy().addParseAction(setName) +
      Optional(inherits) +
      ZeroOrMore(option) +
      Optional(include) +
      Optional(constructor) +
      Optional(constructor2) +
      Optional(includehdr) +
      ZeroOrMore(contents) +
      Literal(')').addParseAction(complete))

enumcode = (enumeratorkw +
            identifier.copy().addParseAction(setName) +
            ZeroOrMore(option) +
            Optional(include) +
            Literal(')')).addParseAction(complete)

enummember = ZeroOrMore(ccomment) + \
    identifier.copy().addParseAction(addEnumMember) + \
    Optional(Literal('=') + inumber.copy().addParseAction(addEnumValue))

enum = enumkw + White() + \
    identifier.copy().addParseAction(setName) + \
    identifier.copy().addParseAction(setEnumType) + \
    Optional(stringvar.copy().addParseAction(addInclude)) + \
    ZeroOrMore(enummember) + \
    Literal(')').addParseAction(complete)

clenum = classenumkw + White() + \
    identifier.copy().addParseAction(setName) + \
    identifier.copy().addParseAction(setEnumType) + \
    Optional(stringvar.copy().addParseAction(addInclude)) + \
    ZeroOrMore(enummember) + \
    Literal(')').addParseAction(complete)

arraysize = arraysizekw + White() + \
    identifier.copy().addParseAction(setName) + \
    Optional(stringvar.copy().addParseAction(addInclude)) + \
    Literal(')').addParseAction(complete)

arraysizeenum = arraysizeenumkw + White() + \
    identifier.copy().addParseAction(setName) + \
    Optional(stringvar.copy().addParseAction(addInclude)) + \
    Literal(')').addParseAction(complete)



# event | stream | enum |
content = ZeroOrMore(comment | ctype | namespace | alignment |
                     eventandstream | enum |
                     headercomments | enum | clenum | enumcode |
                     arraysize | arraysizeenum )

"""
#if 0
  /** placement "new", needed for other versions of the stl.*/
  inline static void* operator new(size_t size, void*& o)
  { return o; }
#endif"""


# code content
c_header = \
"""/* ------------------------------------------------------------------ */
/*      item            : %(name)s.hxx
        generated by    : %(maker)s
        date            : %(date)s
        category        : header file
        description     : DUECA Communication Object (DCO)
                          automatically generated by dueca-codegen
        codegen version : %(codegenversion)i
        language        : C++%(headercomments)s
*/

#ifndef %(name)s_hxx
#define %(name)s_hxx

#include <inttypes.h>

#if !defined(__DCO_NOPACK)
namespace dueca {
class AmorphStore;
class AmorphReStore;
struct DataWriterArraySize;
};
#endif
#if !defined(__DCO_STANDALONE)
namespace dueca {
struct CommObjectDataTable;
};
#include <gencodegen.h>
#if GENCODEGEN != %(codegenversion)i
#error "Generated %(name)s.hxx too old, please clean with 'make mrproper'"
#endif
#include <dueca/CommObjectTraits.hxx>
#endif
#include <iostream>
%(extraheader)s
%(typeincludes)s%(plug_header_include)s
%(namespacecmd0)s%(packpragma)s
%(classcomment)s
struct %(name)s%(inherits)s
{
  /** typedef for internal reference */
  typedef %(name)s __ThisDCOType__;

public:
  /** The name of this class. */
  static const char* const classname;

%(subtypecmd)s%(memberdec)s
public:
  /** a "magic" number, hashed out of the class definition,
      that will be used to check consistency of the sent objects
      across the dueca nodes. */
  static const uint32_t magic_check_number;

  /** default constructor. */
  %(name)s(%(defaultconstructorarguments)s);
%(completeconstructordec)s%(completeconstructordec_s)s
  /** copy constructor. */
  %(name)s(const %(name)s& o);

#if !defined(__DCO_NOPACK)
  /** constructor to restore an %(name)s from amorphous storage. */
  %(name)s(%(inclassprefix)sAmorphReStore& r);
#endif%(arraysizeinitconstructordec)s

  /** destructor. */
  ~%(name)s();

#if !defined(__DCO_STANDALONE)
  /** new operator "new", which places objects not on a
      heap, but in one of the memory arenas. This to speed up
      memory management. */
  static void* operator new(size_t size);

  /** new operator "delete", to go with the new version
      of operator new. */
  static void operator delete(void* p);

  /** placement "new", needed for stl. */
  inline static void* operator new(size_t size, %(name)s*& o)
  { return reinterpret_cast<void*>(o); }
#endif

#if !defined(__DCO_NOPACK)
  /** packs the %(name)s into amorphous storage. */
  void packData(::dueca::AmorphStore& s) const;

  /** packs the %(name)s into amorphous storage.
      only differences with a previous object are packed. */
  void packDataDiff(::dueca::AmorphStore& s, const %(name)s& ref) const;

  /** unpacks the %(name)s from an amorphous storage. */
  void unPackData(::dueca::AmorphReStore& s);

  /** unpacks the differences for %(name)s
      from an amorphous storage. */
  void unPackDataDiff(::dueca::AmorphReStore& s);
#endif

  /** Test for equality. */
  bool operator == (const %(name)s& o) const;

  /** Test for inequality. */
  inline bool operator != (const %(name)s& o) const
  { return !(*this == o); }

  /** Assignment operator. */
  %(name)s& operator=(const %(name)s& o);

  /** prints the %(name)s to a stream. */
  std::ostream & print(std::ostream& s) const;%(plug_header_classcode)s%(createcodedec)s%(extrainclude)s
};
%(namespacecmd1)s%(endpackpragma)s
#if !defined(__DCO_NOPACK)
/** pack the object into amorphous storage. */
inline void packData(::dueca::AmorphStore& s,
                     const %(objprefix)s%(name)s& o)
{ o.packData(s); }

/** pack the differences between this object and another
    into amorphous storage. */
inline void packDataDiff(dueca::AmorphStore& s,
                         const %(objprefix)s%(name)s& o,
                         const %(objprefix)s%(name)s& ref)
{ o.packDataDiff(s, ref); }

/** unpack the object from amorphous storage. */
inline void unPackData(::dueca::AmorphReStore& s,
                       %(objprefix)s%(name)s& o)
{ o.unPackData(s); }

/** unpack the differences to this object from storage. */
inline void unPackDataDiff(dueca::AmorphReStore& s,
                           %(objprefix)s%(name)s& o)
{ o.unPackDataDiff(s); }
#endif
%(publicfunctionsdec)s
namespace std {
/** print to a stream. */
inline std::ostream &
operator << (std::ostream& s, const %(objprefix)s%(name)s& o)
{ return o.print(s); }
};

#if !defined(__DCO_STANDALONE)
namespace dueca {
/** Template specialization, defines a trait that is needed if
    %(name)s is ever used inside other dco objects. */
template <>
struct dco_nested<%(objprefix)s%(name)s> : public dco_isnested { };%(enumtraits)s
};
#endif
%(plug_header_code)s

#endif
"""

c_body = \
"""/* ------------------------------------------------------------------ */
/*      item            : %(name)s.cxx
        generated by    : %(maker)s
        date            : %(date)s
        category        : body file
        description     : DUECA Communication Object (DCO),
                          automatically generated by dueca-codegen
        codegen version : %(codegenversion)i
        language        : C++
*/

#include "%(name)s.hxx"
#include <iostream>%(arraydimsdec)s
%(assertinclude)s
#if !defined(__DCO_NOPACK)
#include <dueca/AmorphStore.hxx>
#include <dueca/PackUnpackTemplates.hxx>
#endif
#include <dueca/DataWriterArraySize.hxx>
%(debugcmd)s
#if !defined(__DCO_STANDALONE)
#include <dueca/Arena.hxx>
#include <dueca/ArenaPool.hxx>
#include <dueca/DataClassRegistrar.hxx>
#include <dueca/CommObjectMemberAccess.hxx>
#include <dueca/DCOFunctor.hxx>
#include <dueca/DCOMetaFunctor.hxx>
%(scriptcreatebodyinc)s
#define DO_INSTANTIATE
#include <dueca/DataSetSubsidiary.hxx>
#endif%(plug_body_include)s


%(namespacecmd0)s%(arraydimsimp)s%(extraincludebody)s

#if !defined(__DCO_STANDALONE)
// static CommObjectMemberAccess objects, that can provide flexible access
// to the members of a %(name)s object%(accessstatics)s

// assemble the above entries into a table in the order in which they
// appear in the %(name)s object
static const ::dueca::CommObjectDataTable entriestable[] = {%(tableentries)s
  { NULL }
};

#endif

// class name, static
const char * const %(name)s::classname = "%(name)s";

// magic number, hashed from class name and member names / classes
const uint32_t %(name)s::magic_check_number=0x%(magic)x;

#if !defined(__DCO_STANDALONE)
// functor table, provides access to user-defined metafunctions through the
// data class registry
static dueca::functortable_type functortable;

// register this class, provides access to a packing/unpacking object,
// and to the member access tables
static ::dueca::DataClassRegistrar registrar
  (%(name)s::classname, %(registrarparent)s,
   entriestable, &functortable,
   new ::dueca::DataSetSubsidiary<%(name)s>());
%(parametertable)s
#endif

#ifndef __CUSTOM_DEFAULT_CONSTRUCTOR
%(name)s::%(name)s(%(defaultconstructorargumentsimp)s)%(defaultconstructorlist)s
{%(defaultconstructorbody)s%(constructorcode)s
  DOBS("default constructor %(name)s");
}
#endif

#ifndef __CUSTOM_FULL_CONSTRUCTOR%(completeconstructorimp)s
#endif

#ifndef __CUSTOM_FULLSINGLES_CONSTRUCTOR%(completeconstructorimp_s)s
#endif

#ifndef __CUSTOM_COPY_CONSTRUCTOR
%(name)s::%(name)s(const %(name)s& other)%(copyconstructorlist)s
{%(copyconstructorbody)s
  DOBS("copy constructor %(name)s");
}
#endif

#if !defined(__CUSTOM_AMORPHRESTORE_CONSTRUCTOR) && !defined(__DCO_NOPACK)
%(name)s::%(name)s(%(inclassprefix)sAmorphReStore& s)%(amorphconstructorlist)s
{%(amorphconstructorbody)s
  DOBS("amorph constructor %(name)s");
}
#endif

#if !defined(__CUSTOM_ARRAY_SIZE_INIT_CONSTRUCTOR)%(arraysizeinitconstructorimp)s
#endif

#ifndef __CUSTOM_DESTRUCTOR
%(name)s::~%(name)s()
{%(destructorbody)s
  DOBS("destructor %(name)s");
}
#endif

#if !defined(__DCO_STANDALONE)
void* %(name)s::operator new(size_t size)
{
  DOBS("operator new %(name)s");
  static ::dueca::Arena* my_arena = arena_pool.findArena
    (sizeof(%(name)s));
  return my_arena->alloc(size);
}

void %(name)s::operator delete(void* v)
{
  DOBS("operator delete %(name)s");
  static ::dueca::Arena* my_arena = arena_pool.findArena
    (sizeof(%(name)s));
  my_arena->free(v);
}
#endif

#if !defined(__CUSTOM_FUNCTION_PACKDATADIFF) && !defined(__DCO_NOPACK)
void %(name)s::packDataDiff(::dueca::AmorphStore& s, const %(name)s& ref) const
{
  DOBS("packDataDiff %(name)s");
  ::dueca::IndexMemory im;%(amorphpackdiff)s
  im.closeoff(s);
}
#endif

#if !defined(__CUSTOM_FUNCTION_UNPACKDATA) && !defined(__DCO_NOPACK)
void %(name)s::unPackData(::dueca::AmorphReStore& s)
{
  DOBS("unPackData %(name)s");
%(amorphunpackfirst)s
%(amorphunpacksecond)s
}
#endif

#if !defined(__CUSTOM_FUNCTION_UNPACKDATADIFF) && !defined(__DCO_NOPACK)
void %(name)s::unPackDataDiff(%(inclassprefix)sAmorphReStore& s)
{
  DOBS("unPackDataDiff %(name)s");
  ::dueca::IndexRecall im;%(amorphunpackdiff)s
}
#endif

#ifndef __CUSTOM_OPERATOR_EQUAL
bool %(name)s::operator == (const %(name)s& other) const
{
  DOBS("operator == %(name)s");%(operatorequal)s
  return true;
}
#endif

#ifndef __CUSTOM_OPERATOR_ASSIGN
%(name)s&
%(name)s::operator=(const %(name)s& other)
{
  DOBS("operator = %(name)s");
  if (this == &other) return *this;%(operatorassign)s
  return *this;
}
#endif

#if !defined(__CUSTOM_FUNCTION_PACKDATA) && !defined(__DCO_NOPACK)
void %(name)s::packData(::dueca::AmorphStore& s) const
{
  DOBS("packData %(name)s");%(amorphpackfirst)s%(amorphpacksecond)s
}
#endif

#ifndef __CUSTOM_FUNCTION_PRINT
std::ostream & %(name)s::print(std::ostream& s) const
{
  s << "%(name)s("%(operatorprint)s
    << ')';
  return s;
}
#endif

%(namespacecmd1)s%(plug_body_code)s%(createcodeimp)s%(publicfunctionsimp)s
"""

e_header = \
"""/* ------------------------------------------------------------------ */
/*      item            : {name}.hxx
        generated by    : {maker}
        date            : {date}
        category        : header file
        description     : DUECA Communication Object (DCO)
                          automatically generated by dueca-codegen
        codegen version : {codegenversion}
        language        : C++{headercomments}
*/

#ifndef {name}_hxx
#define {name}_hxx

#include <inttypes.h>

#if !defined(__DCO_NOPACK)
namespace dueca {{
class AmorphStore;
class AmorphReStore;
}};
#endif

#if !defined(__DCO_STANDALONE)
#include <gencodegen.h>
#if GENCODEGEN != {codegenversion}
#error "Generated {name}.hxx too old, please clean with 'make mrproper'"
#endif
#include <dueca/CommObjectTraits.hxx>
#endif

#include <iostream>
{extraheader}
{typeincludes}{plug_header_include}

{namespacecmd0}
{enumdec}
{namespacecmd1}{extrainclude}

{publicfunctionsdec}

#if !defined(__DCO_STANDALONE)
namespace dueca {{
{enumtraits}
}};
#endif
{plug_header_code}

#endif
"""

e_body = \
"""/* ------------------------------------------------------------------ */
/*      item            : {name}.cxx
        generated by    : {maker}
        date            : {date}
        category        : header file
        description     : DUECA Communication Object (DCO)
                          automatically generated by dueca-codegen
        codegen version : {codegenversion}
        language        : C++
*/

#include "{name}.hxx"
{assertinclude}
#if !defined(__DCO_NOPACK)
#include <dueca/AmorphStore.hxx>
#include <dueca/PackUnpackTemplates.hxx>
#endif
{debugcmd}
#if !defined(__DCO_STANDALONE)
#include <dueca/Arena.hxx>
#include <dueca/ArenaPool.hxx>
#include <dueca/DataClassRegistrar.hxx>
#include <dueca/CommObjectMemberAccess.hxx>
#include <dueca/DCOFunctor.hxx>
#include <dueca/DCOMetaFunctor.hxx>
#define DO_INSTANTIATE
#include <dueca/DataSetSubsidiary.hxx>
#endif{plug_body_include}


{namespacecmd0}{extraincludebody}
{namespacecmd1}{plug_body_code}{publicfunctionsimp}
"""

if __name__ == "__main__":

    #do_debug = True
    from platform import python_version

    if pvals.dcofile:
        for fname in pvals.dcofile:
            with open(fname, 'r', encoding='utf-8') as infile:
                dcodata = ''.join(infile.readlines())
                currentobject = os.path.basename(fname)[:-4]
                try:
                    content.parseString(dcodata, True)
                except CodegenException as e:
                    print('\n'.join([e.__doc__, '', 'Code generation failed!']))
                    sys.exit(1)
                except:
                    raise

    else:
        if list(map(int, python_version().split('.')[:2])) < [3, 7]:
            import io
            dcodata = ''.join(io.TextIOWrapper(
                sys.stdin.buffer, encoding='utf-8').readlines())
        else:
            sys.stdin.reconfigure(encoding='utf-8')
            dcodata = ''.join(sys.stdin.readlines())

        try:
            content.parseString(dcodata, True)
        except CodegenException as e:
            print('\n'.join([e.__doc__, '', 'Code generation failed!']))
            sys.exit(1)
        except:
            raise
