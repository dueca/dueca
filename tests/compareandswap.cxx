/* ------------------------------------------------------------------   */
/*      item            : compareandswap.cxx
        made by         : Rene' van Paassen
        date            : 130114
        category        : body file
        description     :
        changes         : 130114 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define compareandswap_cxx
#include <pthread.h>
#include <iostream>
#include <cstdio>
using namespace std;


volatile int common = 0;
volatile int count = 0;

static void* catchloop(void* arg)
{
  int mynum = reinterpret_cast<long int>(arg);
  for (int ii = 100000; ii--; ) {
    int oldc = common;
    while (!__sync_bool_compare_and_swap(&common, oldc, mynum)) {
      cerr << "thought " << oldc << " got " << common << endl;
      oldc = common;
      __sync_fetch_and_add(&count, 1);
    }
  }
}


int main()
{

  pthread_t t1[10];

  for (int ii = 10; ii--; ) {
    int res = pthread_create(&t1[ii], NULL, catchloop,
                             reinterpret_cast<void*>(ii));
    if (!res) {
      perror("creating thread");
    }
  }
  for (int ii = 10; ii--; ) {
    pthread_join(t1[ii], NULL);
  }
  cout << "Number of surprises " << count << endl;
  return 0;
}

