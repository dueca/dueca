/* ------------------------------------------------------------------   */
/*      item            : DataClassRegistry.hxx
        made by         : Rene van Paassen
        date            : 130120
        category        : header file
        description     :
        changes         : 130120 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataClassRegistry_hxx
#define DataClassRegistry_hxx

#include <inttypes.h>
#include "dueca_ns.h"
#include "DataClassRegistryPredef.hxx"

DUECA_NS_START;

/** Global registry for information about Dueca Communication Object
    (DCO) types.

    Note that most of the interfaces of this class are being used
    internally by DUECA. For some external projects it may be useful
    to inspect DCO classes.
*/
class DataClassRegistry
{
  /** the registry map links the classname to an entry that contains
      all necessary meta-information. */
  typedef DataClassRegistry_map_type  map_type;

  /** iterator for running over the map */
  typedef map_type::const_iterator map_iterator;

  /** The map with the actual entries. */
  map_type entries;

  /** Constructor */
  DataClassRegistry();

  /** Destructor */
  ~DataClassRegistry();

public:
  /** type of a single entry in the map -- external */
  typedef DataClassRegistry_entry_type entry_type;

  /** Singleton access */
  static DataClassRegistry& single();


  /** Access the table with communication objects */
  const CommObjectDataTable* getTable(const std::string& classname);

  /** Return a quick-access entry index given a classname

      This returns a plain C++ pointer.
      @param classname   name of the data class
      @returns           An index to the entry
  */
  DataClassRegistry_entry_type getEntry(const std::string& classname);

  /** Return a quick-access entry index given a classname

      Contrary to the above call, this returns a shared pointer.
      @param classname   name of the data class
      @returns           An index to the entry
  */
  map_type::mapped_type getEntryShared(const std::string& classname);

private:
  /** registration of a new DCO object type; is commonly done
      automatically, at start-up time. */
  void registerClass(const char* classname,
                     const char* parent,
                     const CommObjectDataTable* table,
                     const functortable_type* functortable,
                     const DataSetConverter* converter);

  /** Helper function to complete inheritance information links */
  unsigned completeIndices(map_type::mapped_type ix);

  /** Only the DataClassRegistrar can supply new DCO types */
  friend class DataClassRegistrar;

public:
  /** Get an accessor to data member with name membername. If the member name
      is not available in the present class, parent classes are tested.
      @param ix          index previously returned by getEntry call
      @param membername  name of the data member
      @returns           A pointer to the member accessor, which in
                         turn can be used to access the data.
  */
  const CommObjectMemberAccessBasePtr& getMemberAccessor
  (DataClassRegistry_entry_type ix, const std::string& membername);

  /** Check whether a class is registered */
  bool isRegistered(const std::string& classname);

  /** get an accessor to data member index idx. Note that data members
      indices count from the parents
      @param ix          index returned by the getEntry call
      @param idx         number of the data member
      @returns           A pointer to the member accessor, which in
                         turn can be used to access the data.
  */
  const CommObjectMemberAccessBasePtr& getMemberAccessor
  (DataClassRegistry_entry_type ix, unsigned idx);

  /** Retrieve the name of a member, given a dataclass entry.
      @param ix          index returned by the getEntry call
      @param idx         number of the data member */
  const char* getMemberName(DataClassRegistry_entry_type ix, unsigned idx) const;

  /** Retrieve the offset number of a member, given the dataclass entry
      and member name

      @param ix          index of the class, returned by getEntry call
      @param name        name of the data member
 */
  const unsigned getMemberIndex(DataClassRegistry_entry_type ix, const std::string& name);

  /** Return the number of members in a class

      @param ix          index of the class, returned by getEntry call
  */
  size_t getNumMembers(DataClassRegistry_entry_type ix) const;

  /** Get the magic number.

      The magic number is hashed for each data class. It is used as a
      check on consistency, when communicating with other DUECA
      processes.

      @param ix          index of the class, returned by getEntry call
  **/
  uint32_t getMagic(DataClassRegistry_entry_type ix) const;

  /** Get the classname of a specific entry type

      @param ix          index of the class, returned by getEntry call
  */
  const char* getEntryClassname(DataClassRegistry_entry_type ix) const;

  /** Get the parent class for the data type given in the classname

      @param classname   child class name
      @returns           name of the parent class, or '' if no parent
                         available.
  */
  const std::string& getParent(const std::string& classname);

  /** See whether a class is a parent of (compatible with reading) 
      another class.
      
      @param tryclass    Class you want to access/read with
      @param classname   Class given. 
      */
  bool isCompatible(const std::string& tryclass, 
                    const std::string& classname);

  /** Get a pointer to the dataset converter for this data type.

      @param classname   data type class name
      @returns           a pointer to the converter */
  const DataSetConverter* getConverter(const std::string& classname) const;

  /** Get a metafunctor, searched by name

      Functors are used to implement specific services on dataclasses.
      They typically need to contain state information (like, "to
      which file do I write"), and are thus client-specific. This
      returns the metafunctor, which is only type-specific, and can be
      asked to return functors. The returned functors can be used in
      combination with a channel read or write token.

      @param classname   data class name
      @param fname       functor name
      @returns           weak reference to base functor.
  */
  std::weak_ptr<DCOMetaFunctor>
  getMetaFunctor(const std::string& classname, const std::string& fname) const;
};

DUECA_NS_END;

#endif
