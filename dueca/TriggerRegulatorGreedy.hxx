/* ------------------------------------------------------------------   */
/*      item            : TriggerRegulatorGreedy.hxx
        made by         : Rene van Paassen
        date            : 160701
        category        : header file
        description     : make triggering regular
        changes         : 160701 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TriggerRegulatorGreedy_hxx
#define TriggerRegulatorGreedy_hxx

#include "Trigger.hxx"

DUECA_NS_START;

/** Make triggering (more) regular.

    A TriggerRegulatorGreedy accepts a trigger and a time
    specification.  Using the time specification, the triggering will
    be made regular. This only works with incoming spans. If a span
    starts (after a gap in data on a channel for example), one shorter
    trigger may be passed, and after this regular triggers are
    supplied again.

    Example:
    \code
    myActivity.setTrigger(
      boost::intrusive_ptr<TriggerRegulatorGreedy>(
      new TriggerRegulatorGreedy(my_token, TimeSpec(0,20))));
    \endcode

    If now a triggering is provided by my_token for (150,200),
    (200,250), the regulator will deliver triggers (150,160),
    (160,180), (180,200) etc. If there is a gap in triggering, a
    last remaining bit of span may not be triggered.

    This is the greedy variant, i.e., it will not wait until the
    whole incoming span is present before providing another trigger,
    as soon as part of the span is present, it triggers.
    dueca::TriggerRegulator will do non-greedy regular triggering.


 */
class TriggerRegulatorGreedy: public TargetAndPuller
{
private:
  /** Regularisation of the trigger intervals */
  PeriodicTimeSpec ts;

  /** Remember the incoming triggering pace */
  TimeTickType            prev_end;

  /** Change in period/time requested */
  DataTimeSpec   * volatile new_ts;

  /** Copying is not permitted. */
  TriggerRegulatorGreedy(const TriggerRegulatorGreedy&);

  /** Trigger the regulator. It in turn triggers its target, with
      a regularised time
      \param t   Triggering time
      \param idx Index of the puller. */
  void trigger(const DataTimeSpec& t, unsigned idx);

public:
  /** Constructor.

      @param base     The base triggering object
      @param ts       Time offset and period for triggering
  */
  TriggerRegulatorGreedy(TriggerPuller& base, const TimeSpec& ts =
                         TimeSpec(0,1));

  /** Constructor with a ref-counter object as trigger

      @param base     The base triggering object
      @param ts       Time offset and period for triggering
  */
  TriggerRegulatorGreedy(boost::intrusive_ptr<TargetAndPuller> base,
                         const TimeSpec& ts = TimeSpec(0,1));

  /** Constructor without triggering base, use
      TriggerTarget::setTrigger to specify the trigger.

      @param ts       Time offset and period for triggering */
  TriggerRegulatorGreedy(const TimeSpec& ts = TimeSpec(0,1));

  /** Destructor */
  ~TriggerRegulatorGreedy();

  /** Change the period and offset.

      Prepare a new period and offset. Through triggering, the so
      prepared object should be "picked up". Returns false if not
      possible.

      @param ts    Time specification defining new period and offset.
      @returns     true if the change is accepted.
  */
  bool changePeriodAndOffset(const TimeSpec& ts);

  /** Set the period and offset. Note that this is not thread-safe,
      and should only be done before running.

      @param ts    Time specification defining new period and offset.
  */
  void setPeriodAndOffset(const TimeSpec& ts) {this->ts = ts;}

  /** Name of the regulator */
  const std::string& getTargetName() const;

private:
  /** Update the trigger name, initially or after changes */
  void setTriggerName();
};

DUECA_NS_END;

#endif
