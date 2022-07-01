/* ------------------------------------------------------------------   */
/*      item            : atomic_test.cxx
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

#define atomic_test_cxx

#include <atomic>

int main(int argc, char** argv)
{
  uint64_t one;
  std::atomic<uint64_t> two;
  uint64_t tmp;

  two = 2; one = 1; tmp = two;
  if (!std::atomic_compare_exchange_strong(&two, &tmp, one)) {
    return 1;
  }
  return 0;
}
