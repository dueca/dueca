/* ------------------------------------------------------------------   */
/*      item            : TimeSpec.hh
        made by         : Rene' van Paassen
        date            : 990802
        category        : header file
        description     :
        changes         : 990802 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef TimeSpec_hh
#define TimeSpec_hh

#ifdef TimeSpec_cc
#endif

#include <iostream>
#include <inttypes.h>
#include "ScriptCreatable.hxx"
#include <exception>
#include <dueca/visibility.h>

using namespace std;

#include <dueca_ns.h>
DUECA_NS_START

// advance definitions
typedef uint32_t TimeTickType;
const TimeTickType MAX_TIMETICK = 0xffffffff;
struct DataTimeSpec;
class AmorphStore;
class AmorphReStore;
class TimeSpec;
struct DataTimeSpec;
class PeriodicTimeSpec;
struct ParameterTable;

/** Subtract two tick values, consider they are unsigned. */
inline int subtractTicks(TimeTickType t1, TimeTickType t2)
{
  if (t1 > t2) {
    return int(t1 - t2);
  }
  else {
    return -int(t2 - t1);
  }
}

/** Exception to throw when subtraction of time is illogical */
class LNK_PUBLIC TimeSpecSubtractFailed: public std::exception
{
public:
  /** Constructor */
  TimeSpecSubtractFailed() {};

  /** To print. */
  const char* what() const throw()
  {return "Cannot subtract; spans not equal";}
};


/** A TimeSpec is a specification for a time interval. It is given in
    integer time steps, and defines the step from which the time
    interval is valid, and the step from which it is no longer valid. */
class TimeSpec
{
protected:
  /** The time interval starts here. */
  TimeTickType validity_start;

  /** The time interval ends \em before this. */
  TimeTickType validity_end;

public:
  /// Copy constructor
  TimeSpec(const TimeSpec&);

  /** Complete constructor, with the start and end time.
      \param validity_start   time tick at which data becomes valid
      \param validity_end     time tick at which data (calc, etc.)
                              becomes invalid. */
  TimeSpec(TimeTickType validity_start,
           TimeTickType validity_end);

  /** Complete constructor, with the start and end time.

      This time an integer variation, since often simply integer
      arguments are given.

      \param validity_start   time tick at which data becomes valid
      \param validity_end     time tick at which data (calc, etc.)
                              becomes invalid. */
  TimeSpec(int validity_start, int validity_end);

  /** Constructor for a TimeSpec that starts and ends at the same
      time. For example events are like this. */
  TimeSpec(TimeTickType validity_start);

  /** Constructor with double arguments.

      Note that the argument will be converted to time ticks, and that
      rounding off may occur.

      If start and end are not exactly (as in floating point exactly)
      equal, the integer end time tick will always be at least one
      higher than the start time tick. This is to prevent creation of
      0 timespans, which can produce infinite loops in clocks.

      \param validity_start   time (in s!) at which data becomes valid
      \param validity_end     time (in s!) at which data (calc, etc.)
                              becomes invalid. */
  TimeSpec(double validity_start,
           double validity_end);

  /** Variation with float.

      Note that the argument will be converted to time ticks, and that
      rounding off may occur.

      If start and end are not exactly (as in floating point exactly)
      equal, the integer end time tick will always be at least one
      higher than the start time tick. This is to prevent creation of
      0 timespans, which can produce infinite loops in clocks.

      \param validity_start   time (in s!) at which data becomes valid
      \param validity_end     time (in s!) at which data (calc, etc.)
                              becomes invalid. */
  TimeSpec(float validity_start, float validity_end);


  /** Constructor with a DataTimeSpec as input */
  TimeSpec(const DataTimeSpec&);

public:

  /// Default constructor
  TimeSpec();

  /// Destructor
  virtual ~TimeSpec();

  /** This clones a TimeSpec, giving back a pointer to an identical
      copy. Also clones the derived classes. */
  virtual TimeSpec* clone() const;

  /** A special time spec at the end of time. */
  static const TimeSpec end_of_time;

  /** Timespec at the start of time. */
  static const TimeSpec start_of_time;

public:

  /** The time spec at the end of time. */
  static const TimeSpec& endOfTime() { return end_of_time;}

  /** The time spec at the beginning of time. */
  static const TimeSpec& startOfTime() { return start_of_time;}

  /** Move the time forward with the data given in another TimeSpec.
      \param a TimeSpec with new times. */
  virtual bool advance(const TimeSpec& a);

  /** Move the time forward with the data given in a DataTimeSpec
      \param a TimeSpec with new times. */
  virtual bool advance(const DataTimeSpec& a);

  /** Move the time to the next contiguous interval, that ends at the
      end tick specified in the parameter. */
  virtual bool advance(const TimeTickType& validity_end = MAX_TIMETICK);

  /** Jump ahead in time, possibly leaving a gap. This does not do
      great wonders for a TimeSpec, but a PeriodicTimeSpec (one of the
      derived classes) has different behaviour to an advance and a
      forceAdvance. */
  virtual bool forceAdvance(const TimeTickType& validity_point);

  /** Jump ahead in time, possibly leaving a gap. */
  virtual bool forceAdvance(const DataTimeSpec& t);

public:
  /// Returns the time at which the interval starts
  inline TimeTickType getValidityStart() const
  { return validity_start; }

  /// Returns the time at which the interval has ended
  inline TimeTickType getValidityEnd() const
  { return validity_end; }

