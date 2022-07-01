/* ------------------------------------------------------------------   */
/*      item            : StateGuard.hh
        made by         : Rene' van Paassen
        date            : 990621
        category        : header file
        description     :
        changes         : 990621 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef StateGuard_hh
#define StateGuard_hh

#include <stringoptions.h>
#include <dueca_ns.h>
DUECA_NS_START

// forward declarations
class StateGuardData;

/** Class that guards against race conditions. DUECA internal use. */
class StateGuard
{
  /** Reference to the OS-dependent data */
  StateGuardData* my;

  /** Name for the StateGuard, used in debugging mode. */
  vstring name;

public:
  /** Constructor. If lockit is true, it leaves the guard in a locked
      state, use leaveState() to get out of that. */
  StateGuard(const char* name = "anon", bool lockit = true);

  /// Destructor
  ~StateGuard();

  /** Request to enter the state. */
  void accessState() const;

  /** Request to enter the state, with a description, use this form if
      you need more info at debugging. */
  void accessState(const char* txt) const;

  /** Release access to the state again. */
  void leaveState() const;

  /** Release access to the state again, with a description, use this
      form if you need more info at debugging. */
  void leaveState(const char* txt) const;

  /** Try to get access to the state. If free, you get it, otherwise
      this returns false. */
  bool tryState() const;

  /** Try to get access to the state, with a description, use this
      form if you need more info at debugging. */
  bool tryState(const char* txt) const;

  /** Return the name. */
  inline const vstring& getName() {return name;}
};


/** User of the StateGuard class. Locks a StateGuard upon entry, and
    unlocks again upon deletion, i.e. as the thing goes out of
    scope. Example:
    \code
    {
       // somewhere inside a block
       ScopeLock lockthis(my_state_guard);

       // from now on. guard is locked. Access the data


    } // as the block (function, etc.) goes out of scope, the
      // ScopeLock destructor is called and guard is unlocked.
    \endcode
*/
class ScopeLock
{
  /** Maintain a reference to the stateguard. */
  StateGuard &guard;

public:
  /** Constructor, locks the stateguard */
  inline ScopeLock(StateGuard& guard) :
    guard(guard) { guard.accessState(); }

  /** Destructor, unlocks the guard. */
  inline ~ScopeLock()
  { guard.leaveState(); }

private:
  /** copying is not possible. */
  ScopeLock(const ScopeLock& );

  /** nor is assignment. */
  ScopeLock& operator = (const ScopeLock& );

  /** and new is certainly forbidden! */
  static void* operator new(size_t s);
};


DUECA_NS_END
#endif


