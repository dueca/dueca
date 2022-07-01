/* ------------------------------------------------------------------   */
/*      item            : Inverse.hxx
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

#ifndef Inverse_hxx
#define Inverse_hxx


#include "SimpleFunction.hxx"
#include <dueca_ns.h>
#include <limits>
#include <iostream>

DUECA_NS_START


/** Implementation of a scaling/converting device, using a polynomial
    function. This class is a functor, with a double as input and
    another double as output. The application for this in DUECA is as
    an input calibrator for IO signals, see also the InputCalibrator
    and OutputCalibrator documentation.

    Implements function
    \f[
    y(x) = \frac{K}{x-x0}
    \f]
*/
class Inverse: public SimpleFunction
{
  /** Array of coefficients. */
  double K;

  /** X0 */
  double x0;

  /** limits */
  double epsx;

public:
  /** Constructor.
      \param  K  Gain factor
      \param  x0 asymptote location
      \param  epsx Smallest x (closest to x0) expected, needed to protect for
                 division by zero */
  Inverse(double K, double x0=0,
          double epsx=std::numeric_limits<double>::epsilon());

  /** Destructor. */
  ~Inverse();

  /** The operator. */
  double operator () (const double x) const;

  /** Print the object */
  std::ostream& print (std::ostream& os) const;
};

DUECA_NS_END

/** Print operator for Inverse */
inline std::ostream& operator << (std::ostream& os,
                                  const DUECA_NS::Inverse& o)
{ return o.print(os); }

#endif

