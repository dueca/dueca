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
  sigset_t wait_set;

  // prepare the wait set
  sigemptyset(&wait_set);
  sigaddset(&wait_set, SIGALRM);

  // going to use the alarm signal for delivery. Block it first
  if (sigprocmask(SIG_BLOCK, &wait_set, NULL) == -1) {
    perror("Problem masking signals");
  }

  // 10 ms, good for 100 Hz clock
  int usecs_in_dt = 10000;

  // or whatever the user gave us
  if (argc >= 2) {
    usecs_in_dt = atoi(argv[1]);
  }

  // switch on the SIGALRM generation
  itimerval itv = {{0, usecs_in_dt},{0, usecs_in_dt}};
  if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
    perror("Ticker:: setitimer failure");
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
  int sig;
  sigwait(&wait_set, &sig);
  int64_t before = getclock();
  if (sig != SIGALRM) cout << "wrong signal " << sig << endl;

  // loop and wait
  for (int ii = 0; ii < 1000; ii++ ) {

    sigwait(&wait_set, &sig);
    int64_t after = getclock();
    if (sig != SIGALRM) cout << "wrong signal " << sig << endl;
    delay[ii] = after - before;
    before = after;
  }

  // loop and print
  for (int ii = 0; ii < 1000; ii++) {
    cout << delay[ii] << endl;
  }

  return 0;
}

