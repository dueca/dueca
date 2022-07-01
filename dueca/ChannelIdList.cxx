/* ------------------------------------------------------------------   */
/*      item            : ChannelIdList.cxx
        made by         : Rene' van Paassen
        date            : 980319
        category        : header file
        description     :
        notes           : object for keeping channel data in the registry.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "ChannelIdList.hxx"
#include "ChannelEndUpdate.hxx"
#include <iostream>
DUECA_NS_START

ChannelIdList::ChannelIdList() :
  ns(NameSet("", "", "")),
  channel_id(),
  channel(NULL)
{
  // nothing more, other data added with other calls
}

ChannelIdList::ChannelIdList(const NameSet& ns,
                             UnifiedChannel* channel) :
  ns(ns), channel(channel)
{
  // nothing more, other data added with other calls
}

ChannelIdList::ChannelIdList(const ChannelIdList& l) :
  ns(l.ns),
  channel_id(l.channel_id),
  channel(l.channel)
{
  // wasn't that enough??
}

ChannelIdList::~ChannelIdList()
{
  // nothing?
}

ChannelIdList& ChannelIdList::operator= (const ChannelIdList& l)
{
  if (this != &l) {
    ns = l.ns;
    channel_id = l.channel_id;
    channel = l.channel;
  }
  return *this;
}

void ChannelIdList::adjustChannelEnd(const ChannelEndUpdate& u)
{
  switch (u.update) {
  case ChannelEndUpdate::ID_ISSUED:
    channel_id = u.end_id;
    break;

  case ChannelEndUpdate::DELETE_END:
    channel = 0;
    break;
  default:
    break;
  }
}

std::ostream& ChannelIdList::print (std::ostream& o) const
{
  o << "ChannelIdList(name=" << ns
    << ", channel_id=" << channel_id
    << ", channel=" << channel;
  return o << ")\n";
}
DUECA_NS_END
