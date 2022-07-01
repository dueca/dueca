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
#include <cstring>

DUECA_NS_START

std::map<uint32_t,vstring> &LogCategory::explain()
{
  static  std::map<uint32_t,vstring> _explain;
  return _explain;
}

LogCategory::LogCategory(const char* name, const char* ex)
{
  strncpy(id.name, name, 4);
  id.name[4] = '\000';

  if (explain().find(id.i) != explain().end()) {
    cerr << "Logging category " << id.name << " already entered!" << endl;
    return;
  }
  explain()[id.i] = vstring(ex);
}

LogCategory::LogCategory(AmorphReStore& s)
{
  ::unPackData(s, id.i);
}

LogCategory::LogCategory()
{
  strcpy(id.name, "VOID");
}

LogCategory::~LogCategory()
{
  //
}

void LogCategory::packData(AmorphStore &s) const
{
  ::packData(s, id.i);
}

void LogCategory::unPackData(AmorphReStore &s)
{
  ::unPackData(s, id.i);
}

void LogCategory::print(std::ostream& os) const
{
  os << id.name;
}

void LogCategory::read(const std::string& s)
{
  strncpy(id.name, s.c_str(), 4);
}

const vstring& LogCategory::getExplain() const
{
  static const vstring unknown("No description entered");
  if (explain().find(id.i) != explain().end()) {
    return explain()[id.i];
  }
  return unknown;
}

template<>
const char* getclassname<LogCategory>() {return "LogCategory";}

DUECA_NS_END
