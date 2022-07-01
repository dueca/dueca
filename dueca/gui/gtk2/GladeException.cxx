/* ------------------------------------------------------------------   */
/*      item            : GladeException.cxx
        made by         : Joost Ellerbroek
        date            : 100629
        category        : body file
        description     :
        changes         : 100629 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#include "GladeException.hxx"

DUECA_NS_START;

GladeException::GladeException(const GladeException &e) :
    reason(e.reason)
{
}

GladeException::GladeException(const std::string& reason) :
    reason(reason)
{
}

GladeException::~GladeException() throw()
{
}

std::ostream& GladeException::print(std::ostream& os) const
{
  os << "GladeException:";
  if (!reason.empty())
    os << ' ' << reason;
  return os;
}

DUECA_NS_END;
