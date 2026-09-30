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
#include <algorithm>
namespace dueca {

/** Implementation of a scaling/converting device, using a polynomial
    function. This class is a functor, with a double as input and
    another double as output. The application for this in DUECA is as
    an input calibrator for IO signals, see also the InputCalibrator
    and OutputCalibrator documentation. */

class PolynomialN : public SimpleFunction
{
  /** Order of the polynomial. */
  unsigned int n;

  /** Array of coefficients. */
  double *a;

public:
  /** datatype for the output and calculation */
  typedef double data_type;

  /** Empty */
  PolynomialN();

  /** Constructor.
      \param  n  Order of the polynomial
      \param  ai Array with coefficients, polynomial is
                 \f$a_0 + a_1 x + \ldots + a_n x^n \f$ */
  PolynomialN(unsigned int n, const double ai[]);

  /** Constructor from array */
  template <typename A>
  PolynomialN(const A &ai) :
    n(ai.size() - 1),
    a(new double[n + 1])
  {
    std::copy(ai.begin(), ai.end(), a);
  }

  /** Copy constructor. */
  PolynomialN(const PolynomialN &o);

  /** assignment */
  PolynomialN &operator=(const PolynomialN &o);

  /** Destructor. */
  ~PolynomialN();

  /** The operator. */
  double operator()(const double x) const;

  /** Print the object */
  std::ostream &print(std::ostream &os) const;

  /** Adjust the offset */
  void adjustOffset(const double off);
};

} // namespace dueca

inline std::ostream &operator<<(std::ostream &os,
                                const dueca::PolynomialN &o)
{
  return o.print(os);
}

#endif
