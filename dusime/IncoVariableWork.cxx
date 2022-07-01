/* ------------------------------------------------------------------   */
/*      item            : IncoVariableWork.cxx
        made by         : Rene' van Paassen
        date            : 200527
        category        : body file
        description     :
        changes         : 200527 first version
        language        : C++
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define IncoVariableWork_cxx
#include "IncoVariableWork.hxx"
#include <cmath>
static const double EPS_INCO = 1e-5;


DUECA_NS_START;

IncoVariableWork::IncoVariableWork() :
  IncoVariable("invalid", 0.0, 0.0),
  value(0.0),
  target(0.0)
{
  //
}

IncoVariableWork::IncoVariableWork(const IncoVariable& o) :
  IncoVariable(o),
  value(0.0),
  target(0.0)
{
  //
}

IncoVariableWork::~IncoVariableWork()
{

}

IncoVariableWork& IncoVariableWork::operator = (const IncoVariable& o)
{
  *reinterpret_cast<IncoVariable*>(this) = o;
  return *this;
}

bool IncoVariableWork::merge(IncoMode mode, double val)
{
  if (queryInsertForThisMode(mode)) {
    // the value should have come back as supplied
    if (fabs(val - value) > EPS_INCO) {
      cerr << "Trim variable " << name << " should not have mutated"
            << endl;
      return false;
    }
  }
  return true;
}

bool IncoVariableWork::meetsTarget(IncoMode mode) const
{
  return (findRole(mode) != Target) ||
    fabs(value - target) <= tolerance;
}

bool IncoVariableWork::isUserControllable(IncoMode mode) const
{
  return findRole(mode) == Constraint ||
    (findRole(mode) == Target && fabs(max_value - min_value) > 1e-10);
}

void IncoVariableWork::setTarget(double newval)
{
  if (newval < min_value) newval = min_value;
  if (newval > max_value) newval = max_value;
  target = newval;
}

DUECA_NS_END;
