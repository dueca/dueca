/* ------------------------------------------------------------------   */
/*      item            : Circular.cxx
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


#define Circular_cxx

#include "Circular.hxx"

DUECA_NS_START

Circular::Circular(double K, double izero, double range, double start) :
  K(K),
  izero(izero),
  range(range),
  norm_start(start/range)
{
  //
}

Circular::~Circular()
{
  //
}

double Circular::operator () (const double x) const
{
  // normalize input to range 0 .. 1
  double n = K*(x-izero);
  while (n < norm_start) n += 1.0;
  while (n > norm_start + 1.0) n -= 1.0;
  return range*n;
}

std::ostream& Circular::print(std::ostream& os) const
{
  return os << "Circular(K=" << K << ", izero=" << izero
            << ", range=" << range << ", start=" << norm_start << ")";
}
DUECA_NS_END

