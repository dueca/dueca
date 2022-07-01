/* ------------------------------------------------------------------   */
/*      item            : ActivityBit.hh
        made by         : Rene' van Paassen
        date            : 5 sept 2000
        category        : header file
        description     : DUSIME event/stream data object
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ActivityBit_hxx
#define ActivityBit_hxx

#include <iostream>
#include "AmorphStore.hxx"
#include "TimeSpec.hxx"

#include <dueca_ns.h>
DUECA_NS_START
class ActivityWeaver;
class ActivityBit;
typedef ActivityBit* ActivityBitPtr;
class AmorphReStore;
class AmorphStore;
DUECA_NS_END

DUECA_NS_START

/** An object to contain the log data of one action of an
    ActivityManager. */
class ActivityBit
{
public:

  /// the type of action logged.
  enum ActivityBitType {
    LogStart,        /**< The start of a new log, dummy bit */
    Suspend,         /**< Indicates that the ActivityManager suspends
                           at this point */
    Start,           /**< Start of a new activity */
    Block,           /**< The current Activity blocks on input.
                          This is not a "normal" Activity's behaviour,
                          only Activities that do special stuff, such
                          as file/net IO, would do this. */
    Continue,        /**< A blocked activity continues processing */
    Graphics,        /**< This thread relinquishes control to the
                          graphics code, for an update of the
                          graphics. The next item to be logged is
                          either the start of a new activity, or a
                          suspend to wait for things to do. */
    Schedule         /**< An activity is scheduled for execution.
                            \todo This is currently not logged, and the
                          code in the ActivityLister is probably not
                          able to handle these. */
  };

  /// the offset of the time, in tick value, from the start of the log
  uint16_t tick_plus;

  /** Fractions are scaled portions of tick (granule) size. The
      full-scale value is given here. */
  static const unsigned int frac_max;

  /// the fraction of time within the tick, scaled 0 .. frac_max
  uint16_t frac_in_tick;

  /// the type of activity logged
  ActivityBitType type;

  /// an index describing the activity, use this index to get a full
  /// description from an ActivityDescriptionList
  uint16_t activity;

  /// the model time corresponding to the activity
  DataTimeSpec time_spec;

  /// a pointer giving the next ActivityBit in this list
  ActivityBitPtr next;
public:

  /// constructor
  ActivityBit(const uint16_t& tick_plus,
              const uint16_t& frac_in_tick,
              const ActivityBitType& type,
              const uint16_t& activity,
              const TimeSpec& time_spec);

  /// copy constructor, will in practice not be used
  ActivityBit(const ActivityBit& o);
public:

  /// constructor to restore an ActivityBit from amorphous storage
  ActivityBit(AmorphReStore& r);

  /** new operator "new", which places objects not on a
      heap, but in one of the memory arenas. This may prevent
      problems with asymmetric allocation */
  static void* operator new(size_t size);

  /** new operator "delete", to go with the new version
      of operator new. */
  static void operator delete(void* p);

  /// set the next bit in the chain and returns the same
  inline ActivityBitPtr setNext(ActivityBitPtr n) { return next = n;}

  /// find the next bit in the chain
  inline ActivityBitPtr getNext() const { return next;}

  /// destructor
  ~ActivityBit();

  /// returns a single-line description of the activity
  const vstring reportVerbal(int manager_number,
                           const ActivityWeaver* weaver) const;

  /// packs the ActivityBit into amorphous storage
  void packData(AmorphStore& s) const;

  /// prints the ActivityBit to a stream
  ostream& print (ostream& os) const;

  /// indicates when an ActivityBit logs the (re-)start of something
  inline bool goesActive() const
  {return type == Start || type == Continue || type == Graphics;}

  /** Only for a start or a continue, it makes sense to print the name
      of the activity. */
  inline bool hasNamedActivity() const
  {return type == Start || type == Continue;}

  /** indicates when an ActivityBit logs the end of something */
  inline bool goesInactive() const
  {return type == Suspend || type == Block;}

  /** transforms the time into a single 32bit int, useful for time
      comparison. */
  inline uint32_t getLongT() const
  { return ((uint32_t(tick_plus) << 16) & 0xffff0000) |
      (frac_in_tick & 0xffff);}

  /** Get the time as a floating point tick value, offset from the
      start of the log, i.e. scaled in granules and granule
      fractions. */
  inline double getFloatT() const
  {return double(tick_plus) + double(frac_in_tick) / double(frac_max);}

  /** Get the time as a floating point value in us, offset from the
      start of the log.
      \param fraction_mult   is the multiplier to get microseconds out
                             of fraction counts. */
  inline double getFloatT(double fraction_mult) const
  {return double(tick_plus)*(double(frac_max) * fraction_mult) +
      double(frac_in_tick)*fraction_mult; }

  /** Get the fraction of the integer tick. */
  inline float getFraction() const
  {return float(frac_in_tick)/float(frac_max);}

  /** Return a single beginletter indicating the type. */
  const char getLetter() const;
};

/** returns a string description of an ActivityBitType */
const char* const getString(const ActivityBit::ActivityBitType &o);

DUECA_NS_END

inline void packData(DUECA_NS ::AmorphStore& s,
                     const DUECA_NS ::ActivityBit& o)
{ o.packData(s); }

PRINT_NS_START
/** print an ActivityBit to a stream. */
inline ostream& operator << (ostream& s, const DUECA_NS ::ActivityBit& o)
{ return o.print(s); }

/** print the ActivityBitType to a stream */
inline ostream& operator << (ostream& s,
                             const DUECA_NS :: ActivityBit::ActivityBitType& o)
{ return s << getString(o); }
PRINT_NS_END

/// unpacks an ActivityBitType from amorphous storage
inline void unPackData(DUECA_NS :: AmorphReStore& s,
                       DUECA_NS :: ActivityBit::ActivityBitType& o)
{uint8_t tmp; ::unPackData(s, tmp);
 o = DUECA_NS :: ActivityBit::ActivityBitType(tmp);}

/// packs an ActivityBitType into amorphous storage
inline void packData(DUECA_NS :: AmorphStore& s,
                     const DUECA_NS :: ActivityBit::ActivityBitType& o)
{::packData(s, uint8_t(o));}

#endif

