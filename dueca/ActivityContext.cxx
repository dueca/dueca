/* ------------------------------------------------------------------ */
/*      item            : ActivityContext.cxx
        made by         : repa
        date            : Wed Nov 22 10:11:09 2006
        category        : body file
        description     : DUECA event/stream data object
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "ActivityContext.hxx"
#include <iostream>
#include <iomanip>
#include "Arena.hxx"
#include "ArenaPool.hxx"
#include "NodeManager.hxx"

DUECA_NS_START

ActivityContext::ActivityContext()
{
  parts.manager = 0xff;
  parts.node = static_node_id;
  parts.activity_id = 0;
}

ActivityContext::ActivityContext(const unsigned int& manager,
                                 const unsigned int& activity_id)
{
  parts.manager = manager;
  parts.node = static_node_id;
  parts.activity_id = activity_id;
}

ActivityContext::ActivityContext(const unsigned int& node,
                                 const unsigned int& manager,
                                 const unsigned int& activity_id)
{
  parts.manager = manager;
  parts.node = node;
  parts.activity_id = activity_id;
}



ActivityContext::ActivityContext(const ActivityContext& o) :
  total(o.total)
{
}

ActivityContext::ActivityContext(AmorphReStore& s) :
  total(s)
{
}

ActivityContext::~ActivityContext()
{
}

void ActivityContext::unPackData(AmorphReStore& s)
{
  ::unPackData(s, this->total);
}

void ActivityContext::packData(AmorphStore& s) const
{
  ::packData(s, this->total);
}

std::ostream & ActivityContext::print(std::ostream& s) const
{
  if (this->parts.manager == 0xff) {
    s << "not managed";
  }
  else if (this->parts.node == 0xff) {
    s << "unknown node";
  }
  else {
    s << "N" << std::setw(2) << this->parts.node
      << "/M" << std::setw(2) << this->parts.manager
      << "/A" << std::setw(4) << this->parts.activity_id;
  }
  return s;
}

istream& operator >> (istream&s, DUECA_NS::ActivityContext& o)
{
  s >> o.total;
  return s;
}

template <>
const char* getclassname<ActivityContext>() { return "ActivityContext"; }

DUECA_NS_END
