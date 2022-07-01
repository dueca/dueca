/* ------------------------------------------------------------------   */
/*      item            : SimTime.hh (provisional)
        made by         : Rene' van Paassen
        date            : 980223
        category        : header file
        description     : Time in a simulation; interpreted as long
                          int, no of ticks of the finest clock
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef SimTime_hh
#define SimTime_hh

#ifdef SimTime_cc
#endif

#include "AmorphStore.hxx"
#include <dueca_ns.h>
DUECA_NS_START

typedef uint32_t TimeTickType;

/** This is the integer (model) time of a simulation or process. The
    only method that is used by application programmers is
    getTimeTick, which returns the current tick.

    \todo Remove this archaic piece of junk. */
class SimTime
{
private:

  /** Tick defining this time instant. */
  TimeTickType tick;

  /** The current tick in the simulation. */
  static TimeTickType base_tick;


  friend class Environment;
  friend class Ticker;

  /** Increment the current time in the simulation with increment. */
  static inline void advanceTime(TimeTickType increment)
    {base_tick += increment;}

  /** Jump the current time in the simulation to the new time given in
      the argument. */
  static inline void setTime(TimeTickType new_time) {base_tick = new_time;}

public:
  /// Constructor
  SimTime();

  /// Make a time point from a time
  SimTime(TimeTickType tim);

  /// Construct from packed storage
  SimTime(AmorphReStore& source);

  /// Destructor.
  ~SimTime();


  /** Print to stream, debugging purposes. */
  ostream & print (ostream& s) const;

  /** Returns true if the second time is smaller than this one. */
  inline bool operator < (const SimTime& t2) const
    { return tick < t2.tick;}

  /** Returns true if the second time is larger than this one. */
  inline bool operator > (const SimTime& t2) const
    { return tick > t2.tick;}

  /** Returns true if the second time is equal to this one. */
  inline bool operator == (const SimTime& t2) const
    { return tick == t2.tick;}

  /** Returns true if the second time is not equal to this one. */
  inline bool operator != (const SimTime& t2) const
    { return tick != t2.tick;}

  /** Return the current time tick. */
  static inline TimeTickType getTimeTick() {return base_tick;}

  /** Return a time object made from the current tick. */
  static inline const TimeTickType now() {return base_tick;}

  /** Return the tick value from this time object. */
  inline TimeTickType getTick() const {return tick;}

  /** Add a time to this time object. */
  inline SimTime& operator += (TimeTickType tim)
    { tick += tim; return *this; }

  /** Test whether this time object lies in the future. */
  inline bool future() const {return tick > base_tick;}
  /** Test whether this time object lies in the present. */
  inline bool present() const {return tick == base_tick;}
  /** Test whether this time object lies in the past. */
  inline bool past() const {return tick < base_tick;}
};

DUECA_NS_END

PRINT_NS_START
/// prints the SimTime to a stream
inline ostream & operator << (ostream& s, const
                              DUECA_NS ::SimTime& o)
{ return o.print(s); }
PRINT_NS_END

#endif
