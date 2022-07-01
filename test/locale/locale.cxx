#include <locale>
#include <locale.h>
#include <sstream>
#include <iostream>
using namespace std;

int main(int argc, const char* argv[])
{
  setlocale(LC_NUMERIC, "");
  double f1, f2;
  {
    std::stringstream tester;
    std::cout << "LC_NUMERIC " << setlocale(LC_NUMERIC, NULL) << std::endl;
    tester.imbue(std::locale(setlocale(LC_NUMERIC, NULL)));
    tester << "1,2";
    tester >> f1;
  }
  {
    std::stringstream tester;
    tester.imbue(std::locale(setlocale(LC_NUMERIC, NULL)));
    tester << "1.2";
    tester >> f2;
  }
  if (f1 != 1.2 && f2 == 1.2) {
    // correct
  }
  else if (f1 == 1.2 && f2 != 1.2) {
    std::cerr << "Uncommon locale, take care with datafiles" << std::endl;
    std::cerr << "Numeric locale: " << setlocale(LC_NUMERIC, NULL) << std::endl;
    return 1;
  }
  else {
    std::cerr << "Cannot make heads or tails from locale" << std::endl;
    return 2;
  }
  std::cout << f1 << " " << f2<< std::endl;
  return 0;
}
