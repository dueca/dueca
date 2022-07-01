/* ------------------------------------------------------------------   */
/*      item            : TransportDelayEstimator.cxx
        made by         : Rene' van Paassen
        date            : 990819
        category        : body file
        description     :
        changes         : 990819 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TransportDelayEstimator_cc
#include "TransportDelayEstimator.hxx"
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "VarProbe.hxx"
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

const ParameterTable* TransportDelayEstimator::getParameterTable()
{
  static const ParameterTable table[] = {
    { "const-delay", new VarProbe<TransportDelayEstimator,double>
      (REF_MEMBER(&TransportDelayEstimator::alpha0)),
      "constant/offset delay, for setting up the message + overhead [us]" },
    { "delay-per-byte", new VarProbe<TransportDelayEstimator,double>
      (REF_MEMBER(&TransportDelayEstimator::beta0)),
      "Amount of additional delay per byte, [us]" },
    { "s-v", new VarProbe<TransportDelayEstimator,double>
      (REF_MEMBER(&TransportDelayEstimator::s_v)),
      "observation noise, standard deviation in [us]" },
    { "s-const-delay", new VarProbe<TransportDelayEstimator,double>
      (REF_MEMBER(&TransportDelayEstimator::s_alpha)),
      "Process noise standard deviation constant delay [us]" },
    { "s-delay-per-byte", new VarProbe<TransportDelayEstimator,double>
      (REF_MEMBER(&TransportDelayEstimator::s_beta)),
      "process noise standard deviation per-byte delay [us]" },
    { "innov-max", new VarProbe<TransportDelayEstimator,double>
      (REF_MEMBER(&TransportDelayEstimator::innov_max)),
      "maximum on innovation step [us]" },
    { NULL, NULL,
      "Helper class for media accessors that need to estimate a round"
      "trip time for the data transport." }
  };
  return table;
}

const int accept_same = 20;

TransportDelayEstimator::TransportDelayEstimator() :
  same_count(accept_same),
  moving_avg(0.0)
{
  //
}

bool TransportDelayEstimator::complete()
{
  //(double alpha0, double beta0,
  //double stdR, double s_alpha, double s_beta, double innov_max) :

  R = s_v;
  x[0] = alpha0;
  x[1] = beta0;

  P[0] = 1.0e10;
  P[1] = 0.0;
  P[2] = 0.0;
  P[3] = 1.0e10;
  Q[0] = s_alpha*s_alpha;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = s_beta*s_beta;

  H[0] = 2.0;

  return true;
}

const char* TransportDelayEstimator::getTypeName()
{
  return "TransportDelayEstimator";
}

TransportDelayEstimator::~TransportDelayEstimator()
{
  //
}

inline static void ADD2x2(double* res, const double* a, const double* b)
{
  res[0] = a[0] + b[0];
  res[1] = a[1] + b[1];
  res[2] = a[2] + b[2];
  res[3] = a[3] + b[3];
}

inline static double MXPrePost(const double* p, const double* c)
{
  return p[0]*(p[0]*c[0] + p[1]*c[1]) + p[1]*(p[0]*c[2] + p[1]*c[3]);
}

inline static void MX2x2x1div(double* res, const double* a, const double *b,
                  double k)
{
  res[0] = (a[0]*b[0] + a[1]*b[1])/k;
  res[1] = (a[2]*b[0] + a[3]*b[1])/k;
}

inline static void MUL2x2x2(double* res, const double* a, const double* b)
{
  res[0] = a[0]*b[0]+a[1]*b[2];
  res[1] = a[0]*b[1]+a[1]*b[3];
  res[2] = a[2]*b[0]+a[3]*b[2];
  res[3] = a[2]*b[1]+a[3]*b[3];
}

inline static void MXIm2x1x2X2x2(double* res, const double* a,
                          const double* b, const double* c)
{
  double tmp[4];
  tmp[0] = 1.0 - a[0]*b[0];
  tmp[1] =     - a[0]*b[1];
  tmp[2] =     - a[1]*b[0];
  tmp[3] = 1.0 - a[1]*b[1];
  MUL2x2x2(res, tmp, c);
}

inline static double limit(double a, double mx)
{
  if (a > mx) return mx;
  if (a < -mx) return -mx;
  return a;
}

bool TransportDelayEstimator::update(int bytes, int usecs)
{
  DEB("Measured transport time for " << bytes  << " bytes:" << usecs);

  // calculate a moving average of bytes
  moving_avg = 0.9 * moving_avg + 0.1 * bytes;

  // check whether there is enough excitement in the data. If not
  // (e.g. 1000 times the same byte count) the parameters may run
  // away, so don't use that data
  double relchange = bytes/moving_avg;
  if (relchange > 0.95  || relchange < 1.05) same_count = accept_same;
  if (--same_count < 0) return false;

  // Kalman filter calculations. Note that this is a little
  // shortened, due to the fact that the model is super simple

  // step 1: state variance predictor:
  // Assumes 1-P is known (constructor), Q is known (constructor)
  ADD2x2(Pp, P, Q);

  // step 2: Kalman filter gain
  // Assumes Pp calculated (step 1), 2-H is known. H is special, and
  // varies with the number of bytes transmitted, H[0] from constructor
  H[1] = double(bytes);
  double to_invert = R + MXPrePost(H, Pp);
  MX2x2x1div(K,P,H,to_invert);

  // step 3: new state vector
  double innov = limit(usecs - H[0]*x[0] - H[1]*x[1], innov_max);

  DEB("K[0]=" << K[0] << " K[1]=" << K[1] << " pred="
      << H[0]*x[0] - H[1]*x[1] << " meas=" << usecs
      << " innov=" << innov);

  // changed this to minus RvP
  x[0] += K[0]*innov;
  x[1] += K[1]*innov;

  // step 4, new covariance matrix
  MXIm2x1x2X2x2(P,K,H,Pp);

#if DEBUGLEVEL > 1
  static int countdown = 1000;
  if (countdown-- == 0) {
    DEB1("new time model parameters alpha=" << x[0] << " beta=" << x[1]);
    countdown = 1000;
  }
#endif
  DEB("new time model parameters alpha=" << x[0] << " beta=" << x[1]);

  return true;
}

const int TransportDelayEstimator::operator () (int bytes)
{
  int estimate = int(H[0]*x[0] + bytes*x[1] + 0.5);
  DEB("Estimated transport time for " << bytes  << " bytes:"
         << int(H[0]*x[0] + bytes*x[1] + 0.5));
#if DEBUGLEVEL > 1
  if (estimate < 0 || estimate > 100000) {
    DEB1("Delay estimate for transport of " << bytes <<
         " bytes is " << estimate);
  }
#endif
  return estimate;
}


DUECA_NS_END
