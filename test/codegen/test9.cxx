/* ------------------------------------------------------------------   */
/*      item            : test9.cxx
        made by         : Rene' van Paassen
        date            : 121230
        category        : body file
        description     :
        changes         : 121230 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define test9_cxx
#include <cassert>
#include <vector>
#include "Object9.hxx"
#include <AmorphStore.hxx>


USING_DUECA_NS;

int main()
{
  char buff[1000];

  AmorphStore st(buff, 1000);

  Object9 o1(Object9::One, Object9::Num2::One);
  packData(st, o1);
  Object9 o2(Object9::Two, Object9::Num2::One);
  packDataDiff(st, o2, o1);
  AmorphReStore re(buff, st.getSize());
  Object9 o1c(re);
  Object9 o2c(o1c);
  unPackDataDiff(re, o2c);
  assert(o1 == o1c);
  assert(o2 == o2c);

  return 0;
}
