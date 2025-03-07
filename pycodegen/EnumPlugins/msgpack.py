# AddOn for msgpack option
"""     item            : msgpack.py
        made by         : RvP
        date            : 2017
        category        : python program
        description     : Code generation of DUECA Communication Objects (DCO)
                          msgpack extension
        language        : python
        changes         : 1704xx RvP Added a plugin system, to enable
                                     extension of code generation
                          171010 RvP Added option for compress
        copyright       : TUDelft-AE-C&S

AddOn objects extend the code generation by the dueca-codegen program

Create a file named after the Option you want to add to the code generation,
and install it in the DCOplugins directory. This file adds (Option msgpack) to
DCO files, adding ability to convert DCO objects to and from msgpack.

The same pattern can be followed to add other capabilities.

- base, define, map vs array for objects; Inherited is mapped with
  class name as key!
  https://github.com/msgpack/msgpack-c/wiki/v1_1_cpp_adaptor

- visitor
  https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_visitor

- manual packing
  https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_packer#pack-manually
"""


def joindict(x, y):
    z = x.copy()
    z.update(y)
    return z

class AddOn(object):
    """ Print MSGPACK interaction code for a DCO object.

    - include section of header (printHeaderInclude)
    - in class definition, at end, but just before extra include
      (printHeaderClassCode)
    - in header, outside class (printHeaderCode)
    - include in body file (printBodyInclude)
    - in the body file, in a check section executed when custom body
      code is included (printBodyCheck)
    - in the body (printBodyCode)

    """
    def __init__(self, namespace, name, parent, members):
        """ Initialisation of an AddOn object

        namespace -  name space of the DCO object
        name      -  class name of DCO object
        parent    -  parent class name
        members   -  list with MemberSummary objects describing data members,
                     options:
                         getName()
                         getType(bare=, in_class=, in_namespace=)
                         isEnum()
                         isIterable()
                         and for enums
                         getCType()
                         getMembers(bare=, in_class=, in_namespace=)
        """

        self.namespace = namespace
        self.nsprefix = (namespace and namespace+'::') or ''
        self.nsopen = (
            namespace and "\nnamespace %s {" % namespace) or ''
        self.nsclose = (
            namespace and "\n} // end namespace %s" % namespace) or ''
        self.name = name
        self.parent = parent
        self.members = members
        self.dueca_msgpack_version = 1

    def printHeaderInclude(self):
        """ print the lines that will be added to the header's include area
        """
        return """
#include <dueca/msgpack.hxx>
#include <dueca/msgpack-unstream-iter.hxx>
#ifndef NESTED_DCO
#include <dueca/msgpack-unstream-iter.ixx>
#endif"""

    def printBodyInclude(self):
        """ print the lines that will be added to the body's include area
        """
        return """"""

    def printBodyCheck(self):
        """print the lines *after* a possible include of custom body code to
        check that any custom-built hdf5 code is compatible with the
        version it was designed for. If the interfaces used by the
        code generation of hdf5 code are changed, define a new version
        number, so this check is updated.

        """

        return ""

    def printHeaderClassCode(self):
        """code that will be inserted in the definition of the class. Assumes
        public, and inserted at the end"""

        return ""

    def printHeaderCode(self):
        """code that will be inserted in the header after the class
        definition. Starts outside any namespace directive.
        """

        res = []

        # add any enum's defined in the class
        #eclasses = set()
        #for m in self.members:
        #    if not m.isEnum() or len(m.getMembers()) == 0 or \
        #        m.getType() in eclasses:
        #        continue

        res.append( f"""
#if defined(DUECA_CONFIG_DDFF) || defined(DUECA_CONFIG_MSGPACK)
#ifndef __CUSTOM_MSGPACK_ENUM_{self.members[0].getType(bare=True)}
MSGPACK_ADD_ENUM({self.members[0].getType()});
MSGPACK_ADD_ENUM_VISITOR({self.members[0].getType()});
MSGPACK_ADD_ENUM_UNSTREAM({self.members[0].getType()});
#endif
#endif
""")

        return ''.join(res)

    def printBodyCode(self):
        """code that is inserted in the body file. After all regular
        code, assumes global namespace"""

        return ''

    def getCustomDefines(self):
        """these defines guard the implementation in the body, and can be used
        to override standard implementation. When the standard
        implementation is overridden, define
        __CUSTOM_COMPATLEVEL_MSGPACK_# to indicate compatibility with a
        specific version of the hdf code. These are also checked for
        the general code generation compatibility level.

        """

        emembers = set()
        res = []
        for m in self.members:
            if m.isEnum() and len(m.getMembers()) > 0 and \
                m.getType() not in emembers:
                res.append('__CUSTOM_MSGPACK_ENUM_' + m.getType(bare=True))
        res.append('__CUSTOM_MSGPACK_PACK')
        res.append('__CUSTOM_MSGPACK_VISITOR')
        return res
