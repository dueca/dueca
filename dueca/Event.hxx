/* ------------------------------------------------------------------   */
/*      item            : Event.hh
        made by         : Rene' van Paassen
        date            : 980209
        category        : header file
        description     : Event class
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef Event_hh
#define Event_hh

#include "dstypes.h"
#include "GenericEvent.hxx"

// forward declarations
#include <dueca_ns.h>
DUECA_NS_START

/** The normal event class. This is a parametrized class, an event may
    carry an arbitrary data type (but currently only one type) */

template <class T>
class Event: public GenericEvent
{

public:

  /** Constructor with a pointer to the data. The calling object
      must make the data object, and pass it to the new Event. After
      use (everyone has received the event), data and event are
      destroyed. the target_time is the model time for the event.

      Note that this function is not called by application programs,
      use an EventWrite instead. */
  Event(const T* data, const GlobalId& sender_id,
        const TimeTickType& time);

  /** Constructor that uses a data reference. Safer, but more
      expensive in the absence of optimization. In other aspects this
      constructor is equal to the above one. */
  Event(const T& data, const GlobalId& sender_id,
        const TimeTickType& target_time);

  /** Constructor on the basis of an Amorphous storage object. For
      events "coming in" from the net */
  Event(AmorphReStore& source);

  /// Copy constructor
  Event(const Event<T> &o);

  /// Destructor
  ~Event();

  /** Conversion routine to net representation. Converts both the data
      and the event-specific stuff, such as timing, etc. */
  void packData(AmorphStore& s) const;

  /** Overloaded operator for printing of the event to a stream. This
      is mainly used for debugging purposes. */
  ostream & print (ostream& s) const;

  /** Insert the corresponding data into the event object.
      @param data       DCO object pointer
      @param sender_id  Identification of origin
      @param time       Time of event. */
  void setData(const T* data, const GlobalId& sender_id,
               const TimeTickType& time);

public:

  /** Access the data of an event. */
  inline const T *getEventData() const {return data; };

private:
  /** A pointer to the data object. */
  const T *data;
};

DUECA_NS_END

/// packs the Event<T> into amorphous storage
template<class T>
inline void packData(DUECA_NS ::AmorphStore& s,
                     const DUECA_NS ::Event<T>& o)
{ o.packData(s); }

PRINT_NS_START
/// prints the Event<T> to a stream
template<class T>
inline ostream & operator << (ostream& s, const
                       DUECA_NS ::Event<T>& o)
{ return o.print(s); }
PRINT_NS_END
#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef Event_ii
#define Event_ii

#include <dueca_ns.h>
DUECA_NS_START
template <class T> Event<T>::
Event(const T* data, const GlobalId& sender_id, const TimeTickType& target_time) :
  GenericEvent(sender_id, target_time),
  data(data)
{
  // no other things to do
}

template <class T> Event<T>::
Event(const T& data, const GlobalId& sender_id, const TimeTickType& target_time) :
  GenericEvent(sender_id, target_time),
  data(new T(data))
{
  // no other things to do
}

template <class T> Event<T>::
Event(AmorphReStore& source) :
  GenericEvent(source),
  data(new T(source))
{
  //
}

template <class T> Event<T>::
Event(const Event<T> &o) :
  GenericEvent(o),
  data(new T(*o.data))
{
  //
}

template <class T>
void Event<T>::packData(AmorphStore& target) const
{
  ::packData(target, *static_cast<const GenericEvent*>(this));
  ::packData(target, *(data));
}

template <class T> Event<T>::
~Event()
{
  if (own_event_data)
    delete(data);
}

template<class T>
ostream&  Event<T>::print (ostream& s) const
{
  return s << "Event<T>(" << *static_cast<const GenericEvent*>(this)
           << ",data=" << *(data) << ')';
}

template<class T>
void Event<T>::setData(const T* data, const GlobalId& sender_id, const TimeTickType& target_time)
{
  this->maker_id = sender_id;
  this->time_stamp = target_time;
  this->data = data;
}


DUECA_NS_END
#endif
#endif





