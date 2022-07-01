/* ------------------------------------------------------------------   */
/*      item            : AperiodicAlarm.hxx
        made by         : Rene van Paassen
        date            : 030612
        category        : header file
        description     :
        changes         : 030612 first version
                          040206 additional comments, thanks Olaf
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef AperiodicAlarm_hxx
#define AperiodicAlarm_hxx

#include <dueca_ns.h>
#include <Trigger.hxx>
#include <AsyncList.hxx>

DUECA_NS_START

/** This is a triggering device that can provide your activity with
    user-controlled, a-periodic triggering. Create an a-periodic
    alarm, e.g. as one  of the objects in your module class, and
    request to be triggered at some time.

    Triggering will take place at the clock time specified for the
    alarm, or with the next available clock tick if that time has
    already passed. */
class AperiodicAlarm: public TargetAndPuller
{
  /** a list with the alarm ticks set. */
  AsyncList<TimeTickType> ticks;

  /** newest tick */
  TimeTickType newest_tick;

  /** do not allow copying */
  AperiodicAlarm(const AperiodicAlarm& o);

  /** nor assignment */
  AperiodicAlarm& operator = (const AperiodicAlarm& o);

  /** Trigger. The triggering action is passed on to the targets when
      the alarm has to go off, otherwise this does nothing. */
  void trigger(const DataTimeSpec& t, unsigned idx);

public:
  /** Constructor. */
  AperiodicAlarm(const std::string& name = std::string("AperiodicAlarm()"));

  /** Destructor. */
  ~AperiodicAlarm();

  /** And a target name too */
  const std::string& getTargetName() const;

  /** Request a tick from the alarm. This will fail and return false
      if the ticks are not in chronological order!.
      \param time for which the alarm should "sound".
      \returns True if the alarm could be implemented, false if
      not. The only reason for failure of implementation of an alarm
      is that the time requested is smaller (earlier) than a time
      requested earlier. In other words, alarms only stack up at the
      end. */
  bool requestAlarm(TimeTickType time);

  /** Request the earliest next alarm. The AperiodicAlarm class keeps
      a record of the triggering time, which is updated with the time
      you feed to the requestAlarm(TimeTickType time) call, or, when
      you use this call, it is incremented by one.

      If you only use this alarm, you get triggers at 0, 1, 2
      etc. However, an activity only runs when it is switched on. If
      you switch on your activity later, it usually gets a switch-on
      time in the thousands, and all these initial triggers are
      lost. To prevent this, in this case switch on your activity from
      the "beginning of time",
      \code
      my_activity.switchOn(0);
      \endcode
  */
  void requestAlarm();
};

DUECA_NS_END

#endif
