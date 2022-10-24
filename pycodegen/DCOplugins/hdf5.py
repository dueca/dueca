# AddOn for hdf5 option
"""     item            : hdf5.py
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

Create a file named after the Option you want to add to the code generation,
and install it in the DCOplugins directory. This file adds (Option hdf5) to
DCO files, adding ability to read DCO objects from a DUECA channel and
to store this in an HDF5 format file.

The same pattern can be followed to add other capabilities.
"""


def joindict(x, y):
    z = x.copy()
    z.update(y)
    return z

class AddOn(object):
    """ Print HDF5 interaction code for a DCO object.

    - include section of header (printHeaderInclude)
    - in class definition, at end, but just before extra include
      (printHeaderClassCode)
    - in header, outside class (printHeaderCode)
    - include in body file (printBodyInclude)
    - in the body file, in a check section executed when custom body
      code is included (printBodyCheck)
    - in the body (printBodyCode)

    """
    def __init__(self, namespace, name, parent, members, nest=False):
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
        self.nest = nest
        self.dueca_hdf_version = 1

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

        if self.nest:
            return """

#ifdef DUECA_CONFIG_HDF5
  /** create a HDF5 datatype */
  static const H5::DataType* getHDF5DataType();
#endif"""
        else:
            return ''

    def printHeaderCode(self):
        """code that will be inserted in the header after the class
        definition. Starts outside any namespace directive.
        """
        res = []
        eclasses = set()
        for m in self.members:

            # only for enum, once per type, and only for enums that
            # have not been defined elsewhere
            if not m.isEnum() or m.getType() in eclasses or \
                len(m.getMembers()) == 0:
                continue

            eclasses.add(m.getType())

            # declare the template specializations
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
#endif""" % { 'klassname' : m.getType() })

        # declaration for the functor class
        res.append("""
#if defined(DUECA_CONFIG_HDF5)
// functors guarded in a separate namespace
namespace %(name)s_space {

  // writes to file, reading from channel
  class HDF5DCOWriteFunctor: public dueca::hdf5log::HDF5DCOWriteFunctor
  {
    %(name)s example%(parentdecwrite)s;
  public:
    // constructor, to be invoked by the metafunctor
    HDF5DCOWriteFunctor(boost::weak_ptr<H5::H5File> file,
                        const std::string& path,
                        size_t chunksize,
                        const std::string& label,
                        bool compress, bool writeticks,
                        const dueca::DataTimeSpec* startend);

    // the functor member used by channel reading code
    bool operator() (const void* dpointer, const dueca::DataTimeSpec& ts);
  };

  // reads from file, writing to channel
  class HDF5DCOReadFunctor: public dueca::hdf5log::HDF5DCOReadFunctor
  {
    %(name)s example%(parentdecread)s;
  public:
    // constructor, to be invoked by the metafunctor
    HDF5DCOReadFunctor(boost::weak_ptr<H5::H5File> file,
                       const std::string& path,
                       bool readticks);
    // the functor member used by channel writing code
    bool operator() (void* dpointer);
  };
} // end namespace %(name)s_space
#endif
""" % joindict(
    { 'parentdecwrite' : (self.parent and """;
    %s_space::HDF5DCOWriteFunctor __parent__""" % self.parent) or "",
      'parentdecread' : (self.parent and """;
    %s_space::HDF5DCOReadFunctor __parent__""" % self.parent) or "" },
    self.__dict__))

        return ''.join(res)

    def printBodyCode(self):
        """code that is inserted in the body file. After all regular
        code, assumes global namespace"""

        # part 1, creation of datatype member function
        if self.nest:
            hdf5body = [ """
#if defined(DUECA_CONFIG_HDF5) && !defined(__CUSTOM_GETHDF5DATATYPE)%(nsopen)s
const H5::DataType* %(name)s::getHDF5DataType()
{
  static H5::CompType data_type(sizeof(__ThisDCOType__));
  static bool once = true;
  if (once) {
    __ThisDCOType__ example;""" % self.__dict__ ]
            if self.parent:
                hdf5body.append("""
    data_type.insertMember("%(parent)s", 0,
                           *%(parent)s::getHDF5DataType());""" %
                            self.__dict__)
            for m in self.members:
                hdf5body.append("""
    data_type.insertMember
      ("%(name)s", HOFFSET(__ThisDCOType__, %(name)s),
       *dueca::get_hdf5_type(example.%(name)s));""" % dict(name=m.getName()))
            hdf5body.append("""
    once = false;
  };
  return &data_type;
}%(nsclose)s
#endif
""" % self.__dict__)
        else:
            hdf5body = []

        # part 2, channel reading functor class + constructor
        hdf5body.append("""
#if defined(DUECA_CONFIG_HDF5)
// guarded in a separate namespace
namespace %(name)s_space {

#if !defined(__CUSTOM_HDF5_WRITE_FUNCTOR)
  HDF5DCOWriteFunctor::
  HDF5DCOWriteFunctor(boost::weak_ptr<H5::H5File> file,
                      const std::string& path,
                      size_t chunksize,
                      const std::string& label,
                      bool compress, bool writeticks,
                      const dueca::DataTimeSpec* startend) :
    dueca::hdf5log::HDF5DCOWriteFunctor(file, path, chunksize, label,
                               %(nmembers)s, compress, writeticks,
                               startend)%(parentcon)s
  {
    // add memspaces for all elements
""" % joindict(
    {'nmembers' : len(self.members),
     'parentcon' : (self.parent and """,
    __parent__(file, path + std::string("/__parent__"), chunksize,
               std::string(""), compress, false, startend)""") or "" },
    self.__dict__))
        i = 0
        for i, m in enumerate(self.members):
            hdf5body.append("""
    this->configureDataSet(%(idx)s, "/data/%(name)s",
                           HOFFSET(%(mastername)s, %(name)s),
                           dueca::get_hdf5_elt_type(example.%(name)s),
                           dueca::get_hdf5_elt_length(example.%(name)s));
""" % { 'idx' : i, 'mastername': self.name , 'name': m.getName()})
        hdf5body.append("""
    if (writeticks) {
      dueca::TimeTickType tex;
      this->configureDataSet(%(idx)s, "/tick", 0,
                             dueca::get_hdf5_elt_type(tex), 1);
    }
  }
""" % { 'idx' : i+1 })

        # part 2b, functor itself
        hdf5body.append("""
  // the functor member used by channel reading code, writes data in HDF5 file
  bool HDF5DCOWriteFunctor::operator() (const void* dpointer,
                                        const dueca::DataTimeSpec& ts)
  {
    while (ts.getValidityEnd() <= startend->getValidityStart()) {
      return true;
    }
    if (ts.getValidityStart() >= startend->getValidityEnd()) {
      return false;
    }%(chainparent)s
    this->prepareRow();
""" % { 'chainparent' : (self.parent and """
      __parent__.operator() (dpointer, ts);""") or "" })
        for i,m in enumerate(self.members):
            hdf5body.append("""
    this->sets[%(i)s].writeNew(dpointer, chunkidx, example.%(name)s);
""" % dict(i=i, name=m.getName()))

        hdf5body.append("""
    if (writeticks) {
      this->sets[%(nelts)s].writeNew(&ts);
    }
    return true;
  }
#endif
""" % { 'nelts': len(self.members) })

        # part 2c, read functor
        hdf5body.append("""

#if !defined(__CUSTOM_HDF5_READ_FUNCTOR)
  HDF5DCOReadFunctor::
  HDF5DCOReadFunctor(boost::weak_ptr<H5::H5File> file,
                     const std::string& path,
                     bool readticks) :
    dueca::hdf5log::HDF5DCOReadFunctor(file, path,
                              %(nmembers)s, readticks)%(parentcon)s
  {
    // add memspaces for all elements
""" % joindict(
    { 'parentcon' : (self.parent and """,
    __parent__(file, path + std::string("__parent__"), false)""") or "",
      'nmembers' : len(self.members) }, self.__dict__))
        i = 0
        for i, m in enumerate(self.members):
            hdf5body.append("""
    this->configureDataSet(%(idx)s, "/data/%(name)s",
                           HOFFSET(%(mastername)s, %(name)s),
                           dueca::get_hdf5_elt_type(example.%(name)s),
                           dueca::get_hdf5_elt_length(example.%(name)s));
""" % { 'idx' : i, 'mastername': self.name , 'name': m.getName()})
        hdf5body.append("""
    if (readticks) {
      dueca::TimeTickType tex;
      this->configureDataSet(%(idx)s, "/tick", 0,
                             dueca::get_hdf5_elt_type(tex), 1);
    }
  }
""" % { 'idx' : i+1 })

        # part 2b, functor itself
        hdf5body.append("""
  bool HDF5DCOReadFunctor::operator() (void* dpointer)
  {
""")
        if self.parent:
            hdf5body.append("""
    __parent__.operator() (dpointer);""")
        for i,m in enumerate(self.members):
            hdf5body.append("""
    this->sets[%(i)s].readObjectPart(dpointer, example.%(name)s);
""" % dict(i=i, name=m.getName()))

        hdf5body.append("""
    return true;
  }
#endif
""")

        # part 3, metafunctor class, also in the protected namespace
        hdf5body.append("""

  /** Metafunctor, can be accessed through the table, and can
      produce a functor object and the HDF5 data type */
  class HDF5DCOMetaFunctor: public dueca::hdf5log::HDF5DCOMetaFunctor
  {""")
        if self.nest:
            hdf5body.append("""
    const H5::DataType* operator() ()
    {
      return %(name)s::getHDF5DataType();
    }""" % self.__dict__)

        hdf5body.append("""
    HDF5DCOWriteFunctor* getWriteFunctor(boost::weak_ptr<H5::H5File> file,
                                         const std::string& path,
                                         size_t chunksize,
                                         const std::string& label,
                                         const dueca::DataTimeSpec* startend,
                                         bool compress,
                                         bool writeticks=true)
    {
      return new HDF5DCOWriteFunctor(file, path, chunksize, label,
                                     compress, writeticks, startend);
    }

    HDF5DCOReadFunctor* getReadFunctor(boost::weak_ptr<H5::H5File> file,
                                       const std::string& path,
                                       bool writeticks=true)
    {
      return new HDF5DCOReadFunctor(file, path, writeticks);
    }

  };

#if !defined(__DCO_STANDALONE)
  // loads the metafunctor in the table
  static dueca::LoadMetaFunctor<HDF5DCOMetaFunctor>
    load_functor(functortable, "hdf5");
#endif
} // end namespace %(name)s_space
#endif
"""  % self.__dict__)

        # part 4, any enum members that need a definition
        eclasses = set()
        for m in self.members:
            if not m.isEnum() or m.getType() in eclasses or \
                len(m.getMembers()) == 0:
                continue

            eclasses.add(m.getType())

            #print m.klassref.__class__
            hdf5body.append("""
#if defined(DUECA_CONFIG_HDF5) && !defined(__CUSTOM_HDF5_ENUM_%(barename)s)
namespace dueca {
  template<>
  const H5::DataType*
    get_hdf5_type(const %(klassname)s &o)
  {
    static H5::EnumType data_type(sizeof(%(ctype)s));
    static bool once = true;
    if (once) {
      %(klassname)s v;
""" % { 'klassname' : m.getType(), 'barename' :  m.getType(bare=True),
        'ctype' : m.getCType() })
            for name, barename in zip(m.getMembers(), m.getMembers(bare=True)):
                hdf5body.append("""
      v = %(name)s;
      data_type.insert("%(barename)s", &v);""" % { 'name' : name,
                                                   'barename': barename})
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
    const %(klassname)s &co = o;
    return get_hdf5_type(co);
