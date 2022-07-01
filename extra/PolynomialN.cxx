/* ------------------------------------------------------------------   */
/*      item            : Polynomial.cxx
        made by         : Rene' van Paassen
        date            : 020429
        category        : body file
        description     :
        changes         : 020429 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DO_INSTANTIATE
#define PolynomialN_cxx
#endif

#include "PolynomialN.hxx"

DUECA_NS_START
PolynomialN::PolynomialN() :
  n(0),
  a(new double[1])
{
  a[0] = 0.0;
}

PolynomialN::PolynomialN(unsigned int n, const double ai[]) :
  n(n),
  a(new double[n+1])
{
  for (unsigned int ii = n+1; ii--; ) a[ii] = ai[ii];
}

PolynomialN::PolynomialN(const PolynomialN& o) :
  n(o.n),
  a(new double[n+1])
{
  for (unsigned int ii = n+1; ii--; ) a[ii] = o.a[ii];
}

PolynomialN& PolynomialN::operator = (const PolynomialN& o)
{
  if (this != &o) {
    if (o.n != this->n) {
      delete [] this->a;
      this->a = new double[o.n + 1];
      this->n = o.n;
    }
    for (unsigned int ii = n+1; ii--; ) this->a[ii] = o.a[ii];
  }
  return *this;
}

PolynomialN::~PolynomialN()
{
  delete [] a;
}

double PolynomialN::operator () (const double x) const
{
  double r = a[n];
  for (int ii = n; ii--; )
    r = a[ii] + x * r;
  return r;
}

std::ostream& PolynomialN::print(std::ostream& os) const
{
  os << "PolynomialN(n=" << n;
    for (unsigned int ii = 0; ii < n; ii++) {
    os << ", a_" << ii << "=" << a[ii];
  }
  return os << ")";
}

DUECA_NS_END

