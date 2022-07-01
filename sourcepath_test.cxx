/* ------------------------------------------------------------------   */
/*      item            : sourcepath_test.cxx
        made by         : Rene' van Paassen
        date            : 210426
        category        : body file
        description     :
        changes         : 210426 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define sourcepath_test_cxx

#include <cstring>
#include <iostream>

int main(int argc, char** argv)
{
  std::string fname = __FILE__;
  size_t match = fname.rfind("sourcepath_test.cxx");

  if (match > fname.length()) {
    std::cerr << "Error calculation source path size" << std::endl
	      << "filename with path:" << __FILE__ << std::endl;
    return 1;
  }
  std::cout << int(match) << std::endl;
  return 0;
}
