/* ------------------------------------------------------------------   */
/*      item            : ActivityBit.cxx
        made by         : Rene' van Paassen
        date            :
        category        : header file
        description     : DUSIME event/stream data object
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ActivityBit_cxx

#include "ActivityBit.hxx"
#include "ActivityDescription.hxx"
#include "ActivityWeaver.hxx"
#include <dassert.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "Arena.hxx"
#include "ArenaPool.hxx"
using namespace std;

DUECA_NS_START
const unsigned int ActivityBit::frac_max = 0xff00;

ActivityBit::ActivityBit(const uint16_t& p_tick_plus,
              const uint16_t& p_frac_in_tick,
              const ActivityBitType& p_type,
              const uint16_t& p_activity,
              const TimeSpec& p_time_spec) :
  tick_plus(p_tick_plus),
  frac_in_tick(p_frac_in_tick),
  type(p_type),
  activity(p_activity),
  time_spec(p_time_spec),
  next(NULL)
{
  // assert(tick_plus < 0xff00);
}

ActivityBit::ActivityBit(const ActivityBit& o) :
  tick_plus(o.tick_plus),
  frac_in_tick(o.frac_in_tick),
  type(o.type),
  activity(o.activity),
  time_spec(o.time_spec),
  next(NULL)
{
}

ActivityBit::ActivityBit(AmorphReStore& s) :
  tick_plus(s),
  frac_in_tick(s),
  type(ActivityBitType(uint8_t(s))),
  activity(s),
  time_spec(s),
  next(NULL)
{
}

ActivityBit::~ActivityBit()
{
  //
}

void* ActivityBit::operator new(size_t size)
{
  assert(size == sizeof(ActivityBit));
  static Arena* my_arena = arena_pool.findArena
    (sizeof(ActivityBit));
  return my_arena->alloc(size);
}

void ActivityBit::operator delete(void* v)
{
  static Arena* my_arena = arena_pool.findArena
    (sizeof(ActivityBit));
  my_arena->free(v);
}


void ActivityBit::packData(AmorphStore& s) const
{
  ::packData(s, tick_plus);
  ::packData(s, frac_in_tick);
  ::packData(s, uint8_t(type));
  ::packData(s, activity);
  ::packData(s, time_spec);
}

ostream & ActivityBit::print(ostream& s) const
{
  s << "ActivityBit(tick_plus=" << tick_plus << ','
    << "frac_in_tick=" << frac_in_tick << ','
    << "type=" << type << ','
    << "activity=" << activity
    << "time=" << time_spec << ')';
  return s;
}

static const char* ActivityBitType_names[] = {
  "LogStart",
  "Suspend",
  "Start",
  "Block",
  "Continue",
  "Graphics",
  "Schedule"};

static const char ActivityBitType_letters[] = {
  'L', 'X', 'R', 'B', 'C', 'G', 'Q' };

const char* const getString(const ActivityBit::ActivityBitType &o)
{
  return ActivityBitType_names[int(o)];
}

const char ActivityBit::getLetter() const
{
  return ActivityBitType_letters[type];
}

const vstring ActivityBit::reportVerbal(int manager_number,
                                        const ActivityWeaver* weaver) const
{
  ostringstream st;
  st << "M" << manager_number
     << setw(9) << tick_plus+weaver->getOffset() << setw(0)
     << '+' << setw(6) << setfill('0')
     << int(weaver->getScale()*frac_in_tick + 0.5)
     << setw(0) << setfill(' ');
  if (hasNamedActivity()) {
    st << "/(" << setw(9) << time_spec.getValidityStart()
       << setw(0) << ',' << setw(3) << time_spec.getValiditySpan()
       << setw(0) << ')';
  }
  else {
    st << "                ";
  }
  st  << setw(9) << getString(type) << setw(0);
  if (hasNamedActivity()) {
    st << " " << weaver->getActivityDescription(activity);
      }
  //st << std::ends;
  return st.str();
}

DUECA_NS_END
