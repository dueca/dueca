/* ------------------------------------------------------------------   */
/*      item            : sigcpp.cxx
        made by         : Rene' van Paassen
        date            : 060102
        category        : body file
        description     :
        changes         : 060102 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define sigcpp_cxx
#include <sigc++/sigc++.h>
#include <iostream>
using namespace std;

class MyClass: public sigc::trackable
{
public:
  void methodA() { cout << "called methodA" << endl; }
  MyClass() { cout << "constructor" << endl; }
  ~MyClass() { cout << "destructor" << endl; }
};

struct Table
{
  const char* name;
  sigc::slot_base sb;
};

int main()
{
  MyClass mc;

  Table table[] =
    { { "first", sigc::slot<void>(sigc::mem_fun(mc, &MyClass::methodA)) },
      { NULL } };

  sigc::signal<void> mysig;
  mysig.connect(table[0].sb);
  mysig.emit();
}
