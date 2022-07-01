/* ------------------------------------------------------------------   */
/*      item            : StateChange.hh
        made by         : Rene' van Paassen
        date            : 990713
        category        : header file
        description     :
        changes         : 990713 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef StateChange_hh
#define StateChange_hh

#ifdef StateChange_cc
#endif
#include "SimTime.hxx"
#include <dueca_ns.h>
DUECA_NS_START

/** Templated class, generic for the request of a change of state of
    some module, at some (normally future) time. */
template<class T>
class StateChange
{
public:
  /** Time at which state should be changed. */
  TimeTickType time;

  /** New state effective from that time. */
  T state;

public:

  /** Default constructor. */
  StateChange();

  /** Constructor with time and desired state, normally used. */
  StateChange(const TimeTickType &t, T newstate);

  /** Destructor. */
  ~StateChange();

  /** Compare in time to another state change. */
  bool operator < (const StateChange<T>& other) const;
};

DUECA_NS_END
#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef StateChange_ii
#define StateChange_ii
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
StateChange<T>::StateChange() :
  time(0)
{
  //
}

template<class T>
StateChange<T>::StateChange(const TimeTickType &t, T newstate) :
  time(t), state(newstate)
{
  //
}

template<class T>
StateChange<T>::~StateChange()
{
  //
}

template<class T>
bool StateChange<T>::operator < (const StateChange<T>& other) const
{
  return time < other.time;
}

DUECA_NS_END
#endif
#endif
