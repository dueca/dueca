#define KEEP_STATIC_INLINE
#include <iostream>
using namespace std;
extern "C" {
#include <rtai_lxrt.h>
}
int main()
{

  unsigned long varwait_name = nam2num("inivw");
  unsigned long varwait_name2 = nam2num("runvw");

  cout << "asked address = " << hex << rt_get_adr(varwait_name)
       << "other address = " << hex << rt_get_adr(varwait_name2) << endl;

  return 0;
}

