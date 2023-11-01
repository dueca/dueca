/* ------------------------------------------------------------------   */
/*      item            : DataClassRegistry.cxx
        made by         : Rene' van Paassen
        date            : 130120
        category        : body file
        description     :
        changes         : 130120 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DataClassRegistry_cxx
#include <inttypes.h>

#include "DataClassRegistry.hxx"
#include <CommObjectMemberAccess.hxx>
#include <DataSetConverter.hxx>
#include <dassert.h>
#include <map>
#include <debug.h>
#include <string>
#include <boost/scoped_ptr.hpp>

DUECA_NS_START;


/** Comparison for CommObjectMemberAccessBasePtr names */

/** This describes the entries in the registry. */
struct DCRegistryEntry
{
  /** Name of the parent, if applicable */
  std::string parent;

  /** Pointer to the parent, again, if applicable */
  std::shared_ptr<DCRegistryEntry> iparent;

  /** Table with member name - access object pairs. */
  const CommObjectDataTable* table;

  /** Set with all CommObjectMemberAccess pointers */
  typedef std::map<const std::string,
                   CommObjectMemberAccessBasePtr> mab_map_type;

  /** Set with all CommObjectMemberAccess pointers */
  mab_map_type memberaccess_map;

  /** Object that can convert a byte stream into the associated object. */
  boost::scoped_ptr<const DataSetConverter> converter;

  /** Hash-indexed list of member access objects. */
  const CommObjectDataTable* quickset;

  /** Size of the hash divisor = size of the above list */
  unsigned nquick;

  /** Initialisation of hash function */
  uint32_t initquick;

  /** Table of meta functors. */
  const functortable_type* functortable;

  /** Number of members */
  unsigned n_members;

  /** Number of members in parents */
  unsigned n_in_parents;

public:
  DCRegistryEntry(const char* parent,
                  const CommObjectDataTable* table,
                  const functortable_type* functortable,
                  const DataSetConverter* converter) :
    parent(parent ? parent : ""),
    iparent(),
    table(table),
    converter(converter),
    functortable(functortable),
    n_members(std::numeric_limits<unsigned>::max()),
    n_in_parents(0)
  {
    //
  }
};


DataClassRegistry::DataClassRegistry()
{
  //
}


DataClassRegistry& DataClassRegistry::single()
{
  static DataClassRegistry* one = new DataClassRegistry();
  return *one;
}

void DataClassRegistry::registerClass(const char* classname,
                                      const char* parent,
                                      const CommObjectDataTable* table,
                                      const functortable_type* functortable,
                                      const DataSetConverter* converter)
{
  std::string classname_string(classname);
  if (entries.find(classname) != entries.end()) {
    throw(DataObjectClassDoubleEntry(classname));
  }
  entries.insert(map_type::value_type
                 (std::string(classname),
                  std::shared_ptr<DCRegistryEntry>
                  (new DCRegistryEntry(parent, table,
                                       functortable, converter))));
  assert(entries.find(std::string("")) == entries.end());
}

const CommObjectDataTable*
DataClassRegistry::getTable(const std::string& classname)
{
  map_type::const_iterator ix = entries.find(classname);
  if (ix == entries.end()) throw(DataObjectClassNotFound(classname));
  return ix->second->table;
}

bool DataClassRegistry::isRegistered(const std::string& classname)
{
  return bool(entries.count(classname));
}

inline uint32_t djb2(const std::string& s, uint32_t hash)
{
  for (std::string::const_iterator it = s.begin(); it != s.end(); it++) {
    hash = (hash << 5) + hash + uint32_t(*it);
  }
  return hash;
}

DataClassRegistry_entry_type DataClassRegistry::getEntry
(const std::string& classname)
{
  map_type::iterator ix = entries.find(classname);
  if (ix == entries.end()) throw(DataObjectClassNotFound(classname));
  completeIndices(ix->second);
  return ix->second.get();
}

DataClassRegistry::map_type::mapped_type DataClassRegistry::getEntryShared
(const std::string& classname)
{
  map_type::iterator ix = entries.find(classname);
  if (ix == entries.end()) throw(DataObjectClassNotFound(classname));
  completeIndices(ix->second);
  return ix->second;
}

const CommObjectMemberAccessBasePtr& DataClassRegistry::
getMemberAccessor(DataClassRegistry_entry_type ix, const std::string& membername)
{
  const auto res = ix->memberaccess_map.find(membername);
  if (res != ix->memberaccess_map.end()) {
    return res->second;
  }

  // member not found
  throw(DataClassMemberNotFound(ix->converter->getClassname(),
                                membername));
}

const CommObjectMemberAccessBasePtr&
DataClassRegistry::getMemberAccessor(DataClassRegistry_entry_type ix, unsigned  idx)
{
  if (idx >= ix->n_members) {
    throw(DataClassMemberNotFound
          (ix->converter->getClassname(),
           std::string("#") + boost::lexical_cast<std::string>(idx)));
  }
  if (idx >= ix->n_in_parents)
    return ix->table[idx - ix->n_in_parents].access;
  return this->getMemberAccessor(ix->iparent.get(), idx);
}

