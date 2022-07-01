/* ------------------------------------------------------------------   */
/*      item            : SinglePack.cxx
        made by         : Rene' van Paassen
        date            : 041110
        category        : body file
        description     :
        changes         : 041110 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define SinglePack_cxx
#include <iostream>

using namespace std;

class TestClass
{
public:
  int i;
  double d;
  int ir[3];
  int i2;
  TestClass() :i(0), d(1.0), i2(2)
  {
    for (int ii = 3; ii--; ) ir[ii] = ii;
  }
};

ostream& operator << (ostream& os, const TestClass& tc)
{
  return os << "i=" << tc.i << " d=" << tc.d << " ir[0]=" << tc.ir[0]
            << " ir[1]=" << tc.ir[1] << " ir[2]=" << tc.ir[2]
            << " i2=" << tc.i2 << endl;
}

void packi(TestClass* tc, void* data, int i)
{
  (*tc) .i = *reinterpret_cast<int*>(data);
}

void packd(TestClass* tc, void* data, void* var)
{
  (*tc) .* ((double TestClass::*) var) = *reinterpret_cast<double*>(data);
}

void packiarray(TestClass* tc, void* data, void* var, int idx)
{
  (*tc) . = *reinterpret_cast<int*>(data);
}

int main()
{
  TestClass tc;

  struct TestClassSinglePackTable
  {
    void (* fun) (TestClass* tc, void*, int);
    void * var;
  };

  TestClassSinglePackTable table[] = {
    { &packi, 0},
    { &packd, (void*) &TestClass::d },
    { &packi, (void*) &TestClass::ir}};


  int ni = 10;
  cout << hex << &(tc.i) << 'p' << &(tc.ir) << ' ' <<&(tc.ir[0]) << ' ' << &(tc.i2) << endl;

  packi(&tc, (void*) &ni, (void*) &TestClass::i);
  cout << tc;

  return 0;
}
