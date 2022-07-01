/* ------------------------------------------------------------------   */
/*      item            : Condition.cxx
        made by         : Rene' van Paassen
        date            : 000209
        category        : body file
        description     :
        changes         : 000209 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Condition_cc
#include "Condition.hxx"
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

#undef SYNC_WITH_XENOMAI
#define USE_POSIX_THREADS

#endif

#else
#ifdef USE_POSIX_THREADS
# include <pthread.h>
#ifndef EBUSY
#define EBUSY 16
#endif
#endif

#endif

#include <dassert.h>

//#define D_ACT
//#define I_ACT
//#define W_ACT
#define E_ACT
#define debug_h
#include <debug-direct.h>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

/** Implementation-dependend data for conditional waiting. */
class ConditionData
{
    bool need_init;

#if defined(USE_POSIX_THREADS) || \
    defined(SYNC_WITH_RTAI) || \
    defined(SYNC_WITH_XENOMAI)

  /** Mutex associated with the condition. */
  pthread_mutex_t mutex;

  /** Condition itself. */
  pthread_cond_t  condition;

  inline void init()
  {
    if (need_init) {
      pthread_condattr_t cattr;
      pthread_condattr_init(&cattr);
      pthread_cond_init(&condition, &cattr);

      // initialise the mutex
      pthread_mutexattr_t mattr;
      pthread_mutexattr_init(&mattr); // defaults to FAST on Linux
      pthread_mutex_init(&mutex, &mattr);
      need_init = false;
    }
  }
#elif defined(USE_WIN_THREADS)
  // combination of ??
#error "Windows threads not yet implemented"
#endif

public:
  ConditionData() : need_init(true) { }

#if defined(USE_POSIX_THREADS) ||                \
    defined(SYNC_WITH_RTAI) || \
    defined(SYNC_WITH_XENOMAI)

  inline void lock()
  {
    init();
    pthread_mutex_lock(&mutex);
  }

  inline void wait() {
    pthread_cond_wait(&condition, &mutex);
  }

  inline void signal() {
    pthread_cond_signal(&condition);
  }

  inline void unlock()
  {
    pthread_mutex_unlock(&mutex);
  }

  inline void cleanup()
  {
    if (!need_init) {
      if (pthread_cond_destroy(&condition) == EBUSY) {
        /* DUECA activity.

           A Condition object was in use at its destruction. */
        W_ACT("condition " << name << " in use at destruction");
      }
      if (pthread_mutex_destroy(&mutex) == EBUSY) {
        /* DUECA activity.

           The mutex for a Condition object was locked at its destruction. */
        W_ACT( "mutex " << name << " locked at destruction");
      }
    }
  }
#endif
};

Condition::Condition(const char* name) :
  my(new ConditionData()),
  name(name)
{
  //  assert(sizeof(buff) >= sizeof(ConditionData));
}

Condition::~Condition()
{
  my->cleanup();
  delete my;
}

void Condition::enterTest()
{
  my->lock();
  DEB( "got mutex for condition " << name);
}

void Condition::wait()
{
  DEB( "start waiting for condition " << name );
  my->wait();
  DEB( "stopped waiting for condition " << name );
}

void Condition::leaveTest()
{
  DEB( "unlocking mutex for condition " << name);
  my->unlock();
}

void Condition::signal()
{
  my->signal();
  DEB("signalled condition " << name);
}
DUECA_NS_END
