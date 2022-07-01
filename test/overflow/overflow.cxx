#include <SimTime.hxx>
#include <PeerTiming.hxx>
#include <iostream>
#include <cassert>

using namespace std;
using namespace dueca;

#define DEB(A)
#ifndef DEB
#define DEB(A) cout << A << endl;
#endif
#define DEBR(A)
#ifndef DEBR
#define DEBR(A) cout << A << endl;
#endif

typedef TimeTickType T;

int main()
{
  TimeTickType t0 = 100023;
  TimeTickType t1 = 5000;

  // local check, correct from t1 to t0 timin
  TimeTickType correct01 = t0 - t1;

  assert(t0 + 8U == t1 + 8U + correct01);

  // local check, correct from t1 to t0 timin
  TimeTickType correct10 = t1 - t0;

  assert(t1 + 8U == t0 + 8U + correct10);

  // now with rounding?
  T delta01_plus5 = (correct01 + T(5)) % T(10);
  T correct01b = correct01 + T(5) - delta01_plus5;
  assert(correct01b + T(5) - correct01 < 10);

  T jump = 20;

  for (t0 = 4981U; t0 < 5042; t0++) {
    PeerTiming pt(jump);
    pt.adjustDelta(t0, t1, false);
    //assert(long(t1 + pt.getTransition()+2*jump) - long(t0+2*jump) <= jump/2);
    //std::cout << pt.getTransition();
  }

  t0 = 4981U;
  PeerTiming pt(jump, 0.01);
  // unsigned cnt = 0;
  pt.adjustDelta(t0, t1, false);
  /*  while(pt.newTransition() != 0) {
    pt.adjustDelta(t1, t1, false);
    cnt++;
  }
  assert(cnt == 133);
  std::cout << "jump adjustment after " << cnt << " cycles" << std::endl;
  */

  // TimeTickType correct02b = TimeTickType(0) -
  //       ((t1 + TimeTickType(5) - t0) % TimeTickType(10));
  return 0;
}
