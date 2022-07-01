/* ------------------------------------------------------------------   */
/*      item            : Circular.hxx
        made by         : Rene van Paassen
        date            : 020429
        category        : header file
        description     :
        changes         : 020429 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Circular_hxx
#define Circular_hxx

#include "SimpleFunction.hxx"
#include <dueca_ns.h>
#include <limits>
#include <iostream>

DUECA_NS_START


/** Implementation of a rotary scaling/converting device, using a
    simple gain. This class is a functor, with a double as input and
    another double as output. The application for this in DUECA is as
    an input calibrator for IO signals, see also the InputCalibrator
    and OutputCalibrator documentation.

    Implements function
    \f[
    y(x) = R * ( \frac{K}{x-x_0} ) + n * R
    \f]

    Here the integer \f$n\f$ is adjusted so that the function's output
    (reach) runs from \f$y_{start}\f$ to \f$y_{start}+R\f$

    How to calibrate:

    * Determine the input value for which your parameter is zero, set
      \f$ x_0 \f$ to this value

    * Determine the range of input values and set \f$ K \f$ to its inverse,
      e.g., if the range is \f$ 2^{12} \f$, then
      \f$ K = \frac{1}{2^{12}} \f$

    * Determine the output range, e.g., 360 deg, set \f$ R \f$ to this
      value.

    * Determine where you want to "start" the output, e.g.,
      \f$ y_{start}  = -180 \f$ means you will get outputs from -180
      to 180, with \f$ y_{start}  = 0 \f$ the output will be 0 to
      360.

*/
class Circular: public SimpleFunction
{
  /** Array of coefficients. */
  double K;

  /** Integer value of zero reading */
  double izero;

  /** Range to scale up to, full rotation */
  double range;

  /** Start of range */
  double norm_start;

public:
  /** Constructor.
      @param  K       Gain coefficient, 1.0/(increments in full rotation)
      @param  xzero   Input value when angle is zero (normally integer)
      @param  range   Range that the value is to be scaled to, e.g. 360
                      degrees or \f$2\pi\f$
      @param  ystart  Start value of the output range, for exaple 0, -180, etc.
  */
  Circular(double K, double xzero, double range, double ystart);

  /** Destructor. */
  ~Circular();

  /** The operator. */
  double operator () (const double x) const;

  /** Print the object */
  std::ostream& print (std::ostream& os) const;
};

DUECA_NS_END

/** Print operator for Circular */
inline std::ostream& operator << (std::ostream& os,
                                  const DUECA_NS::Circular& o)
{ return o.print(os); }

#endif

