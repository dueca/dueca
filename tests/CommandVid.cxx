#include <iostream>
using namespace std;
extern "C" {
#define KEEP_INLINE
#define HARD_LXRT
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include <rtai_fifos_lxrt_user.h>
#include <rtai_fifos_lxrt.h>
}
#include <cstdio>
#include <sys/time.h>
#include <pthread.h>
#include <sys/mman.h>

#define USE_LXRT_FIFO

// conclusion: rt_task is thread specific! Waiting etc. needs an
// rt_task per thread.
const unsigned int fifonum = 9;

int main()
{
  int fullcnt = 0;
  unsigned long varwait_name = nam2num("inivw");

  RT_TASK *rttask = rt_task_init(varwait_name, 1, 0, 0);
  if (rttask == NULL) {
    cerr << "cannot init task" << endl;

    RT_TASK *rttask2 = reinterpret_cast<RT_TASK*>(rt_get_adr(varwait_name));
    rt_task_delete(rttask2);

    exit(1);
  }

  //mlockall(MCL_CURRENT | MCL_FUTURE);


#ifdef USE_LXRT_FIFO
  int fres = rtf_create(fifonum, sizeof(RTIME));
  if (fres < 0) {
    cout << "Error fifo create " << fres << endl;
    rt_task_delete(rttask);
  }
  //rtf_reset(fifonum);
#else
  // create fifo
  int fif = open("/dev/rtf9", O_WRONLY| O_NONBLOCK);
#endif

  rt_set_oneshot_mode();
  start_rt_timer(0);

  rt_sleep(nano2count(10000000));

  RTIME period = nano2count(int(1.0/85.03*1000000000.0));

  // get the time now
  RTIME the_time = rt_get_time();

  rt_make_hard_real_time();

  for (int ii = 5000; ii--; ) {

    // to wait for
    the_time += period;

    // write to FIFO
#ifdef USE_LXRT_FIFO
    int wres = rtf_put(fifonum, &the_time, sizeof(RTIME));
    if (wres == 0) {
      fullcnt++;
    }
    else if (wres < 0) {
      cout << "fifo write failure" << wres << endl;
      rtf_destroy(fifonum);
      rt_task_delete(rttask);
      return -1;
    }
#else
    int wres = write(fif, &the_time, sizeof(RTIME));
    if (wres < 0 && errno != EWOULDBLOCK) {
      cout << "fifo write failure" << wres << endl;
      rtf_destroy(fifonum);
      rt_task_delete(rttask);
      return -1;
    }
#endif

    // wait for next
    rt_sleep_until(the_time);
  }

  rt_make_soft_real_time();

  rtf_destroy(fifonum);
  stop_rt_timer();
  rt_task_delete(rttask);

  cout << "fifo was " << fullcnt << " times full" << endl;
  return 0;
}

