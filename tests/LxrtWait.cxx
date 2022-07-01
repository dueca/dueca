#include <iostream>
using namespace std;
#include <rtai_lxrt.h>
#include <cstdio>
#include <sys/time.h>
//#include <pthread.h>

// conclusion: rt_task is thread specific! Waiting etc. needs an
// rt_task per thread.

inline int64_t getclock()
{
  //timeval tv;
  //gettimeofday(&tv, NULL);
  //return int64_t(tv.tv_sec)*1000000 + tv.tv_usec;

  return rt_get_time_ns() / 1000;
}

int delay[500] = {-1};
RT_TASK *rttask2;

void* do_the_wait(void* dum)
{
  cout << "entered waiting thread" << endl;
  unsigned long varwait_name = nam2num("runvw");
  rttask2 = rt_task_init(varwait_name, 1, 0, 0);
  if (rttask2 == NULL) {
    cerr << "cannot init task" << endl;
    // clean up?
    exit(1);
  }

  rt_make_hard_real_time();
  int waittimes[5] = { 20000, 30000, 40000, 50000, 600000 };
  int64_t before = getclock();
  for (int ii = 0; ii < 500; ii++) {

    int itim = ii/100;
    rt_sleep(nano2count(waittimes[itim]*1000+400000));
    if (ii == 0) cout << "first" << endl;
    int64_t after = getclock();
    delay[ii] = after - before;
    before = after;
  }
  rt_make_soft_real_time();
  rt_task_delete(rttask2);

  return NULL;
}

int main()
{

  unsigned long varwait_name = nam2num("inivw");
  pthread_t waiter_thread;

  RT_TASK *rttask = rt_task_init(varwait_name, 1, 0, 0);
  if (rttask == NULL) {
    cerr << "cannot init task" << endl;
    // clean up?
    exit(1);
  }

  cout << "task pointer = " << hex << reinterpret_cast<void*>(rttask)
       << dec << endl;

  rt_set_oneshot_mode();
  start_rt_timer(0);

  rt_sleep(nano2count(10000000));

  pthread_create(&waiter_thread, NULL, do_the_wait, NULL);
  cout << "created other thread" << endl;

  pthread_join(waiter_thread, NULL);
  cout << "joined threads" << endl;

  stop_rt_timer();

  rt_task_delete(rttask);

  // loop and print
  for (int ii = 0; ii < 500; ii++) {
    cout << delay[ii] << endl;
  }

  return 0;
}

