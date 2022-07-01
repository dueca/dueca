/* ------------------------------------------------------------------   */
/*      item            : catch0.cxx
        made by         : Rene' van Paassen
        date            : 030218
        category        : body file
        description     :
        changes         : 030218 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define catch0_cxx

#include <iostream>
using namespace std;

int main()
{
  try {
    throw int(2);
  }
  catch (int& i) {
    cout << "caught " << i << endl;
  }
}
