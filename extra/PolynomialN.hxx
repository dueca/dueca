/* ------------------------------------------------------------------   */
/*      item            : PolynomialN.hxx
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

#ifndef PolynomialN_hxx
#define PolynomialN_hxx

#ifdef PolynomialN_cxx
#endif

#include "SimpleFunction.hxx"
#include <dueca_ns.h>
#include <iostream>
DUECA_NS_START


/** Implementation of a scaling/converting device, using a polynomial
    function. This class is a functor, with a double as input and
    another double as output. The application for this in DUECA is as
    an input calibrator for IO signals, see also the InputCalibrator
    and OutputCalibrator documentation. */
class PolynomialN: public SimpleFunction
{
  /** Order of the polynomial. */
  unsigned int n;

  /** Array of coefficients. */
  double *a;

public:
  /** Empty */
  PolynomialN();

  /** Constructor.
      \param  n  Order of the polynomial
      \param  ai Array with coefficients, polynomial is
                 \f$a_0 + a_1 x + \ldots + a_n x^n \f$ */
  PolynomialN(unsigned int n, const double ai[]);

  /** Copy constructor. */
  PolynomialN(const PolynomialN& o);

  /** assignment */
  PolynomialN& operator = (const PolynomialN& o);

  /** Destructor. */
  ~PolynomialN();

  /** The operator. */
  double operator () (const double x) const;

  /** Print the object */
  std::ostream& print (std::ostream& os) const;
};

DUECA_NS_END

inline std::ostream& operator << (std::ostream& os,
                                  const DUECA_NS::PolynomialN& o)
{ return o.print(os); }

#endif

