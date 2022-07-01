/* ------------------------------------------------------------------   */
/*      item            : StatusT1.cxx
        made by         : Rene' van Paassen
        date            : 010824
        category        : body file
        description     :
        changes         : 010824 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define StatusT1_cxx
#include "StatusT1.hxx"
#include <EntityManager.hxx>
#include <DuecaView.hxx>
#include <debug.h>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

StatusT1::StatusT1() :
  mstate(ModuleState::Neutral),
  sstate(SimulationState::Neutral),
  m_actualised_at(MAX_TIMETICK),
  s_actualised_at(MAX_TIMETICK)
{
  //
}

StatusT1::StatusT1(const StatusT1& o) :
  mstate(o.mstate),
  sstate(o.sstate),
  m_actualised_at(o.m_actualised_at),
  s_actualised_at(o.s_actualised_at)
{
  //
}

StatusT1::~StatusT1()
{
  //
}

StatusT1& StatusT1::operator &= (const StatusT1& o)
{
  // update time. Oldest time is the rule, unless one of the two has
  // no real state, and is in neutral. In that case, is is initialised
  // to MAX_TIMETICK, and the min function works again.
  DEB("combining " << *this << "&=" << o);
  m_actualised_at = min(m_actualised_at, o.m_actualised_at);
  s_actualised_at = min(s_actualised_at, o.s_actualised_at);

  // state. Oring is enough
  mstate &= o.mstate;
  sstate &= o.sstate;
  DEB("result " << *this);

  return *this;
}

void StatusT1::clear()
{
  mstate = ModuleState::Neutral;
  m_actualised_at = MAX_TIMETICK;
  sstate = SimulationState::Neutral;
  s_actualised_at = MAX_TIMETICK;
}

ostream& StatusT1::print(ostream& os) const
{
  os << "StatusT1(mstate=" << mstate << '/' << m_actualised_at;
  os << ", sstate=" << sstate << '/' << s_actualised_at;
  return os << ')';
}

bool StatusT1::operator == (const StatusT1 & o) const
{
  return mstate == o.mstate && sstate == o.sstate &&
    m_actualised_at == o.m_actualised_at &&
    s_actualised_at == o.s_actualised_at;
}

bool StatusT1::equiv(const StatusT1 & o) const
{
  return mstate == o.mstate && sstate == o.sstate;
}

DUECA_NS_END
