/* ------------------------------------------------------------------   */
/*      item            : TransportDelayEstimator.hh
        made by         : Rene' van Paassen
        date            : 990819
        category        : header file
        description     :
        changes         : 990819 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TransportDelayEstimator_hh
#define TransportDelayEstimator_hh

#ifdef TransportDelayEstimator_cc
#endif

#include "ScriptCreatable.hxx"
#include <dueca_ns.h>
#include <iostream>

DUECA_NS_START
struct ParameterTable;

/** Class to determine transport delays in messages sent over a
    network. It uses a non-linear Kalman filter, and tries to estimate
    time per byte and the set-up time. */
class TransportDelayEstimator :
  public ScriptCreatable
{
  /** state variable. */
  double x[2];
  /** Variance and variance prediction. \{ */
  double P[4], Pp[4];  /// \}
  /** Kalman filter gain. \{ */
  double K[2], H[2];   /// \}

  /** State and observation noises. \{ */
  double R, innov_max; /// \}
  /** Noise matrix. */
  double Q[4];

  /** \name Excitation check
      Variables used to determine whether there is significant
      excitation (different transmit lengths) in the data. */
  //@{
  /** Number of time the input looks (almost) the same. */
  int same_count;

  /** Moving average, to compare the input to. */
  double moving_avg;
  //@}

  /** initial value for per-packet delay */
  double alpha0;

  /** initial value for per-byte delay. */
  double beta0;

  /** Standard deviation observation noise. */
  double s_v;

  /** Standard deviation per-packet delay */
  double s_alpha;

  /** Standard deviation per-byte delay. */
  double s_beta;

public:
  /** Creates an estimator. The model assumes the delay is a constant
      factor, plus a delay per byte sent.
      innov_max is the maximum innovation accepted. Suggest 50 usec. */
  TransportDelayEstimator();

  /** Destructor. */
  virtual ~TransportDelayEstimator();

  /** Complete method. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Update the estimate with the knowledge that so many bytes took
      usecs microseconds. */
  bool update(int bytes, int usecs);

  /** Returns an estimate for the sending time of some bytes. */
  const int operator () (int bytes);

  // scheme connectivity
  SCM_FEATURES_DEF;

  /** Return table with tunable parameters. */
  static const ParameterTable* getParameterTable();

  /** Print, for debugging. */
  friend std::ostream& operator << (std::ostream& os,
                                    const TransportDelayEstimator& a);
};

DUECA_NS_END
#endif