  /// Returns the size of the interval
  inline TimeTickType getValiditySpan() const
  { return validity_end -  validity_start; }

  /// Compare one interval to another
  inline bool operator == (const TimeSpec& other) const
    {return validity_start == other.validity_start &&
     validity_end == other.validity_end;}

  /// Compare one interval to another, and return true when not equal
  inline bool operator != (const TimeSpec& other) const
    {return validity_start != other.validity_start ||
     validity_end != other.validity_end;}

  /// Move an interval up with a time delta
  TimeSpec operator+ (const int delta) const;

  /// Move an interval up with a time delta
  TimeSpec operator+ (const unsigned int delta) const;

  /// Move an interval up with a time in seconds
  TimeSpec operator+ (const double delta) const;

  /// Move an interval up with a time in seconds
  inline TimeSpec operator+ (const float delta) const
  { return *this + double(delta); }

  /// Move an interval up with a time in seconds
  TimeSpec operator- (const double delta) const;

  /// Move an interval up with a time in seconds
  inline TimeSpec operator- (const float delta) const
  { return *this - double(delta); }

  /// Move an interval back with a time delta
  TimeSpec operator- (const int delta) const;

  /// Move an interval back with a time delta
  TimeSpec operator- (const unsigned int delta) const;

  /// Get the difference between two timespec's
  int operator- (const TimeSpec& to) const;

  /// Copy from a data timespec
  TimeSpec& operator= (const DataTimeSpec& o);

  /// Get the value of the interval in seconds
  double getDtInSeconds() const;

  /** Find out how many microseconds elapsed since the formal start of
      this time. */
  int getUsecsElapsed() const;

  friend struct DataTimeSpec;
  friend class PeriodicTimeSpec;

  /// Write to stream, mainly for debugging purposes
  ostream& print (ostream& os) const;
};


/** A PeriodicTimeSpec is derived from the normal TimeSpec. It behaves
    differently, in that it tries to enforce the period specified at
    its creation in the "advance" updates. */
class PeriodicTimeSpec:
  public ScriptCreatable,
  public TimeSpec
{
  /// Normal span of the TimeSpec
  TimeTickType period;

public:
  /** Class name, for error messages. A white lie, actually PeriodicTimeSpec,
      but this timespec doubles for all in the script creation */
  static const char* classname;

  /** Copy constructor. */
  PeriodicTimeSpec(const PeriodicTimeSpec&);

  /** Constructor that creates a PeriodicTimeSpec from a TimeSpec. The
      interval between the end and start times is taken as the
      period. */
  PeriodicTimeSpec(const TimeSpec&);

  /** Constructor. Acts like a normal TimeSpec constructor, and makes
      a TimeSpec initially equal to [validity_start,
      validity_start+period). */
  PeriodicTimeSpec(TimeTickType validity_start = 0,
                   TimeTickType period = 1);

  /** Complete method, called after constructor and supply of
      parameters, has to check the validity of the parameters. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Obtain a pointer to the parameter table. */
  static const ParameterTable* getParameterTable();

  /// Destructor
  ~PeriodicTimeSpec();

  /// Clone method, re-implemented from TimeSpec
  TimeSpec* clone() const;

public:
  /** Advances the interval in the PeriodicTimeSpec with a value equal
      to the period specified in its creation, but only if the end
      time specified in the TimeSpec parameter is beyond the
      PeriodicTimeSpec's end time.

      if the method returns false, no advance was made, if the method
      returns true, and advance was made. This is central to the
      frequency step-up and step-down method used in DUECA. */
  bool advance(const TimeSpec& a);

  /** Advance with a DataTimeSpec */
  bool advance(const DataTimeSpec& a);

  /** Advance with an end time tick. */
  bool advance(const TimeTickType& validity_end = MAX_TIMETICK);

  /** "Greedy" variant of the advance method. Advances until the
      a.validity_end is no longer past the current time spec start

      @param a   Time span to cover.
  */
  bool greedyAdvance(const DataTimeSpec& a);

  /** Spool ahead fast (so forget all the intermediate intervals), to
      the time specified in the argument. The PeriodicTimeSpec will
      not forget its offset+period when doing this, so the actual time
      to which it spools may be rounded off downwards. */
  bool forceAdvance(const TimeTickType& validity_point);

  /** Spool ahead fast. */
  bool forceAdvance(const DataTimeSpec& t);

  /** Slide forward, can also change offset. */
  void slideAdvance(const TimeTickType& t);

  /** Read back the period. */
  inline TimeTickType getPeriod() const { return period; }

  /** Change the period. */
  inline void setPeriod(TimeTickType p) {period = p;}

  /// Write to stream, mainly for debugging purposes
  ostream& print (ostream& os) const;
public:
  /** Call macro to give this class connection to the script
      language. */
  SCM_FEATURES_DEF;

private:
  /** Set start time, to be called from the parameter table. */
  bool setStart(const int &i);

  /** Set period, to be called from the parameter table. */
  bool setPeriod(const int &i);
};

DUECA_NS_END

PRINT_NS_START
/** Print a time specification */
inline ostream& operator << (ostream& os, const DUECA_NS ::TimeSpec& t)
{ return t.print(os); }

/** Print a periodic time specification */
inline ostream& operator << (ostream& os, const DUECA_NS ::PeriodicTimeSpec& t)
{ return t.print(os); }
PRINT_NS_END

#include <DataTimeSpec.hxx>

#endif
