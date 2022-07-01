/* ------------------------------------------------------------------   */
/*      item            : GenericEvent.hh
        made by         : Rene' van Paassen
        date            : 980209
        category        : header file
        description     : base class for Event classes; implements
                          common properties typically handled by
                          channels and environment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef GenericEvent_hh
#define GenericEvent_hh

#include "dstypes.h"
#include "SimTime.hxx"
#include "GlobalId.hxx"

#include <dueca_ns.h>
DUECA_NS_START
/** Generic event class. Defines the generic event data, such as time
    stamp and originator of the event */

class GenericEvent
{
protected:

  /** Constructor. Note that making GenericEvent events is not
      useful, this constructur is usually called from a derived
      event. Application-level code seldom or never needs to construct
      events. Instead send (and implicitly create) events by using an
      EventWriter object. */
  GenericEvent(const GlobalId& maker_id, const TimeTickType& time_stamp);

  /** Copy constructor. */
  GenericEvent(const GenericEvent& ev);

  /** Constructs a GenericEvent from amorphous storage. */
  GenericEvent(AmorphReStore& source);

public:
  /** Destructor. */
  virtual ~GenericEvent();

  /** Return the time associated with the event. */
  inline const TimeTickType& getTime() const {return time_stamp;}

  /** Return the ID of the maker of the event. */
  inline const GlobalId& getMaker() const {return maker_id;}

protected:
  /// the maker of the event
  GlobalId maker_id;

  /// the time at which this event was created
  TimeTickType time_stamp;

protected:
  /** A flag to indicate whether event data is owned by this event,
      if this is so, the event data is deleted upon event deletion. */
  mutable bool own_event_data;

public:

  /** Packs the data of the GenericEvent into an amorphous storage
      object.
      Note that a GenericEvent by itself is not useful. Specific event
      types derived from it are useful, and this function is called
      by the packData function packing the derived event type. */
  inline void packData(AmorphStore& target) const
  {
    ::packData(target, maker_id);
    ::packData(target, time_stamp);
  }

  /** Print the generic event to stream. */
  ostream& print(ostream& os) const;

  /** Returns true if this event is earlier than another one. */
  inline bool operator < (const GenericEvent& e2) const
    { return time_stamp < e2.time_stamp;}

  /** Returns true if this event is later than another one. */
  inline bool operator > (const GenericEvent& e2) const
    { return time_stamp > e2.time_stamp;}

  /** Returns true if this event from the same time as another one. */
  inline bool operator == (const GenericEvent& e2) const
    { return time_stamp == e2.time_stamp;}

  /** This is a very dangerous function. It is used by DUECA itself to
      avoid copying huge objects (typically activity log data). It can
      only be used if the calling function is the only user of the
      event channel. Responsibility for destroying the event data will
      lie with the caller that took over ownership. Do not consider
      using this unless you are really really sure of yourself. */
  void assumeDataOwnership(const GlobalId& new_owner) const;
};

DUECA_NS_END

/// packs the GenericEvent into amorphous storage
inline void packData(DUECA_NS ::AmorphStore& s,
                     const DUECA_NS ::GenericEvent& o)
{ o.packData(s); }

PRINT_NS_START
/// prints the GenericEvent to a stream
inline ostream & operator << (ostream& s, const
                              DUECA_NS ::GenericEvent& o)
{ return o.print(s); }
PRINT_NS_END

#endif




