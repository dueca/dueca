/* ------------------------------------------------------------------   */
/*      item            : DataClassRegistryPredef.hxx
        made by         : Rene van Paassen
        date            : 170327
        category        : header file
        description     : Header pre-defines for DCO and dataclass-
                          related classes
        changes         : 170327 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 17 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataClassRegistryPredef_hxx
#define DataClassRegistryPredef_hxx

#include <dueca_ns.h>
#include <Exception.hxx>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <dueca/visibility.h>

DUECA_NS_START;

class CommObjectMemberAccessBase;
typedef CommObjectMemberAccessBase* CommObjectMemberAccessBasePtr;
struct CommObjectDataTable;
class DCOMetaFunctor;
class DCOFunctor;
typedef std::map<std::string,boost::shared_ptr<dueca::DCOMetaFunctor> >
functortable_type;
class DataSetConverter;
struct DCRegistryEntry;
typedef std::map<const std::string,boost::shared_ptr<DCRegistryEntry> >
  DataClassRegistry_map_type;

/** Entry of the DataClassRegistry */
//typedef DataClassRegistry_map_type::mapped_type DataClassRegistry_entry_type;
typedef const DCRegistryEntry* DataClassRegistry_entry_type;

/** Exception thrown when the class name searched has not been registered */
class DataObjectClassNotFound: public MsgException<128>
{
public:
  /** Constructor

      @param classname  Name for missing class*/
  DataObjectClassNotFound(const std::string& classname);
};

/** Double entry of a dataclass */
class DataObjectClassDoubleEntry: public MsgException<128>
{
public:
  /** Constructor

      @param classname   Name for the class
  */
  DataObjectClassDoubleEntry(const std::string& classname);
};

/** Member not present in the dataclass */
class DataClassMemberNotFound: public MsgException<128>
{
public:
  /** Constructor

      @param klass   Name for the class
      @param mbmr    Missing member
  */
  DataClassMemberNotFound(const char* klass, const std::string& mbmr);
};

/** Incorrect functor conversion attempted */
class FunctorTypeMismatch: public std::exception
{
public:
  /** To print. */
  const char* what() const throw()
  { return "Returned functor base cannot be cast to requested class"; }
};

/** Functor type not defined for the class */
class UndefinedFunctor: public MsgException<128>
{
public:
  /** Constructor

      @param msg     Message
  */
  UndefinedFunctor(const std::string& msg);
};

DUECA_NS_END;

#endif
