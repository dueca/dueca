
#include <iostream>
using namespace std;

#ifdef NATIVE
#include <native/task.h>
#include <native/timer.h>
#else
#include <posix/posix.h>
#endif
#include <sys/mman.h>

// conclusion: rt_task is thread specific! Waiting etc. needs an
// rt_task per thread.

inline int64_t getclock()
{
  //timeval tv;
  //gettimeofday(&tv, NULL);
  //return int64_t(tv.tv_sec)*1000000 + tv.tv_usec;
#ifdef NATIVE
  return rt_timer_tsc2ns(rt_timer_tsc())/1000L;
#else
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_nsec / 1000L + tp.tv_sec*1000000L;
#endif
}

int delay[500] = {-1};
int difmax = 0;

volatile bool busy = true;

#ifdef NATIVE
void do_the_wait(void* dum)
#else
void* do_the_wait(void* dum)
#endif
{
  cout << "entered waiting thread" << endl;

#ifdef NATIVE
  int ret = rt_task_set_mode(0, T_PRIMARY, NULL);
  if (ret) {
    cerr << "problem switching to primary" << endl;
    return ;
  }
#endif

  int waittimes[5] = { 2000, 3000, 4000, 5000, 6000 };
  int64_t before = getclock();
  for (int ii = 0; ii < 500; ii++) {

    int itim = ii/100;
#ifdef NATIVE
    rt_task_sleep( waittimes[itim]*1000+400000);
#else
    struct timespec twait = {0, waittimes[itim]*1000+400000};
    nanosleep(&twait, NULL);
#endif

    //if (ii == 0) cout << "first" << endl;
    int64_t after = getclock();
    delay[ii] = after - before;
    int dif =delay[ii] - waittimes[itim] - 400;
    if (dif > difmax) difmax = dif;
    //cout << "delay " << delay[ii] << endl;
    before = after;
  }
#ifdef NATIVE
  rt_task_set_mode(0, 0, &mode_r);
  if (!(mode_r & T_PRIMARY)) {
    cerr << "fell out of primary mode" << endl;
  }
  rt_task_set_mode(T_PRIMARY, 0, &mode_r);
#endif

  cout << "done waiting" << endl;
  busy = false;
#ifndef NATIVE
  return NULL;
#endif
}

int main()
{
  if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
    perror("Cannot memlock program");
  }

#ifdef NATIVE
  bool task_created = false;
  RT_TASK my_task;
  int ret = rt_task_create(&my_task,"my_task",0,50,0);
  if (ret) {
    cerr << "task creation failed, code " << ret << endl;
    goto error;
  }
  task_created = true;

  ret = rt_task_start(&my_task, &do_the_wait, NULL);
  if (ret) {
    cerr << "failed to start task, code " << ret << endl;
    goto error;
  }

  while (busy) sleep(1);
#else
  pthread_t waiter_thread;
  pthread_attr_t waiter_attr;
  sched_param waiter_schedpar = {0};
  waiter_schedpar.sched_priority = 95;
  pthread_attr_init(&waiter_attr);
  pthread_attr_setschedparam(&waiter_attr, &waiter_schedpar);
  pthread_attr_setschedpolicy(&waiter_attr, SCHED_FIFO);
  //  pthread_attr_setname_np(&waiter_attr, "waiter");
  pthread_attr_setinheritsched(&waiter_attr, PTHREAD_EXPLICIT_SCHED);

  pthread_create(&waiter_thread, &waiter_attr, do_the_wait, NULL);

  cout << "created other thread" << endl;

  pthread_join(waiter_thread, NULL);
  cout << "joined threads" << endl;

#endif

  // loop and print
  for (int ii = 0; ii < 500; ii++) {
    cout << delay[ii] << endl;
  }

  cout << "max lat " << difmax << endl;
  return 0;

#ifdef NATIVE
 error:
  if (task_created) {
    rt_task_delete(&my_task);
  }
#endif
}

