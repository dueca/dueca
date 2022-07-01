/* ------------------------------------------------------------------   */
/*      item            : sigwait.cxx
        made by         : Rene' van Paassen
        date            : 030212
        category        : body file
        description     : Test of sigwait timing, written after
                          detecting problems on some RedHat 7.3
                          machines; sigwait would not wait
                          consistently 10 ms
        changes         : 030212 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <cstdio>
#include <sys/time.h>
#include <inttypes.h>
#include <sched.h>
#include <cstdlib>
using namespace std;

inline int64_t getclock()
{
  timeval tv;
  gettimeofday(&tv, NULL);
  return int64_t(tv.tv_sec)*1000000 + tv.tv_usec;
}

int main(int argc, char* argv[])
{
  // 4.3 ms, out of any clock Hz clock
  int usecs_in_dt = 4300;

  // or whatever the user gave us
  if (argc >= 2) {
    usecs_in_dt = atoi(argv[1]);
  }

  // room for remembering
  int delay[1000];

  // over to real-time/FIFO
  sched_param rtpar; rtpar.sched_priority = 50;
  int err = sched_setscheduler(0, SCHED_FIFO, &rtpar);
  if (err) {
    perror("setting prio");
  }

  // get to wait the first time
  uint64_t base = getclock() + usecs_in_dt;
  struct timespec timeout =
    {0, usecs_in_dt*1000};

  if (nanosleep(&timeout, NULL) != 0) {
      perror("nanosleep");
  }

  int64_t before = getclock();

  // loop and wait
  for (int ii = 0; ii < 1000; ii++ ) {

    base += usecs_in_dt;
    uint64_t now = getclock();
    if (base > now) {
      struct timespec timeout =
        {(base-now) / 10000000, ((base-now) % 1000000)*1000};
      if (nanosleep(&timeout, NULL) != 0) {
        perror("nanosleep");
      }
    }
    int64_t after = getclock();
    delay[ii] = after - before;
    before = after;
  }

  // loop and print
  for (int ii = 0; ii < 1000; ii++) {
    cout << delay[ii] << endl;
  }

  return 0;
}

