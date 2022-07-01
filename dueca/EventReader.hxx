/* ------------------------------------------------------------------   */
/*      item            : EventReader.hxx
        made by         : Rene van Paassen
        date            : 010402
        category        : header file
        description     :
        changes         : 010402 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef EventReader_hxx
#define EventReader_hxx

#include <EventAccessToken.hxx>
#include <DataReader.hxx>

#include <dueca_ns.h>
DUECA_NS_START
/** This is a facilitator for reading event data. By creating an
    "EventReader", the access token is used to gain access to the
    event in the channel. You can read out the data in the event, the
    time and the creator of the event. */
template<class T>
class EventReader: public DataReader<T,VirtualJoin>
{
public:
  /** Constructor. Gains access to the channel for which the token was
      made, and makes the next event available, given that it already
      exists at the specified time.
      \param token Read access token.
      \param ts    Time specification. Only events valid at or before the
                   start of the period defined in ts can be read. Default
                   value for ts is the end of time. */
  EventReader(EventChannelReadToken<T>& token,
              const TimeSpec& ts = TimeSpec::end_of_time) :
    DataReader<T, VirtualJoin>(token, ts)
  { }

  /** Destructor. Does nothing. */
  ~EventReader() { }

  /** Get the actual time of the event. */
  inline const SimTime getTime()
  {return SimTime(DataReader<T, VirtualJoin>::timeSpec().
                  getValidityStart()); }

  /** Get the actual tick of the event. */
  inline const TimeTickType getTick()
  {return DataReader<T,VirtualJoin>::timeSpec().getValidityStart(); }

  /** Get a reference to the event's creator id. */
  inline const GlobalId& getMaker()
  {return DataReader<T,VirtualJoin>::origin(); }

private:
  /** Copying is not possible, copy constructor is therefore private
      and not implemented. */
  EventReader(const EventReader<T>& );

  /** Assignment is not allowed, private and not implemented. */
  EventReader<T>& operator = (const EventReader<T>& );

  /** Only allocation on the stack is allowed, so new is certainly
      forbidden! */
  static void* operator new(size_t s);
};

DUECA_NS_END
#endif
