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
    // 'new'
    TimeWarp* tw = new TimeWarp(Ticker::single(), 20);
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
*/


class TimeWarp:  public TargetAndPuller
{
private:
  /** Offset in time. */
  int warp_time;

  /** Copying is not permitted. */
  TimeWarp(const TimeWarp&);

  /** Trigger the warp. It in turn triggers its target, with a warped time
      \param t   Triggering time
      \param idx Index of the puller. */
  void trigger(const DataTimeSpec& t, unsigned idx) override;

public:
  /** Constructor.
      \param base   Trigger pulling object whose triggering is to be
                    warped
      \param warp   Warp time, the time added to the triggering time
                    specification. Positive means it becomes
                    "logically" later, negative means it becomes
                    locigally earlier.
  */
  TimeWarp(TriggerPuller& base, int warp = 0);

  /** Destructor. */
  virtual ~TimeWarp();

  /** Change the warp time. */
  inline void warpTime(int w2) {warp_time = w2;}

  /** Targets are usually "living" objects. To find them, use
      getName() */
  virtual const std::string& getTargetName() const override;

private:
  /** adjust the name */
  void setTriggerName() override;
};

DUECA_NS_END
#endif
