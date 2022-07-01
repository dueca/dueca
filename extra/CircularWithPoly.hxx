/* ------------------------------------------------------------------   */
/*      item            : CircularWithPoly.hxx
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

#ifndef CircularWithPoly_hxx
#define CircularWithPoly_hxx

#include "SimpleFunction.hxx"
#ifdef UNITTEST
#define DUECA_NS_START
#define DUECA_NS_END
#define DUECA_NS
#else
#include <dueca_ns.h>
#endif
#include <limits>
#include <iostream>

DUECA_NS_START


/** Implementation of a rotary scaling/converting device, using a
    polynomial for final correction and optionally tracking of the
    angle. This class is a functor, with a double as input and another
    double as output. The application for this in DUECA is as an input
    calibrator for IO signals, see also the InputCalibrator and
    OutputCalibrator documentation.

    The input is first scaled with a gain K, and then normalized to a
    range \f$n_0\f$ .. \f$n_0+1\f$. The polynomial is then applied
    to this normalized result.

    Implements in step 1:
    \f[
    x_t(x) = (K\left(x-x_0\right) - n_0) \mod 1 + n_0
    \f]

    And with that result (ranging from
    \f[
    y(x_t) = a_0 + a_1 x_t \ldots + a_n (x_t)^n
    \f]

    As an example, consider a 10 bit synchro measuring angle of
    attack, with output values from 0 to 1023. At 0 degrees angle of
    attack, the output from the measurement is 980, and so with
    different inputs the measurement is likely to flip around from
    1023 to 0. Determine \f$x_0=980\f$, \f$K=1/1024\f$ and
    \f$n_0=-0.5\f$, then the output from the first step ranges from -0.5
    to 0.5 for a -180 to 180 degree (hypothetical) input. With the
    polynomial from the second step the scaling may be further
    defined.
*/
class CircularWithPoly: public SimpleFunction
{
  /** Initial scaling gain \f$K\f$. */
  double K;

  /** Integer value of zero reading \f$x_0\f$ */
  double xzero;

  /** Start of range \f$n_0\f$ */
  double norm_start;

  /** Order of the polynomial. */
  const size_t n;

  /** Array of coefficients. */
  double *a;

public:
  /** Constructor.
      @param  K       Gain coefficient, 1.0/(increments in full rotation)
      @param  xzero   Input value when angle is zero
      @param  norm_start   Normalised start value of first step, for example
                      -0.5, to get a -0.5 to 0.5 output, or zero to get a
                      0 to 1 output
      @param  n       Order of the calibrating polynomial
      @param  ai      Array with polynomial coefficients, polynomial is
                      \f$a_0 + a_1 x + \ldots + a_n x^n \f$
  */
  CircularWithPoly(double K, double xzero, double norm_start,
                   size_t n, const double ai[]);

  /** Copy constructor */
  CircularWithPoly(const CircularWithPoly& o);

  /** assignment */
  CircularWithPoly& operator = (const CircularWithPoly& o);

  /** Destructor. */
  ~CircularWithPoly();

  /** The operator. */
  double operator () (const double x) const;

  /** Print the object */
  std::ostream& print (std::ostream& os) const;
};

DUECA_NS_END

/** Print operator for CircularWithPoly */
inline std::ostream& operator << (std::ostream& os,
                                  const DUECA_NS::CircularWithPoly& o)
{ return o.print(os); }

#endif