#if 0
    static H5::EnumType data_type(sizeof(%(ctype)s));
    static bool once = true;
    if (once) {
      %(klassname)s v;
""" % { 'klassname' : m.getType(),
        'ctype' : m.getCType() })
            for name in m.getMembers():
                hdf5body.append("""
      v = %(name)s;
      data_type.insert("%(barename)s", &v);""" % { 'barename': barename,
                                                   'name' : name })
            hdf5body.append("""
      once = false;
    }
    return &data_type;
#endif
  }

  template<>
  const H5::DataType* get_hdf5_elt_type(%(klassname)s &o)
  { return get_hdf5_type(o); }

  template<>
  unsigned get_hdf5_elt_size(%(klassname)s &o)
  { return 1; }

} // end dueca namespace
#endif""" % { 'klassname' : m.getType() })
        return ''.join(hdf5body)

    def getCustomDefines(self):
        """these defines guard the implementation in the body, and can be used
        to override standard implementation. When the standard
        implementation is overridden, define
        __CUSTOM_COMPATLEVEL_HDF_# to indicate compatibility with a
        specific version of the hdf code. These are also checked for
        the general code generation compatibility level.

        """
        eclasses = set()
        res = []
        for m in self.members:
            if m.isEnum() and len(m.getMembers()) > 0 and \
                m.getType() not in eclasses:
                res.append('__CUSTOM_HDF5_ENUM_' + m.getType(bare=True))
        if self.nest:
            res.append('__CUSTOM_GETHDF5DATATYPE')
        res.append('__CUSTOM_HDF5_WRITE_FUNCTOR')
        res.append('__CUSTOM_HDF5_READ_FUNCTOR')
        return res
