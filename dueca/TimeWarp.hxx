/* ------------------------------------------------------------------   */
/*      item            : TimeWarp.hxx
        made by         : Rene van Paassen
        date            : 030325
        category        : header file
        description     :
        changes         : 030325 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TimeWarp_hxx
#define TimeWarp_hxx

#ifdef TimeWarp_cxx
#endif

#include "Trigger.hxx"
#include "dueca_ns.h"

DUECA_NS_START

/** Time warp for a triggering device. Say that you want to calculate
    your turbulence/wind field or whatever property that does not rely
    on input ahead of time. You could trigger on the ticker with a
    time warp:

    \code
    // make a 'permanent' time warp (element of your class, or with
    // 'new', assuming that `span` defines the time span (integer ticks)
    // of your model updates:
    TimeWarp* tw = new TimeWarp(Ticker::single(), span);
    my_activity.setTrigger(*tw);
    \endcode

    Now at -- say -- step 100-101, your activity gets triggered for
    120-121, for a logical time that comes later (in the future) than
    the actual time of triggering. For negative warps the thing works
    the other way around.

    Be careful when doing this with channels. If you timewarp into the
    (logical) future, the channel will not yet be filled with data
    from the time span you are referring to, and if you warp too much
    into the past, a stream channel may have run out of data.

    Another use-case is in breaking a logical loop. Suppose you create
    a controller (autopilot) for your aircraft model. The controller
    uses the data from the input channel, let's call it `ac://u`, and
    the data from the output channel for the aircraft, `ac://y`. You
    need to combine the input at time `t`, with the output from one time
    step back, at `t - span`. Create a TimeWarp for the output channel:

    @code
    TimeWarp* tw = new TimeWarp(r_ac_y, span);
    my_activity.setTrigger(*tw && r_ac_u);
    @endcode

    Whenever the output channel is written for a time `t`, it will now
    trigger your autopilot for a time `t + span`. As soon as the control
    input for that time is also available, your autopilot will be
    triggered.

    Now there is a snag with this set-up. When the model is started at
    a time `t0`, there will be no output data from the aircraft model
    for time `(t0 - span, t0)`, which would be used by your timewarp
    to trigger the autopilot for `(t0, t0 + span)`. You can fix this by
    adding a manual TriggerPuller and running that one for
    `(t0, t0 + span)`. The manual puller is 'or-ed' with the timewarp.

    @code
    InitPuller* ipull = new ManualTriggerPuller("start-up puller");
    TimeWarp* tw = new TimeWarp(r_ac_y, span);
    my_activity.setTrigger((*tw || *ipull) && r_ac_u);
    @endcode

    When you start your module's activities, provide the activation for
    the first round. Note also that you won't find data in the `r_ac_y`
    channel for that time, so make sure you catch the exception that
    results, and perform appropriately.

    @code
    MyModule::startModule(const TimeSpec &time)
    {
      do_calc.switchOn(time);
      ipull->pull(DataTimeSpec(time.getValidityStart(),
                  time.getValidityStart() + span));
    }
    @endcode

    Another option would be to modify your aircraft simulation code, and
    provide (dummy) data on the output channel for the span just
    before the start time. It might be more efficient, but it also
    requires a code modification in a module that is not related to the
    autopilot module.

    Note that in the examples given above, the TimeWarp and
    ManualTriggerPuller were created with `new`. You can of course also
    create these as members of your module class.
*/

class TimeWarp : public TargetAndPuller
{
private:
  /** Offset in time. */
  int warp_time;

  /** Copying is not permitted. */
  TimeWarp(const TimeWarp &);

  /** Trigger the warp. It in turn triggers its target, with a warped time
      \param t   Triggering time
      \param idx Index of the puller. */
  void trigger(const DataTimeSpec &t, unsigned idx) override;

public:
  /** Constructor.
      \param base   Trigger pulling object whose triggering is to be
                    warped
      \param warp   Warp time, the time added to the triggering time
                    specification. Positive means it becomes
                    "logically" later, negative means it becomes
                    locigally earlier.
  */
  TimeWarp(TriggerPuller &base, int warp = 0);

  /** Destructor. */
  virtual ~TimeWarp();

  /** Change the warp time. */
  inline void warpTime(int w2) { warp_time = w2; }

  /** Targets are usually "living" objects. To find them, use
      getName() */
  virtual const std::string &getTargetName() const override;

private:
  /** adjust the name */
  void setTriggerName() override;
};

DUECA_NS_END
#endif
