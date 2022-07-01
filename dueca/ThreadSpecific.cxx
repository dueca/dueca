/* ------------------------------------------------------------------   */
/*      item            : ThreadSpecific.cxx
        made by         : Rene' van Paassen
        date            : 061121
        category        : body file
        description     :
        changes         : 061121 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ThreadSpecific_cxx
#include "ThreadSpecific.hxx"

#include <dueca-conf.h>

#if defined(SYNC_WITH_RTAI)
#ifdef HAVE_RTAI_LXRT_H
#include <rtai_lxrt.h>
#else
#error RTAI configuration
#endif

#elif defined(SYNC_WITH_XENOMAI)
#ifdef HAVE_POSIX_POSIX_H
#include <posix/posix.h>
#else
#error Xenomai configuration
#endif

#else
#ifdef USE_POSIX_THREADS
# include <pthread.h>
#endif

#endif /* HAVE_RTAI_LXRT_H */

#ifdef __QNXNTO__
#include <stdio.h>
#else
#include <cstdio>
#endif

DUECA_NS_START

/* Keep track of whether this is running multi thread. */
#ifdef LATE_SYNC_PRIMITIVES
static bool local_multi_thread = false;
void ThreadSpecific::toMultiThread()
{ local_multi_thread = true; }
#else
static bool local_multi_thread = true;
void ThreadSpecific::toMultiThread()
{ }
#endif

class ThreadSpecificData
{
  bool need_init;
  void* singlethread_data;

#if defined(USE_POSIX_THREADS) || defined(SYNC_WITH_XENOMAI) || \
  defined(SYNC_WITH_RTAI)
  pthread_key_t key;

  inline void init()
  {
    if (need_init && local_multi_thread) {
      if (pthread_key_create(&key, NULL)) {
        perror("At ThreadSpecific.cxx");
      }
      else {
        pthread_setspecific(key, singlethread_data);
      }
      need_init = false;
    }
  }
#endif

public:
  ThreadSpecificData() :
    need_init(true),
    singlethread_data(NULL) { }

#if defined(USE_POSIX_THREADS)
  inline void cleanup()
  {
    if (!need_init) pthread_key_delete(key);
  }

  inline void set(const void* data)
  {
    init();
    if (need_init) singlethread_data = const_cast<void*>(data);
    else pthread_setspecific(key, data);
  }

  inline void* get()
  {
    if (need_init) {
      return singlethread_data;
    }
    else {
      return pthread_getspecific(key);
    }
  }
#endif
};


ThreadSpecific::ThreadSpecific() :
  my(*new ThreadSpecificData())
{
  //
}

ThreadSpecific::~ThreadSpecific()
{
  my.cleanup();
  delete (&my);
}

void* ThreadSpecific::ptr()
{
  return my.get();
}

void ThreadSpecific::setPtr(const void* newpt)
{
  my.set(newpt);
}
DUECA_NS_END
