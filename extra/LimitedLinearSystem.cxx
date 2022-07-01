/* ------------------------------------------------------------------   */
/*      item            : LimitedLinearSystem.cxx
        made by         : Joost Ellerbroek
        date            : 090515
        category        : body file
        description     :
        changes         : 090515 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#include "LimitedLinearSystem.hxx"
#include <cstdio>
#include <algorithm>
using namespace std;

static const char *e_state_limit_size =
"Limit vector sizes don't match state vector size";

static const char *e_new_state_limit =
"Assigned new state exceeds state limits";

LimitedLinearSystem::LimitedLinearSystem() :
    LinearSystem(),
    saturation(false)
{
}

LimitedLinearSystem::LimitedLinearSystem(const Vector& num, const Vector& den, double dt) :
    LinearSystem(num, den, dt),
    saturation(false)
{
}

LimitedLinearSystem::LimitedLinearSystem(const LimitedLinearSystem & rhs) :
    LinearSystem(rhs), ul(rhs.ul), ll(rhs.ll), saturation(rhs.saturation)
{
}

LimitedLinearSystem::LimitedLinearSystem(const Matrix& A, const Matrix& B, const Matrix& C, const Matrix& D, double dt) :
    LinearSystem(A, B, C, D, dt),
    saturation(false)
{
}

LimitedLinearSystem::~LimitedLinearSystem()
{
}

LimitedLinearSystem& LimitedLinearSystem::operator = (const LimitedLinearSystem& o)
{
  LinearSystem::operator=(o);
  ul = o.ul;
  ll = o.ll;
  saturation = o.saturation;

  return *this;
}

void LimitedLinearSystem::setSaturationLimits(const Vector& lower, const Vector& upper)
{
  if (x.size() != lower.size() || x.size() != upper.size())
    throw (LinSysException(__FILE__, __LINE__, e_state_limit_size));
  ul = upper;
  ll = lower;
}

void LimitedLinearSystem::acceptState(const Vector& x_new)
{
  x = x_new;
  for (int i = 0; i < x.size(); ++i) {
    if (x(i) < ll(i) || x(i) > ul(i))
      throw (LinSysException(__FILE__, __LINE__, e_new_state_limit));
  }
}

void LimitedLinearSystem::reset()
{
  x.setZero();
  y.setZero();
  saturation = false;
}

const Vector& LimitedLinearSystem::step(const Vector& u)
{
  // Step
  x = Psi * u + Phi * x;

  // Limit
  saturation = false;
  for (int i = 0; i < x.size(); ++i) {
    if (x(i) < ll(i) || x(i) > ul(i)) saturation = true;
    x(i) = min(max(ll(i), x(i)), ul(i));
  }

  // Output equation
  y = C * x + D * u;
  return y;
}

const Vector& LimitedLinearSystem::step(double u)
{
  Vector vu(1);vu<<u;
  return step(vu);
}
