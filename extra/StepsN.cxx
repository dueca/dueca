/* ------------------------------------------------------------------   */
/*      item            : StepsN.cxx
        made by         : Rene' van Paassen
        date            : 030228
        category        : body file
        description     :
        changes         : 030228 first version
                          040219 RvP, discovered first error
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define StepsN_cxx
#define DO_INSTANTIATE
#include "StepsN.hxx"
#include <float.h>
#include <cmath>
#include <exception>

class steps_exception: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return "steps need 2 or more values"; }
};

DUECA_NS_START

static void StepsN_sort_and_copy(double *ui, double *yi,
                                 double *tp, double *y, int n)
{
  // creating this with n < 1 is not possible
  if (n < 1) {
    throw(steps_exception());
  }

  // for efficient look-up, values must be sorted in increasing
  // order. Start with a simple bubble sort
  int order[n];
  for (int ii = n; ii--; ) order[ii] = ii;

  bool sorted = false;
  while (!sorted) {
    sorted = true;
    for (int ii = n; --ii; ) {
      if (ui[order[ii]] < ui[order[ii-1]]) {
        sorted = false;        int tmp = order[ii];
        order[ii] = order[ii-1]; order[ii-1] = tmp;
      }
    }
  }

  // copy the result, calculate the transition points
  for (int ii = n; --ii; ) {
    y[ii] = yi[order[ii]];
    tp[ii-1] = 0.5*(ui[order[ii]] + ui[order[ii-1]]);
  }
  y[0] = yi[order[0]];
}

StepsN::StepsN(unsigned int n, double yi[], double ui[]) :
  n(n),
  y(new double[n > 1 ? n : 1]),
  ui(new double[n > 1 ? n : 1]),
  tp(new double[n > 2 ? n - 1 : 1])
{
  StepsN_sort_and_copy(ui, yi, tp, y, n);
  for (unsigned ii=n; ii--; ) {
    this->ui[ii] = ui[ii];
  }
}

StepsN::StepsN(const StepsN& o) :
  n(o.n),
  y(new double[o.n > 1 ? o.n : 1]),
  ui(new double[o.n > 1 ? o.n : 1]),
  tp(new double[o.n > 2 ? o.n - 1 : 1])
{
  this->operator = (o);
}

StepsN& StepsN::operator = (const StepsN& o)
{
  if (this != &o) {
    if (o.n != this->n) {
      delete [] y; y = new double[o.n > 1 ? o.n : 1];
      delete [] ui; ui = new double[o.n > 1 ? o.n : 1];
      delete [] tp; tp = new double[o.n > 2 ? o.n - 1 : 1];
      this->n = o.n;
    }
    for (unsigned int ii = n; ii--; ) {
      this->y[ii] = o.y[ii];
      this->ui[ii] = o.ui[ii];
    }
    for (unsigned int ii = n-1; ii--; ) {
      this->tp[ii] = o.tp[ii];
    }
  }
  return *this;
}


StepsN::~StepsN()
{
  delete [] y;
  delete [] ui;
  delete [] tp;
}

double StepsN::operator () (const double x) const
{
  for (int ii = n - 1; ii--; ) {
    if (x > tp[ii]) return y[ii+1];
  }
  return y[0];
}

std::ostream& StepsN::print(std::ostream& os) const
{
  os << "StepsN(n=" << n << "y:" << y[0];
  for (unsigned int ii = 1; ii < n; ii++) {
    os << " <" << tp[ii-1] << "> " << y[ii];
  }
  return os << ")";
}

DUECA_NS_END
