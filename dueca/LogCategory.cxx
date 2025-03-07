/* ------------------------------------------------------------------   */
/*      item            : LogCategory.cxx
        made by         : Rene' van Paassen
        date            : 061117
        category        : body file
        description     :
        changes         : 061117 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define LogCategory_cxx
#include "LogCategory.hxx"
#include <iostream>
#include <dueca/dcoprint.hxx>

#include <cassert>
#if !defined(__DCO_NOPACK)
#include <dueca/AmorphStore.hxx>
#include <dueca/PackUnpackTemplates.hxx>
#endif
#include <dueca/DataWriterArraySize.hxx>
#define DOBS(A)
#if !defined(__DCO_STANDALONE)
#include <dueca/Arena.hxx>
#include <dueca/ArenaPool.hxx>
#include <dueca/DataClassRegistrar.hxx>
#include <dueca/CommObjectMemberAccess.hxx>
#include <dueca/DCOFunctor.hxx>
#include <dueca/DCOMetaFunctor.hxx>
#define DO_INSTANTIATE
#include <dueca/DataSetSubsidiary.hxx>
#endif

// getclassname implementation, always namespace dueca
namespace dueca {
  template<>
  const char* getclassname<LogCategory>()
  { return "LogCategory"; }
}

#if !defined(__DCO_STANDALONE)
// static CommObjectMemberAccess objects, that can provide flexible access
// to the members of a LogCategory object
static ::dueca::CommObjectMemberAccess
  <dueca::LogCategory,dueca::Dstring<5> >
  LogCategory_member_name(&dueca::LogCategory::name, "name");
  static ::dueca::CommObjectMemberAccess
  <dueca::LogCategory,uint32_t>
  LogCategory_member_i(&dueca::LogCategory::i, "i");

// assemble the above entries into a table in the order in which they
// appear in the LogCategory object
static const ::dueca::CommObjectDataTable entriestable[] = { 
  { &LogCategory_member_name },
   { NULL }
};
#endif

#include <cstring>

DUECA_NS_START

// magic number, hashed from class name and member names / classes
const uint32_t LogCategory::magic_check_number=0x26c8c043;

#if !defined(__DCO_STANDALONE)
// functor table, provides access to user-defined metafunctions through the
// data class registry
static functortable_type functortable;

// register this class, provides access to a packing/unpacking object,
// and to the member access tables
static DataClassRegistrar registrar
  (getclassname<LogCategory>(), NULL,
   entriestable, &functortable,
   new ::dueca::DataSetSubsidiary<LogCategory>());

#endif

std::map<uint32_t,vstring> &LogCategory::explain()
{
  static  std::map<uint32_t,vstring> _explain;
  return _explain;
}

LogCategory::LogCategory(const char* name, const char* ex)
{
  this->name = name;

  if (explain().find(i) != explain().end()) {
    cerr << "Logging category " << name << " already entered!" << endl;
    return;
  }
  explain()[i] = vstring(ex);
}

LogCategory::LogCategory(AmorphReStore& s)
{
  ::unPackData(s, i);
}

LogCategory::LogCategory()
{
  name = "VOID";
}

LogCategory::~LogCategory()
{
  //
}

void LogCategory::packData(AmorphStore &s) const
{
  ::packData(s, i);
}

void LogCategory::unPackData(AmorphReStore &s)
{
  ::unPackData(s, i);
}

void LogCategory::print(std::ostream& os) const
{
  os << name;
}

void LogCategory::read(const std::string& s)
{
  name = s;
}

const vstring& LogCategory::getExplain() const
{
  static const vstring unknown("No description entered");
  if (explain().find(i) != explain().end()) {
    return explain()[i];
  }
  return unknown;
}

DUECA_NS_END
