/* ------------------------------------------------------------------   */
/*      item            : GenericEvent.cxx
        made by         : Rene' van Paassen
        date            : 980209
        category        : body file
        description     : Event class
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define GenericEvent_cc

#include <GenericEvent.hxx>
#define E_CHN
#include <debug.h>

DUECA_NS_START

GenericEvent::GenericEvent(const GlobalId& maker_id, const TimeTickType&
                           time_stamp) :
  maker_id(maker_id), time_stamp(time_stamp), own_event_data(true)
{
  // no other things to do
}

GenericEvent::GenericEvent(AmorphReStore& source) :
    maker_id(source),
    time_stamp(source),
    own_event_data(true)
{
  //
}

GenericEvent::GenericEvent(const GenericEvent& ev) :
  maker_id(ev.maker_id),
  time_stamp(ev.time_stamp),
  own_event_data(true)
{
  //
}

GenericEvent::~GenericEvent()
{
  // no other things to undelete
}

std::ostream& GenericEvent::print(std::ostream& os) const
{
  return os << "GenericEvent(maker_id=" << maker_id
              << ",time_stamp=" << time_stamp << ")\n";
}

void GenericEvent::assumeDataOwnership(const GlobalId& new_owner) const
{
  if (!own_event_data) {
    /* DUECA Channels.

       A really obsolete problem, with an old-style generic event passing
       data ownership. */
    E_CHN("Event from " << maker_id
          << " cannot be transferred to " << new_owner);
    throw(CannotTransferOwnership(GlobalId(0,0), new_owner));
  }
  own_event_data = false;
}
DUECA_NS_END
