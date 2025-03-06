/* ------------------------------------------------------------------   */
/*      item            : Dstring.cxx
        made by         : Rene' van Paassen
        date            : 010215
        category        : body file
        description     :
        changes         : 010215 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "Dstring.ixx"
#include <dassert.h>

DUECA_NS_START

template class Dstring<5>;
template class Dstring<8>;
template class Dstring<16>;
template class Dstring<32>;
template class Dstring<40>;
template class Dstring<64>;
template class Dstring<128>;
const char *dstring::classname = "Dstring";

/** Create a single check on the sizes of all Dstring objects. A size
    change would wreak havoc on a lot of stuff. */
struct CheckDStringSize
{
  CheckDStringSize()
  {
    assert(sizeof(Dstring<5>) == 5);
    assert(sizeof(Dstring<8>) == 8);
    assert(sizeof(Dstring<16>) == 16);
    assert(sizeof(Dstring<32>) == 32);
    assert(sizeof(Dstring<40>) == 40);
    assert(sizeof(Dstring<64>) == 64);
    assert(sizeof(Dstring<128>) == 128);
  }
};
static CheckDStringSize check;

DUECA_NS_END

PRINT_NS_START
// instantiate templated print
template std::ostream &operator<<(std::ostream &os,
                                  const DUECA_NS ::Dstring<5> &o);
template std::ostream &operator<<(std::ostream &os,
                                  const DUECA_NS ::Dstring<8> &o);
template std::ostream &operator<<(std::ostream &os,
                                  const DUECA_NS ::Dstring<16> &o);
template std::ostream &operator<<(std::ostream &os,
                                  const DUECA_NS ::Dstring<32> &o);
template std::ostream &operator<<(std::ostream &os,
                                  const DUECA_NS ::Dstring<40> &o);
template std::ostream &operator<<(std::ostream &os,
                                  const DUECA_NS ::Dstring<64> &o);
template std::ostream &operator<<(std::ostream &os,
                                  const DUECA_NS ::Dstring<128> &o);
PRINT_NS_END

// instantiate templated pack and unpack
//template void packData(DUECA_NS ::AmorphStore &s,
//                       const DUECA_NS ::Dstring<5> &o);
//template void unPackData(DUECA_NS ::AmorphReStore &s, DUECA_NS ::Dstring<5> &o);
template void packData(DUECA_NS ::AmorphStore &s,
                       const DUECA_NS ::Dstring<8> &o);
template void unPackData(DUECA_NS ::AmorphReStore &s, DUECA_NS ::Dstring<8> &o);
template void packData(DUECA_NS ::AmorphStore &s,
                       const DUECA_NS ::Dstring<16> &o);
template void unPackData(DUECA_NS ::AmorphReStore &s,
                         DUECA_NS ::Dstring<16> &o);
template void packData(DUECA_NS ::AmorphStore &s,
                       const DUECA_NS ::Dstring<32> &o);
template void unPackData(DUECA_NS ::AmorphReStore &s,
                         DUECA_NS ::Dstring<32> &o);
template void packData(DUECA_NS ::AmorphStore &s,
                       const DUECA_NS ::Dstring<40> &o);
template void unPackData(DUECA_NS ::AmorphReStore &s,
                         DUECA_NS ::Dstring<40> &o);
template void packData(DUECA_NS ::AmorphStore &s,
                       const DUECA_NS ::Dstring<64> &o);
template void unPackData(DUECA_NS ::AmorphReStore &s,
                         DUECA_NS ::Dstring<64> &o);
template void packData(DUECA_NS ::AmorphStore &s,
                       const DUECA_NS ::Dstring<128> &o);
template void unPackData(DUECA_NS ::AmorphReStore &s,
                         DUECA_NS ::Dstring<128> &o);
