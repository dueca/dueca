/* ------------------------------------------------------------------   */
/*      item            : StateGuard.cxx
        made by         : Rene' van Paassen
        date            : 990621
        category        : body file
        description     :
        changes         : 990621 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define StateGuard_cc
#include "StateGuard.hxx"


#include <dueca-conf.h>

#if defined(SYNC_WITH_RTAI)
#ifdef HAVE_RTAI_LXRT_H
#include <rtai_lxrt.h>
extern RT_TASK* rtai_guile_task;
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
# include <errno.h>
#endif

#endif

#include <iostream>
#include <new>
#include <dassert.h>

#ifdef TEST_OPTIONS
#define DEBUG_MUTEX 1
#endif

// include the applicable OS-specific synchronisation stuff
DUECA_NS_START

/** The class StateGuardData keeps the data for the StateGuard in an
    implementation-dependent fashion. */
class StateGuardData
{
  bool need_init;

#if defined(USE_POSIX_THREADS) || \
    defined(SYNC_WITH_RTAI) || \
    defined(SYNC_WITH_XENOMAI)
  pthread_mutex_t guard;

  inline void init()
  {
    if (need_init) {
      pthread_mutexattr_t mattr;
      pthread_mutexattr_init(&mattr); // defaults to FAST on Linux
      pthread_mutex_init(&guard, &mattr);
      need_init = false;
    }
  }
#endif

public:
  StateGuardData() : need_init(true)
  {
#if defined(SYNC_WITH_RTAI)
    if (!rtai_guile_task) {
      if (!(rtai_guile_task = rt_task_init(nam2num("DGUIL"), 1, 0, 0))) {
        cerr << "Cannot init DGUIL rtai task\n";
        exit(1);
      }

      // should allow for non-root user making this rt
      rt_allow_nonroot_hrt();
    }
#endif
}

#if defined(USE_POSIX_THREADS) ||                \
    defined(SYNC_WITH_RTAI) || \
    defined(SYNC_WITH_XENOMAI)
  inline void lock()
  {
    init();
    pthread_mutex_lock(&guard);
  }

  inline void unlock()
  {
    pthread_mutex_unlock(&guard);
  }

  inline bool trylock()
  {
    init();
    return (pthread_mutex_trylock(&guard) != EBUSY);
    return true;
  }

  inline void cleanup()
  {
    if (!need_init) {
      pthread_mutex_destroy(&guard);
    }
  }
#endif
};

StateGuard::StateGuard(const char* name, bool lockit) :
  my(new StateGuardData()),
  name(name)
{
  // lock the mutex
  if (lockit) accessState();
}

StateGuard::~StateGuard()
{
  my->cleanup();
  delete my;
}

#ifndef DEBUG_MUTEX

void StateGuard::accessState() const
{ my->lock();}

void StateGuard::accessState(const char* txt) const
{ my->lock();}

void StateGuard::leaveState() const
{ my->unlock(); }

void StateGuard::leaveState(const char* txt) const
{ my->unlock(); }

bool StateGuard::tryState() const
{ return my->trylock(); }

bool StateGuard::tryState(const char* txt) const
{ return my->trylock(); }

#else

void StateGuard::accessState() const
{
#  if DEBUG_MUTEX == 2
  cerr << "Getting mutex " << name << endl;
#  endif
  if (!my->trylock()) {
    cerr << "Locked mutex (" << name << ") found" << endl;

    // lock did not work, lock it now
    my->lock();
  }
}

void StateGuard::accessState(const char* txt) const
{
#  if DEBUG_MUTEX == 2
  cerr << "Getting mutex \"" << name << "\" at " << txt << endl;
#  endif
  if (!my->trylock()) {
    cerr << "Locked mutex (" << name << ") found at" << txt << endl;

    // lock did not work, lock it now
    my->lock();
  }
}

void StateGuard::leaveState() const
{
#  if DEBUG_MUTEX == 2
  cerr << "Releasing mutex " << name << endl;
#  endif
  my->unlock();
}

void StateGuard::leaveState(const char* txt) const
{
#  if DEBUG_MUTEX == 2
  cerr << "Releasing mutex \"" << name << "\" at " << txt << endl;
#  endif
  my->unlock();
}

bool StateGuard::tryState() const
{
#  if DEBUG_MUTEX == 2
  cerr << "Trying mutex " << name << endl;
#  endif
  return my->trylock();
}

bool StateGuard::tryState(const char* txt) const
{
#  if DEBUG_MUTEX == 2
  cerr << "Trying mutex \"" << name << "\" at " << txt << endl;
#  endif
  return my->trylock();
}

#endif // DEBUG_MUTEX
DUECA_NS_END
