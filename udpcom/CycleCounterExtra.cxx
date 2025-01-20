/* ------------------------------------------------------------------   */
/*      item            : CycleCounterExtra.hxx
        made by         : Rene van Paassen
        date            : 200612
        category        : header file
        description     :
        changes         : 2001612 first version
        language        : C++
        copyright       : (c) 2020 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define __CUSTOM_COMPATLEVEL_110

#define DEBPRINTLEVEL -1
#include <debprint.h>

#define __CUSTOM_FUNCTION_PRINT
std::ostream & CycleCounter::print(std::ostream& s) const
{
  s << (this->cycle_counter >> 4) << "+" << (this->cycle_counter & 0xf);
  return s;
}

#if 0
bool CycleCounter::cycleHasBeenProcessed(uint32_t cycle) const
{
  // accept a leeway of 2 messages??
  bool res =
    ((cycle & ~0xf) == (cycle_counter & ~0xf) + 0x10) || // normal, advance
    ((cycle & ~0xf) == (cycle_counter & ~0xf)) || // repeat in progress
    ((cycle & ~0xf) + 0x10 == (cycle_counter & ~0xf)) || // recovery
    ((cycle & ~0xf) + 0x20 == (cycle_counter & ~0xf)); // second recov
#if DEBPRINTLEVEL > 0
  if (res) return res;
  DEB("Cycle count insufficient " << (cycle_counter >> 4) << " versus " <<
      (cycle >> 4));
#endif
  return res;
}
#endif
