/* ------------------------------------------------------------------   */
/*      item            : PeerTiming.cxx
        made by         : Rene' van Paassen
        date            : 200531
        category        : body file
        description     :
        changes         : 200531 first version
        language        : C++
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define PeerTiming_cxx
#include "PeerTiming.hxx"
#include <cmath>
#include <dueca/Ticker.hxx>
// #define I_INT
#include <debug.h>
#define DEBPRINTLEVEL 0
#include <debprint.h>
#include <dueca/DataTimeSpec.hxx>

STARTNSREPLICATOR;

typedef TimeTickType T;

PeerTiming::AdjustmentHistory::AdjustmentHistory(TimeTickType theirtime,
                                                 TimeTickType transition) :
  theirtime(theirtime),
  transition(transition)
{ }


PeerTiming::PeerTiming(TimeTickType jumpsize, double time_gain) :
  delta_time(std::nan("")),
  time_gain(time_gain),
  jumpsize(jumpsize)
{ }


PeerTiming::~PeerTiming()
{ }

static inline double timediff(TimeTickType mytime, TimeTickType theirtime)
{
  if (mytime >= theirtime) { return double(mytime - theirtime); }
  return -1.0*double(theirtime - mytime);
}

void PeerTiming::adjustDelta(TimeTickType mytime, TimeTickType theirtime,
                             bool runclock, int offset_usecs)
{
  if (std::isnan(delta_time)) {

    // new transition time
    T transition = 0;

    // and set the transition value
    if (mytime >= theirtime) {
      T trans_bare = mytime - theirtime;
      T delta01_plushalf = (trans_bare + T(jumpsize >> 1)) % jumpsize;
      transition = trans_bare + T(jumpsize >> 1) - delta01_plushalf;
    }
    else {
      T trans_bare = theirtime - mytime;
      T delta10_plushalf = (trans_bare + T(jumpsize >> 1)) % jumpsize;
      transition = 0U -
        (trans_bare + T(jumpsize >> 1) - delta10_plushalf);
    }

    // the initial adjustment goes as far back in time as possible, to make
    // sure that any writing at this end -- of old, backlogged/badly timed
    // data, does not end up as far in the future through adjustment.
    if (mytime >= theirtime) {
      // other node is behind, transition is positive, can map from 0
      adjustment.emplace_front(0U, transition);
    }
    else {
      // other node is ahead of us. Can only transition from common
      // start point
      adjustment.emplace_front(theirtime - mytime, transition);
    }

    // first call, take time as given
    delta_time = timediff(mytime, theirtime + transition);

    /* DUECA interconnect.

       Information on the timing differences between this node and a
       peer node at initial connection. */
    I_INT("PeerTiming first delta=" << delta_time << " jump=" << jumpsize <<
          " transition=" << transition << " transition start=" <<
          adjustment.front().theirtime);
    return;
  }

  // calculate the new delta
  delta_time += time_gain *
    (timediff(mytime, theirtime + adjustment.front().transition) - delta_time);

  // effects. Either control clock, or calculate new transitions
  if (runclock) {
    Ticker::single()->dataFromMaster
      (theirtime + adjustment.front().transition, offset_usecs);
  }
  else {
    if (delta_time > 0.75*jumpsize) {
      adjustment.emplace_front(theirtime + jumpsize,
                               adjustment.front().transition + jumpsize);
      delta_time -= jumpsize;
#if DEB_ACTIVE
      DEB("PeerTiming +jump, new delta=" << delta_time);
#else
      /* DUECA interconnect.

         The timing difference between this node and a peer has been
         increasing so much that a jump in translating data time is
         needed. The present node jumps forward. */
      D_INT("PeerTiming +jump, new delta=" << delta_time);
#endif
    }
    else if (delta_time < -0.75*jumpsize) {
      adjustment.emplace_front(theirtime + jumpsize,
                               adjustment.front().transition - jumpsize);
      delta_time += jumpsize;
#if DEB_ACTIVE
      DEB("PeerTiming -jump, new delta=" << delta_time);
#else
      /* DUECA interconnect.

         The timing difference between this node and a peer has been
         increasing so much that a jump in translating data time is
         needed. The present node jumps backwards. */
      D_INT("PeerTiming -jump, new delta=" << delta_time);
#endif
    }
  }
}

bool PeerTiming::translate(DataTimeSpec& theirtime) const
{
  T t0 = MAX_TIMETICK;
  T t1 = 0U;
  if (theirtime.getValiditySpan()) {
    for (const auto &t: adjustment) {
      if (theirtime.getValidityEnd() >= t.theirtime) {
        t1 = theirtime.getValidityEnd() + t.transition;
        break;
      }
    }
    for (const auto &t: adjustment) {
      if (theirtime.getValidityStart() > t.theirtime) {
        t0 = theirtime.getValidityStart() + t.transition;
        break;
      }
    }
    if (t1 > t0) {
      theirtime = DataTimeSpec(t0, t1);
      return true;
    }
  }
  else {
    for (const auto &t: adjustment) {
      if (theirtime.getValidityStart() > t.theirtime) {
        t0 = theirtime.getValidityEnd() + t.transition;
        theirtime = DataTimeSpec(t0, t0);
        return true;
      }
    }
  }
  DEB("Timing translate failed, " << theirtime <<
      " to (" << t0 << "," << t1 << ")");
  return false;
}


ENDNSREPLICATOR;
