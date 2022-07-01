/* ------------------------------------------------------------------   */
/*      item            : TrimLink.cxx
        made by         : Rene' van Paassen
        date            : 010926
        category        : body file
        description     :
        changes         : 010926 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TrimLink_cxx
#include "TrimLink.hxx"
#include <dueca-conf.h>
#include <TrimView.hxx>
#include <dassert.h>
DUECA_NS_START

TrimLink::TrimLink(float value, float min_accept, float max_accept) :
  value(value),
  locked(true),
  min_accept(min_accept),
  max_accept(max_accept)
{
  //
}

void TrimLink::setValue(float v)
{
  if (locked) return;
  if (v < min_accept) v = min_accept;
  if (v > max_accept) v = max_accept;
  value = v;
}

ostream& operator << (ostream& os, const TrimLink& l)
{
  os << "TrimLink(value=" << l.value
     << ", locked=" << l.locked
     << ", min_accept=" << l.min_accept
     << ", max_accept=" << l.max_accept << ')';
  return os;
}

void* TrimLink::getTree()
{
#if defined(BUILD_DMODULES)
  // return TrimView::single()->getTree();
  return NULL;
#else
  assert(0);
  return NULL;
#endif
}
DUECA_NS_END
