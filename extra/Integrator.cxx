/* ------------------------------------------------------------------   */
/*      item            : Integrator.cxx
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
#include "Integrator.hxx"
#include <cstdio>
#include <algorithm>
using namespace std;
namespace dueca {

static double int_num[] = {1.0};
static double int_den[] = {0.0, 1.0};

Integrator::Integrator(double dt) :
    LimitedLinearSystem(VectorE(int_num, 1), VectorE(int_den, 2), dt)
{
}

Integrator::Integrator(double lower_limit, double upper_limit, double dt) :
    LimitedLinearSystem(VectorE(int_num, 1), VectorE(int_den, 2), dt)
{
  setSaturationLimits(lower_limit, upper_limit);
}

Integrator::~Integrator()
{
}

void Integrator::setDT(double dt)
{
  createFromNumDen(VectorE(int_num, 1), VectorE(int_den, 2), dt);
}

void Integrator::setSaturationLimits(double lower_limit, double upper_limit)
{
  Vector lltmp, ultmp;
  lltmp << lower_limit;
  ultmp << upper_limit;
  LimitedLinearSystem::setSaturationLimits(lltmp, ultmp);
}

void Integrator::setSaturationLimits(const Vector& lower, const Vector& upper)
{
  LimitedLinearSystem::setSaturationLimits(lower, upper);
}

void Integrator::acceptState(double x_new)
{
  x[0] = x_new;
}

void Integrator::acceptState(const Vector& x_new)
{
  x[0] = x_new[0];
}

const Vector& Integrator::step(const Vector& u)
{
  if (ll.size() == 0) return LinearSystem::step(u);
  else                return LimitedLinearSystem::step(u);
}

const Vector& Integrator::step(double u)
{
  if (ll.size() == 0) return LinearSystem::step(u);
  else                return LimitedLinearSystem::step(u);
}

} // namespace dueca