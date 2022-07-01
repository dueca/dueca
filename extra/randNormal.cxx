/* ------------------------------------------------------------------   */
/*      item            : randNormal.cxx
        made by         : Rene' van Paassen
        date            : 010615
        category        : body file
        description     :
        changes         : 010615 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define randNormal_cxx
#include <iostream>
#include "randNormal.hxx"
#include <dueca-conf.h>

#include <cstdlib>
#include <cmath>

#ifndef RANDNORMALTEST
DUECA_NS_START
#endif

#define RAND_NORMAL_BOX_MULLER

#if defined(RAND_NORMAL_SUM)
double randNormal()
{
  double rnd = -6.0;

  for (int ii = 12; ii--; ) {
#if defined(HAVE_RANDOM)
    rnd += random()/(RAND_MAX+1.0);
#elif defined(HAVE_RAND)
    rnd += rand()/(RAND_MAX+1.0);
#else
# error "No suitable random function available"
#endif
  }
  return rnd;
}
#elif defined(RAND_NORMAL_BOX_MULLER)
double randNormal()
{
  // Marsaglia polar method.

  static bool gen = true;
  static double R2;
  const double eps = 1e-38;

  if (gen) {
    double U1, U2, S;
    do {
#if defined(HAVE_RANDOM)
      U1 = 2.0*double(random())/double(RAND_MAX)-1.0;
      U2 = 2.0*double(random())/double(RAND_MAX)-1.0;
#elif defined(HAVE_RAND)
      U1 = 2.0*double(rand())/double(RAND_MAX)-1.0;
      U2 = 2.0*double(rand())/double(RAND_MAX)-1.0;
#else
# error "No suitable random function available"
#endif
      S = U1*U1 + U2*U2;
    } while (S < eps || S >= 1.0);
    gen = false;

    double tmpS = sqrt(-2.0*log(S)/S);
    R2 = U2 * tmpS;
    return U1 * tmpS;
  }
  else {
    gen = true;
    return R2;
  }
}
#else
#error "No random algorithm choice defined"
#endif

#ifndef RANDNORMALTEST
DUECA_NS_END
#else

using namespace std;
extern "C" {
int main(int argc, char* argv[])
{
  double sum = 0.0;
  double sumsq = 0.0;
  for (int ii = 1000; ii--; ) {
    double a = randNormal();
    cout << a << endl;
    sum += a;
    sumsq += a*a;
  }
  cout << sum << ' ' << sumsq << endl;
  return 0;
}
}
#endif



