#if 0
/* ------------------------------------------------------------------   */
/*      item            : IcalcTest.cxx
        made by         : Rene' van Paassen
        date            : 010403
        category        : body file
        description     :
        changes         : 010403 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define IcalcTest_cxx
#include "IcalcTest.hxx"

#include <dueca-conf.h>

#ifndef BUILD_DMODULES
int main()
{
  return 0;
}
#else

#include <IntervalCalculation.hxx>
#include <list>
#include <cmath>
#include <mtl/mtl.h>
#include <dueca_ns.h>
using namespace mtl;
USING_DUECA_NS

#define EPS 1e-5

static void f(Vector& x, Vector& y)
{
  y.resize(3);
  y[0] = 4.0 * std::sqrt(x[0]) - 2.0 + 0.2 * x[1]*x[1];
  y[1] = x[2] + 3.0;
  y[2] = 0.5 * x[2] + std::sqrt(x[1]);
}

int main(int argc, char* argv[])
{
  // make a solution
  DUECA_NS ::IntervalCalculation icalc;

  //  double xmind[] = {0.0, 0.0, -5.0};
  //double xmaxd[] = {10.0, 10.0, 10.0};
  //double xmind[] = { 0.05 , 2.1, -3.1};
  //double xmaxd[] = {0.07, 2.3, -2.9};
  double xmind[] = { 0.0 , 1.0, -5.1};
  double xmaxd[] = {0.3, 4.0, -0.9};
  Vector xmin(3), xmax(3);
  for (int ii = 3; ii--; ) {
    xmin[ii] = xmind[ii];
    xmax[ii] = xmaxd[ii];
  }

  icalc.initialise(xmin, xmax, 3);

  Vector x(3), y(3);

  bool stop = false;
  while (!stop) {

    // ask all values that need to be evaluated
    list<Vector> all_y;
    while (icalc.needEvaluation(x) != -1) {
      Vector y; f(x, y);
      all_y.push_back(y);
      cout << "Evaluated for x=" << x[0] << ' ' << x[1] << ' ' << x[2]
           << "              y=" << y[0] << ' ' << y[1] << ' ' << y[2] << endl;
    }

    // push them all in the calculator
    int cnt = 0;
    for (list<Vector>::iterator ii = all_y.begin();
         ii != all_y.end(); ii++) {
      if (cnt > 3 && cnt % 3 == 1) cnt++;
      icalc.mergeResult(cnt++, *ii);
      if (cnt == 2) {
        for (int jj = 3; --jj; ) {
          icalc.mergeResult(1 + jj*3, *ii);
        }
      }
    }

    // let the calculator run
    icalc.step();

    Vector y(3); icalc.getResult(y);
    cout << "new estimate values " << y[0] << ' ' << y[1] << ' '
         << y[2] << endl;
    // stop if close
    stop = std::fabs(y[0]) < EPS && std::fabs(y[1]) < EPS && std::fabs(y[2]) < EPS;
  }
}
#endif
#endif
