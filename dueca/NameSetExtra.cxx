/* ------------------------------------------------------------------   */
/*      item            : NameSetExtra.cxx
        made by         : Rene' van Paassen
        date            : 130928
        category        : addition dco object
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110
#include <dueca/visibility.h>
#include <sstream>

NameSet::NameSet(const std::string &e, const std::string &c,
                 const std::string &p) :
  name(p.size() ? c + string("://") + e + string("/") + p
                : c + string("://") + e)
{
  validate_set();
}

#include <boost/lexical_cast.hpp>

NameSet::NameSet(const std::string &e, const std::string &c, int p) :
  name(c + string("://") + e + string("/") +
       boost::lexical_cast<std::string>(p))
{
  validate_set();
}

improper_nameset::improper_nameset(const std::string &ns)
{
  message << "Name '" << ns
          << "' is not a proper nameset; need form of class://path";
}

  /** Re-implementation of std:exception what. */
const char *improper_nameset::what() const throw()
{ return message.str().c_str(); }

void NameSet::validate_set()
{
  size_t idxc = name.find("://");
  if (idxc == string::npos) {
    // no class specifier
    throw(improper_nameset(name));
  }

#if 0
  unsigned nelt = 0;
  size_t idx0 = idxc + 3;
  size_t idx1 = name.find('/', idx0);

  for ( ; idx0 != string::npos; idx0 = name.find('/', idx0+1)) {
    nelt++;
  }
#endif
}

std::string NameSet::getEntity() const
{
  size_t idxc = name.find("://");
  return name.substr(idxc + 3, name.find('/', idxc + 3) - idxc - 3);
}

std::string NameSet::getPart() const
{
  const std::string empty("");
  size_t idxc = name.find("://");
  size_t idxe = name.find('/', idxc + 3);
  if (idxe == string::npos)
    return empty;
  return name.substr(idxe + 1);
}

std::string NameSet::getClass() const
{
  return name.substr(0, name.find("://"));
}

#define __CUSTOM_FUNCTION_PRINT
std::ostream &NameSet::print(std::ostream &s) const
{
  s << this->name;
  return s;
}
