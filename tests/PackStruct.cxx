#include <iostream>
using namespace std;

struct PS
{
  unsigned int part1 : 8;
  unsigned int part2 : 24;
};

int main()
{
  cout << sizeof(PS) << endl;

  union {
    PS p;
    uint32_t i;
  } u;

  u.p.part1 = 5;
  u.p.part2 = 258;

  cout << hex << u.i << endl;
}
