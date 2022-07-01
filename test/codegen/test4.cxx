/* ------------------------------------------------------------------   */
/*      item            : test4.cxx
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

#define test4_cxx
#include <cassert>
#include "Object4.hxx"
#include <AmorphStore.hxx>

USING_DUECA_NS;

int main()
{
  char buff[1000];

  AmorphStore st(buff, 1000);

  Object4 o1;
  o1.a.push_back(0.0);
  o1.a.push_back(0.0);
  o1.a.push_back(0.0);
  o1.a.push_back(0.0);
  Object4 o2 = o1;
  o2.a.front() = -1.3;
  list<double>::iterator ii = o2.a.begin();
  ii++; ii++; ii++; *ii = 6;
  cout << o1 << endl << o2 << endl;
  packData(st, o1);
  packDataDiff(st, o2, o1);
  Object4 o3(o2);
  packDataDiff(st, o3, o2);
  packData(st, o3);
  Object4 o4(o2);
  o4.a.front() = 5;
  packDataDiff(st, o4, o2);
  Object4 o5(o1);
  o5.a.front() = 10;
  packDataDiff(st, o5, o1);
  Object4 o6(o1);
  *(--o6.a.end()) = 10;
  packDataDiff(st, o6, o1);
  AmorphReStore re(buff, st.getSize());

  Object4 o1r(re), o2r(o1);
  unPackDataDiff(re, o2r);

  cout << o1r << endl;
  cout << o2r << endl;

  assert (o1 == o1r);
  assert (o2 == o2r);
  Object4 o3r(o2); cout << "o3r"<< endl;
  unPackDataDiff(re, o3r);
  Object4 o3s(re); cout << "o3s" << endl;
  assert(o3 == o3r);
  assert(o3 == o3s);
  Object4 o4r(o2); cout << "o4r" << endl;
  unPackDataDiff(re, o4r);
  assert(o4 == o4r);
  Object4 o5r(o1); cout << "o5r" << endl;
  unPackDataDiff(re, o5r);
  assert(o5 == o5r);
  Object4 o6r(o1); cout << "o6r" << endl;
  unPackDataDiff(re, o6r);
  assert(o6 == o6r);
  assert (re.getSize() == 0);
  return 0;
}
