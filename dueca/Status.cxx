/* ------------------------------------------------------------------   */
/*      item            : ModuleStatus.cxx
        made by         : Rene' van Paassen
        date            : 010822
        category        : body file
        description     :
        changes         : 010822 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ModuleStatus_cxx
#include "Status.hxx"

#include <ModuleState.hxx>
#include <dusime/SimulationState.hxx>
#include <dassert.h>
DUECA_NS_START

template <class S>
Status<S>::Status()
{
  //
}

template <class S>
Status<S>::Status(const S &status) :
  status(status)
{
  //
}

template <class S>
Status<S>::Status(const Status& o) :
  status(o.status)
{
  //
}

template <class S>
Status<S>::~Status()
{
  //
}

template <class S>
GenericStatus* Status<S>::clone() const
{
  return new Status(*this);
}

template <class S>
void Status<S>::operatorAndEq (const GenericStatus& o)
{
  if (this != &o) return;
  assert (dynamic_cast<const Status<S>*>(&o) != NULL);
  status &= dynamic_cast<const Status<S>*>(&o)->status;
}

template <class S>
ostream& Status<S>::print(ostream& os) const
{
  return os << status;
}

template <class S>
void Status<S>::clear()
{
  status.neutral();
}

template <class S>
bool Status<S>::operator == (const GenericStatus& o) const
{
  if (dynamic_cast<const Status<S>*>(&o) == NULL) return false;
  return status == dynamic_cast<const Status<S>*>(&o)->status;
}

template Status<ModuleState>;
#ifdef BUILD_DMODULES
template Status<SimulationState>;
#endif
DUECA_NS_END
