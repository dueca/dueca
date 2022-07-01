/* ------------------------------------------------------------------   */
/*      item            : TransportNotification.cxx
        made by         : Rene' van Paassen
        date            : 010413
        category        : body file
        description     :
        changes         : 010413 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TransportNotification_cxx
#include "TransportNotification.hxx"
#include <GenericPacker.hxx>
#include <UnifiedChannel.hxx>
#include <dassert.h>
DUECA_NS_START

TransportNotification::
TransportNotification(GenericPacker* t, UnifiedChannel *c,
                      int idx, Channel::TransportClass tclass) :
  channel(c),
  transporter(t),
  idx(idx),
  tclass(tclass)
{
  //
}

TransportNotification::
~TransportNotification()
{
  //
}

void TransportNotification::trigger(UChannelEntry *entry,
                                    TimeTickType ts)
{
  transporter->notification(entry, ts, getIdx());
}

ostream& operator << (ostream& os, const TransportNotification& o)
{
  return os << "TransportNotification(ch=" << o.channel->getId()
            << ",idx=" << o.idx << ')';
}
DUECA_NS_END
