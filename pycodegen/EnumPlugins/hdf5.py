# AddOn for hdf5 option
"""item            : hdf5.py
        made by         : RvP
        date            : 2017
        category        : python program
        description     : Code generation of DUECA Communication Objects (DCO)
                          hdf5 extension
        language        : python
        changes         : 1704xx RvP Added a plugin system, to enable
                                     extension of code generation
                          171010 RvP Added option for compress
        copyright       : TUDelft-AE-C&S

AddOn objects extend the code generation by the dueca-codegen program

Create a file named after the Option you want to add to the code
generation, and install it in the DCOplugins directory. This file adds
(Option hdf5enum) to DCO files, adding ability to store restore
enum objects in an HDF5 format file.

The same pattern can be followed to add other capabilities.

"""

def joindict(x, y):
    z = x.copy()
    z.update(y)
    return z

class AddOn(object):
    """ Print HDF5 interaction code for a DCO enum.

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
        self.dueca_hdf_version = 1
        #print(self.__dict__)
        
    def printHeaderInclude(self):
        """ print the lines that will be added to the header's include area
        """
        return """
#ifdef DUECA_CONFIG_HDF5
#include <H5Cpp.h>
#include <hdf5utils/HDF5DCOMetaFunctor.hxx>
#endif"""


    def printBodyInclude(self):
        """ print the lines that will be added to the body's include area
        """
        return """
#ifdef DUECA_CONFIG_HDF5
#include <hdf5utils/HDF5Templates.hxx>
#endif"""

    def printBodyCheck(self):
        """print the lines *after* a possible include of custom body code to
        check that any custom-built hdf5 code is compatible with the
        version it was designed for. If the interfaces used by the
        code generation of hdf5 code are changed, define a new version
        number, so this check is updated.

        """

        return r"""
#define DUECA_HDF_CODEGEN_VERSION %(dueca_hdf_version)s
#if defined(%(customdefines)s)
#ifndef __CUSTOM_COMPATLEVEL_HDF_%(dueca_hdf_version)s
#error "Verify custom hdf code compatibility with version %(dueca_hdf_version)s.\
 Then define __CUSTOM_COMPATLEVEL_HDF_%(dueca_hdf_version)s"
#endif
#endif
""" % dict(dueca_hdf_version=self.dueca_hdf_version,
           customdefines=r""") || \
    defined(""".join(self.getCustomDefines()))

    def printHeaderClassCode(self):
        """code that will be inserted in the definition of the class. Assumes
        public, and inserted at the end"""

        return ''

    def printHeaderCode(self):
        """code that will be inserted in the header after the class
        definition. Starts outside any namespace directive.
        """
        res = []

        res.append("""
#if defined(DUECA_CONFIG_HDF5)
namespace dueca {
  template<typename T>
  const H5::DataType* get_hdf5_type(const T& t);
  template<>
  const H5::DataType* get_hdf5_type(const %(klassname)s &o);
  template<typename T>
  const H5::DataType* get_hdf5_elt_type(const T& t);
  template<>
  const H5::DataType* get_hdf5_elt_type(const %(klassname)s &o);
  template<typename T>
  unsigned get_hdf5_elt_size(const T& t);
  template<>
  unsigned get_hdf5_elt_size(const %(klassname)s &o);

  template<typename T>
  const H5::DataType* get_hdf5_type(T& t);
  template<>
  const H5::DataType* get_hdf5_type(%(klassname)s &o);
  template<typename T>
  const H5::DataType* get_hdf5_elt_type(T& t);
  template<>
  const H5::DataType* get_hdf5_elt_type(%(klassname)s &o);
  template<typename T>
  unsigned get_hdf5_elt_size(T& t);
  template<>
  unsigned get_hdf5_elt_size(%(klassname)s &o);
} // end namespace dueca
#endif""" % { 'klassname' : self.members[0].getType() })

        return ''.join(res)

    def printBodyCode(self):
        """code that is inserted in the body file. After all regular
        code, assumes global namespace"""

        hdf5body = []


        # part 4, any enum members that need a definition
        #print m.klassref.__class__
        hdf5body.append("""
#if defined(DUECA_CONFIG_HDF5) && !defined(__CUSTOM_HDF5_ENUM_%(name)s)
namespace dueca {
  template<>
  const H5::DataType*
    get_hdf5_type(const %(klassname)s &o)
  {
    static H5::EnumType data_type(sizeof(%(ctype)s));
    static bool once = true;
    if (once) {
      %(klassname)s v;
""" % { 'name' : self.members[0].getType(bare=True),
        'klassname': self.members[0].getType(),
        'ctype' : self.members[0].getCType() })
        for name, barename in zip(self.members[0].getMembers(), 
                                 self.members[0].getMembers(bare=True)):
                hdf5body.append("""
      v = %(name)s;
      data_type.insert("%(barename)s", &v);""" % { 'barename' : barename,
                                                   'name' : name })
        hdf5body.append("""
      once = false;
    }
    return &data_type;
  }

  template<>
  const H5::DataType* get_hdf5_elt_type(const %(klassname)s &o)
  { return get_hdf5_type(o); }

  template<>
  unsigned get_hdf5_elt_size(const %(klassname)s &o)
  { return 1; }

  template<>
  const H5::DataType*
    get_hdf5_type(%(klassname)s &o)
  {
    static H5::EnumType data_type(sizeof(%(ctype)s));
    static bool once = true;
    if (once) {
      %(klassname)s v;
""" % { 'name' : self.members[0].getType(bare=True),
        'klassname': self.members[0].getType(),
        'ctype' : self.members[0].getCType() })
        for name, barename in zip(self.members[0].getMembers(), 
                                 self.members[0].getMembers(bare=True)):
                hdf5body.append("""
      v = %(name)s;
      data_type.insert("%(barename)s", &v);""" % { 'barename' : barename,
                                                   'name' : name })
        hdf5body.append("""
      once = false;
    }
    return &data_type;
  }

  template<>
  const H5::DataType* get_hdf5_elt_type(%(klassname)s &o)
  { return get_hdf5_type(o); }

  template<>
  unsigned get_hdf5_elt_size(%(klassname)s &o)
  { return 1; }

} // end dueca namespace
#endif""" % { 'klassname' : self.members[0].getType() })
        return ''.join(hdf5body)

    def getCustomDefines(self):
        """these defines guard the implementation in the body, and can be used
        to override standard implementation. When the standard
        implementation is overridden, define
        __CUSTOM_COMPATLEVEL_HDF_# to indicate compatibility with a
        specific version of the hdf code. These are also checked for
        the general code generation compatibility level.

        """

        res = [ '__CUSTOM_HDF5_ENUM_' + self.members[0].getType(bare=True) ]
        return res
