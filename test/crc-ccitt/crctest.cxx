/* ------------------------------------------------------------------   */
/*      item            : ddff.cxx
        made by         : Rene' van Paassen
        date            : 211218
        category        : body file
        description     :
        changes         : 211218 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ddff_cxx
#include <CRCcheck.hxx>
#include <cassert>
#include <iostream>

using namespace dueca;

int main()
{
  char const  data[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
     0x38, 0x39 };

  assert(crc16_ccitt(data, sizeof(data)) == 0x29B1);

  return 0;
}
