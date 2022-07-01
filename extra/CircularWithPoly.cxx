/* ------------------------------------------------------------------   */
/*      item            : CircularWithPoly.cxx
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


#define CircularWithPoly_cxx

#include "CircularWithPoly.hxx"
#include <cmath>

DUECA_NS_START

CircularWithPoly::CircularWithPoly(double K, double xzero, double norm_start,
                                   size_t n, const double ai[]) :
  K(K),
  xzero(xzero),
  norm_start(norm_start),
  n(n),
  a(new double[n+1])
{
  for (unsigned int ii = n+1; ii--; ) a[ii] = ai[ii];
}

CircularWithPoly::CircularWithPoly(const CircularWithPoly& o) :
  K(o.K),
  xzero(o.xzero),
  norm_start(o.norm_start),
  n(o.n),
  a(new double[o.n+1])
{
  for (unsigned int ii = n+1; ii--; ) a[ii] = o.a[ii];
}

CircularWithPoly& CircularWithPoly::operator = (const CircularWithPoly& o)
{
  if (this != &o) {
    if (o.n != this->n) {
      delete [] a;
      a = new double[n + 1];
    }
    for (unsigned int ii = n+1; ii--; ) this->a[ii] = o.a[ii];
    this->K = o.K;
    this->xzero = o.xzero;
    this->norm_start = o.norm_start;
  }
  return *this;
}


CircularWithPoly::~CircularWithPoly()
{
  delete [] a;
}

double CircularWithPoly::operator () (const double x) const
{
  // normalize input to range 0 .. 1
  double xt = K*(x-xzero) - norm_start;
  xt = xt - floor(xt) + norm_start;
  double r = a[n];
  for (int ii = n; ii--; )
    r = a[ii] + xt * r;
  return r;
}

std::ostream& CircularWithPoly::print(std::ostream& os) const
{
  os << "CircularWithPoly(K=" << K << ", xzero=" << xzero
     << ", start=" << norm_start << ", n=" << n;
  for (unsigned int ii = 0; ii < n; ii++) {
    os << ", a_" << ii << "=" << a[ii];
  }
  return os << ")";
}

DUECA_NS_END

#ifdef UNITTEST
using namespace std;

int main()
{
  double poly[2] = {0.055, 180.0};
  CircularWithPoly p(1.0/1024.0, 980.0, -0.5, 1, poly);

  const int ntval = 3;
  int tval[ntval] = {980, 1023, 1030-1024};

  for (int ii = 0; ii < ntval; ii++) {
    cout << tval[ii] << ' ' << p(tval[ii]) << endl;
  }
  return 0;
}
#endif