/* Helper, searches for a named entry in the entry table, and returns
   the index of the entry. Parent entries are indexed first, therefore
   the search starts there. */
static bool _searchMember(DataClassRegistry_entry_type ix,
                          const char* member, unsigned& idx)
{
  // go up first
  if (ix->iparent) {
    if (_searchMember(ix->iparent.get(), member, idx)) {
      return true;
    }
  }

  // search my own entries
  const CommObjectDataTable *res = ix->table;
  while (res->access) {
    if (!strcmp(res->access->getName(), member)) return true;
    idx++; res++;
  }
  return false;
}

const unsigned DataClassRegistry::getMemberIndex
(DataClassRegistry_entry_type ix, const std::string& name)
{
  // see if the associated pointer in the table is not NULL.
  unsigned idx = 0;

  if (_searchMember(ix, name.c_str(), idx)) {
    return idx;
  }

  throw(DataClassMemberNotFound
        (ix->converter->getClassname(), name));
}


const char* DataClassRegistry::getMemberName(DataClassRegistry_entry_type ix, unsigned idx) const
{
  if (idx >= ix->n_members) {
    throw(DataClassMemberNotFound
          (ix->converter->getClassname(),
           std::string("#") + boost::lexical_cast<std::string>(idx)));
  }
  if (idx >= ix->n_in_parents) {
    return ix->table[idx - ix->n_in_parents].access->getName();
  }
  return this->getMemberName(ix->iparent.get(), idx);
}

size_t DataClassRegistry::getNumMembers(DataClassRegistry_entry_type ix) const
{
  return ix->n_members;
}

uint32_t DataClassRegistry::getMagic(DataClassRegistry_entry_type ix) const
{
  return ix->converter->getMagic();
}

const char* DataClassRegistry::getEntryClassname(DataClassRegistry_entry_type ix) const
{
  return ix->converter->getClassname();
}

unsigned DataClassRegistry::completeIndices(map_type::mapped_type ix)
{
  if (ix->n_members == std::numeric_limits<unsigned>::max()) {
    ix->n_members = 0;
    for (const CommObjectDataTable* ip = ix->table; ip->access; ++ip) {
      ix->n_members++;
      ix->memberaccess_map[ip->access->getName()] = ip->access;
    }
    if (ix->parent != std::string("")) {
      ix->iparent = this->getEntryShared(ix->parent);
      ix->n_in_parents = this->completeIndices(ix->iparent);
      for (const auto& pm: ix->iparent->memberaccess_map) {
        if (ix->memberaccess_map.count(pm.first)) {
          /* DUECA channel.

             A transportable (DCO) data type has a member that shadows
             a member in one of its parent data types. Check your .dco
             files. */
          W_MOD("DCO " << ix->converter->getClassname() << " member \"" <<
                pm.first << "\" shadows member in parent");
        }
        else {
          ix->memberaccess_map[pm.first] = pm.second;
        }
      }
      ix->n_members += ix->n_in_parents;
    }
  }
  return ix->n_members;
}

const std::string&
DataClassRegistry::getParent(const std::string& classname)
{
  map_type::const_iterator ix = entries.find(classname);
  if (ix == entries.end()) throw(DataObjectClassNotFound(classname));
  return ix->second->parent;
}

const DataSetConverter*
DataClassRegistry::getConverter(const std::string& classname) const
{
  map_type::const_iterator ix = entries.find(classname);
  if (ix == entries.end()) throw(DataObjectClassNotFound(classname));
  return ix->second->converter.get();
}

bool DataClassRegistry::isCompatible(const std::string& tryclass, 
                                     const std::string& classname)
{
  map_type::const_iterator ix = entries.find(classname);
  if (ix == entries.end()) throw(DataObjectClassNotFound(classname));
  if (tryclass == classname) return true;
  std::shared_ptr<DCRegistryEntry> ip = ix->second->iparent;
  while (ip->iparent) {
    if (ip->parent == tryclass) return true;
    ip = ip->iparent;
  }
  return false;
}

std::weak_ptr<DCOMetaFunctor>
DataClassRegistry::getMetaFunctor(const std::string& classname,
                                  const std::string& fname) const
{
  map_type::const_iterator ix = entries.find(classname);
  if (ix == entries.end()) throw(DataObjectClassNotFound(classname));

  functortable_type::const_iterator fi = ix->second->functortable->find(fname);
  if (fi == ix->second->functortable->end()) throw(UndefinedFunctor(classname));

  return std::weak_ptr<DCOMetaFunctor>(fi->second);
}

DataObjectClassNotFound::DataObjectClassNotFound(const std::string& msg) :
  MsgException("Registry does not contain DCO class ", msg.c_str())
{ }

DataObjectClassDoubleEntry::DataObjectClassDoubleEntry(const std::string& msg) :
  MsgException("DCO class ", msg.c_str(), " multiply defined")
{ }

DataClassMemberNotFound::DataClassMemberNotFound(const char* klass,
                                                 const std::string& mmbr) :
  MsgException("Data class ", klass, " has no member ", mmbr.c_str())
{ }

UndefinedFunctor::UndefinedFunctor(const std::string& msg) :
  MsgException("functor type is not defined for DCO class ", msg.c_str())
{ }

DUECA_NS_END;
