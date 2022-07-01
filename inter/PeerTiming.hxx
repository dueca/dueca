/* ------------------------------------------------------------------   */
/*      item            : PeerTiming.hxx
        made by         : Rene van Paassen
        date            : 200531
        category        : header file
        description     :
        changes         : 200531 first version
        language        : C++
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef PeerTiming_hxx
#define PeerTiming_hxx

#include <dueca/TimeSpec.hxx>
#include "ReplicatorNamespace.hxx"
#include <list>

DUECA_NS_START;
struct DataTimeSpec;
DUECA_NS_END;

STARTNSREPLICATOR;

/** Maintain per-peer timing differences */
class PeerTiming
{
  /** Filtered timing difference */
  double                         delta_time;

  /** Gain for filtering time */
  double                         time_gain;

  /** More-or-less fixed correction */
  TimeTickType                   theirtime;

  /** Size of adjustment, typically once the communication timing update */
  TimeTickType                   jumpsize;

  /** Skip history */
  struct AdjustmentHistory {
    /** Point at which a transition correction was determined. */
    TimeTickType                 theirtime;

    /** Transition correction */
    TimeTickType                 transition;

    /** Constructor */
    AdjustmentHistory(TimeTickType theirtime, TimeTickType transition);
  };

  /** List of adjustments */
  std::list<AdjustmentHistory>   adjustment;

public:
  /** Constructor */
  PeerTiming(TimeTickType jumpsize=1U, double time_gain=0.002);

  /** Destructor */
  ~PeerTiming();

  /** Filter or set the timing delta. */
  void adjustDelta(TimeTickType mytime, TimeTickType theirtime,
                   bool runclock, int offset_usecs=0);

  /** Translate a remote time tick into a local one. */
  bool translate(DataTimeSpec& ts) const;
};

ENDNSREPLICATOR;

#endif
