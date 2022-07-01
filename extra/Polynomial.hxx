/* ------------------------------------------------------------------   */
/*      item            : Polynomial.hxx
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

#ifndef Polynomial_hxx
#define Polynomial_hxx

#ifdef Polynomial_cxx
#endif

#include <dueca_ns.h>
#include <iostream>
DUECA_NS_START


/** Implementation of a scaling/converting device, using a polynomial
    function. This class is a functor, with a double as input and
    another double as output. The application for this in DUECA is as
    an input calibrator for IO signals, see also the InputCalibrator
    and OutputCalibrator documentation. */
template<int n>
class Polynomial
{
  /** Array of coefficients. */
  double a[n+1];

public:
  /** Constructor.
      \param ai    Array with coefficient, polynomial is
                   \f$a_0 + a_1 x + a_2 x^2 + \ldots\f$ */
  Polynomial(const double ai[n+1]);

  /** Destructor. */
  ~Polynomial();

  /** The operator. */
  double operator () (const double x) const;

  /** Print the value */
  std::ostream& print (std::ostream& os) const;
};

DUECA_NS_END

template<int n>
inline std::ostream& operator << (std::ostream& os,
                                  const DUECA_NS::Polynomial<n>& o)
{ return o.print(os); }

#endif

#if defined(Polynomial_cxx) || defined(DO_INSTANTIATE)

DUECA_NS_START

template<int n>
Polynomial<n>::Polynomial(const double ai[n+1])
{
  for (int ii = n+1; ii--; ) a[ii] = ai[ii];
}

template<int n>
Polynomial<n>::~Polynomial()
{
  //
}

template<int n>
double Polynomial<n>::operator () (const double x) const
{
  double r = a[n];
  for (int ii = n; ii--; )
    r = a[ii] + x * r;
  return r;
}

template<int n>
std::ostream& Polynomial<n>::print(std::ostream& os) const
{
  os << "Polynomial<" << n << ">(";
  for (int ii = 0; ii < n; ii++) {
    if (ii > 0) os << ", ";
    os << "a_" << ii << "=" << a[ii];
  }
  return os << ")";
}

DUECA_NS_END

#endif
