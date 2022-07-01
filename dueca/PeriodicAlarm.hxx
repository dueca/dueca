/* ------------------------------------------------------------------   */
/*      item            : PeriodicAlarm.hxx
        made by         : Rene van Paassen
        date            : 030612
        category        : header file
        description     :
        changes         : 030612 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef PeriodicAlarm_hxx
#define PeriodicAlarm_hxx

#ifdef PeriodicAlarm_cxx
#endif

#include <dueca_ns.h>
#include <Trigger.hxx>
#include <TimeSpec.hxx>
#include <AsyncList.hxx>

DUECA_NS_START

/** This is a triggering device that can provide your activity with
    user-controlled, periodic triggering. Create a periodic
    alarm, e.g. as one  of the objects in your module class.

    Triggering will take place at the intervals specified for the
    alarm. Trigger period can be changed. */
class PeriodicAlarm: public TargetAndPuller
{
  /** Periodic time spec giving rate. */
  PeriodicTimeSpec pspec;

  /** Change of period or offset */
  struct ModifyAlarm {
    /** New offset of the modified alarm. */
    TimeTickType offset;
    /** New period of the modified alarm. */
    TimeTickType period;

    /** Constructor. */
    ModifyAlarm(TimeTickType offset = 0, TimeTickType period = 1);
  };

  /** Changes to the rate. */
  AsyncList<ModifyAlarm> new_period;

  /** do not allow copying */
  PeriodicAlarm(const PeriodicAlarm& o);

  /** nor assignment */
  PeriodicAlarm& operator = (const PeriodicAlarm& o);

  /** Trigger. The triggering action is passed on to the targets when
      the alarm has to go off, otherwise this does nothing. */
  void trigger(const DataTimeSpec& t, unsigned idx);

public:
  /** Constructor.
      \param ts    Time specification for the alarm. */
  PeriodicAlarm(const TimeSpec& ts);

  /** Constructor without parameters. This produces an alarm that
      triggers at the time steps of the default clock. See your
      dueca.cnf file for that value. */
  PeriodicAlarm();

  /** Destructor. */
  ~PeriodicAlarm();

  /** Set the requested period. The current period is maintained for
      one more activation, after this the new period is used. */
  void changePeriod(TimeTickType interval);

  /** Set the requested period and an offset from a time
      specification. The current period is maintained for
      one more activation, after this the new period is used. */
  void changePeriodAndOffset(const TimeSpec& ts);

  /** get the name of the target */
  const std::string& getTargetName() const;
};

DUECA_NS_END

#endif
