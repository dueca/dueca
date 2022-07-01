/* ------------------------------------------------------------------   */
/*      item            : ThreadSpecific.hxx
        made by         : Rene van Paassen
        date            : 061121
        category        : header file
        description     :
        changes         : 061121 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ThreadSpecific_hxx
#define ThreadSpecific_hxx

#ifdef ThreadSpecific_cxx
#endif

#include <dueca_ns.h>
DUECA_NS_START

class ThreadSpecificData;

class ThreadSpecific
{
  /** Reference to OS-dependent data. */
  ThreadSpecificData& my;

public:
  /** Constructor. */
  ThreadSpecific();

  /** Destructor. */
  ~ThreadSpecific();

/** Function to be called before starting additional threads.

    Some environments need special initialisation before threading
    functions can be used. The StateGuard encapsulation of mutexes or
    the like can wait before actually using the mutexes while still in
    single thread mode. Call this function *before* starting
    multi-thread, and *after* initialisation of your task
    environment or other initialisation needed for multi-threading. */
  static void toMultiThread();

  /** Access the pointer. */
  void* ptr();

  /** Change the pointer. */
  void setPtr(const void* newpt);
};

DUECA_NS_END
#endif
