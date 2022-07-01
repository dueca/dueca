/* ------------------------------------------------------------------   */
/*      item            : Inverse.cxx
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

#define Inverse_cxx

#include "Inverse.hxx"

DUECA_NS_START

Inverse::Inverse(double K, double x0, double epsx) :
  K(K),
  x0(x0),
  epsx(epsx)
{
  //
}

Inverse::~Inverse()
{
  //
}

double Inverse::operator () (const double x) const
{
  if (x - x0 >= 0.0 && x - x0 < epsx) {
    return K/epsx;
  }
  else if (x - x0 < 0.0 && x0 - x < epsx) {
    return -K/epsx;
  }
  return K/(x-x0);
}

std::ostream& Inverse::print(std::ostream& os) const
{
  return os << "Inverse(K=" << K << ", x0=" << x0
            << ", epsx=" << epsx << ")";
}


DUECA_NS_END

