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
#include "Enum10.hxx"
#include <AmorphStore.hxx>
#include <iostream>
#include <sstream>

USING_DUECA_NS;

int main()
{
  char buff[1000];

  AmorphStore st(buff, 1000);

  Enum10 o1 = Enum10::Two;
  packData(st, o1);
  AmorphReStore re(buff, st.getSize());
  Enum10 o1c;
  unPackData(re, o1c);
  assert(o1 == o1c);
  std::cout << o1 << std::endl;
  stringstream ois;
  ois << o1;
  std::cout << ois.str() << std::endl;
  Enum10 o1r;
  ois >> o1r;
  assert(o1 == o1r);
  return 0;
}
