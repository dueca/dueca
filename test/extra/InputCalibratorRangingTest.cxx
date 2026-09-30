/* ------------------------------------------------------------------   */
/*      item            : InputCalibratorRangingTest.cxx
        made by         : Rene' van Paassen
        date            : 260930
        category        : body file
        description     :
        changes         : 260930 first version
        language        : C++
        copyright       : (c) 2026 René van Paassen
        license         : EUPL-1.2
*/

#define InterpTest_cxx

#include <extra/InputCalibratorRanging.hxx>
#include <extra/PolynomialN.hxx>
#include <cassert>
#include <iostream>
#include <cmath>

using namespace dueca;

template <typename FN>
void check(const char *name, const FN &s, double u, double y)
{
  auto y2 = s(u);
  std::cout << name << "(" << u << ") = " << y2 << " ? " << y << std::endl;
  assert(std::abs(y - y2) < 1e-8);
}

static double ai[] = { 0.0, 0.001 };

int main()
{
  InputCalibratorRanging<PolynomialN> ic1(-20000, 20000,
                                          dueca::PolynomialN(1, ai), -1.0, 1.0);

  ic1.newConversion(0);
  ic1.newConversion(-100);
  ic1.newConversion(1900);
  bool ic1off = ic1.lockOffset();
  assert("lock first calibrator correct" && ic1off);
  std::cout << "IC1 " << ic1.lockReason() << std::endl;

  ic1.resetHoming();
  ic1.newConversion(0);
  ic1.newConversion(-100);
  ic1.newConversion(2200);

  bool ic1off2 = ic1.lockOffset();
  assert("lock second false" && !ic1off2);
  std::cout << "IC2 " << ic1.lockReason() << std::endl;

  ic1.resetHoming();
  ic1.newConversion(-1000);
  ic1.newConversion(1000);
  bool ic1off3 = ic1.lockOffset();
  assert("lock third calibrator correct" && ic1off3);
  std::cout << "IC3 " << ic1.lockReason() << std::endl;

  return 0;
}
