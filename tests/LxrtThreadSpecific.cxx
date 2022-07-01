/* ------------------------------------------------------------------   */
/*      item            : LxrtThreadSpecific.cxx
        made by         : Rene' van Paassen
        date            : 081216
        category        : body file
        description     :
        changes         : 081216 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define LxrtThreadSpecific_cxx
#include <rtai_lxrt.h>
#include <iostream>
#include <error.h>
#include <cstdio>

using namespace std;

// shared among threads
pthread_key_t key;
union joiner {
  long intval;
  void* ptrval;
};
pthread_mutex_t mutex;
pthread_mutex_t mutex2;
pthread_mutexattr_t attrib;


void *second_thread(void* dum)
{
  cout << "entered second thread" << endl;

  unsigned long rtainame = nam2num("run2");
  RT_TASK *rttask = rt_task_init(rtainame, 1, 0, 0);
  if (rttask == NULL) {
    cerr << "cannot init 2nd task" << endl;
    exit(1);
  }
  rt_make_hard_real_time();

  // wait until soft task ready
  pthread_mutex_lock(&mutex);

  // set my key stuff
  joiner val; val.intval = 2;
  pthread_setspecific(key, val.ptrval);

  usleep(200000);
  pthread_mutex_unlock(&mutex);

  // check the key
  joiner check; check.ptrval = NULL;
  check.ptrval = pthread_getspecific(key);
  cout << "Check in second thread " << check.intval << endl;

  rt_make_soft_real_time();
  rt_task_delete(rttask);

  return NULL;
}


int main()
{
  unsigned long rtainame = nam2num("main");
  pthread_t other_thread;

  RT_TASK *rttask = rt_task_init(rtainame, 1, 0, 0);
  if (rttask == NULL) {
    cerr << "cannot init main task" << endl;
    exit(1);
  }

  // early mutex init? Is apparently OK.
  pthread_mutexattr_init(&attrib);
  pthread_mutex_init(&mutex, &attrib);
  pthread_mutex_lock(&mutex);

  if (pthread_create(&other_thread, NULL, second_thread, NULL)) {
    perror("creating other thread");
    exit(1);
  }
  cout << "created other thread" << endl;

  if (pthread_key_create(&key, NULL)) {
    perror("in main");
  }

  joiner val; val.intval = 1;
  pthread_setspecific(key, val.ptrval);

  usleep(200000);

  cout << "second mutex" << endl;
  if (pthread_mutex_init(&mutex2, &attrib)) {
    perror("second mutex");
  }
  pthread_mutex_lock(&mutex2);
  pthread_mutex_unlock(&mutex);

  joiner check; check.ptrval = NULL;
  check.ptrval = pthread_getspecific(key);
  cout << "Check in main thread " << check.intval << endl;

  if (pthread_mutex_destroy(&mutex)) {
    perror("trying to destroy mutex");
  }

  pthread_mutex_unlock(&mutex2);
  rt_task_delete(rttask);

  if (pthread_join(other_thread, NULL)) {
    perror("Trying to join");
  }

  return 0;
}

