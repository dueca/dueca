/* ------------------------------------------------------------------   */
/*      item            : Condition.hh
        made by         : Rene' van Paassen
        date            : 000209
        category        : header file
        description     :
        changes         : 000209 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Condition_hh
#define Condition_hh

#include <stringoptions.h>

#include <dueca_ns.h>
DUECA_NS_START
class ConditionData;

/** Encapsulation of the pthread_cond functionality of posix threads. */
class Condition
{
  /** Reference to the OS-dependent data */
  ConditionData* my;

  /** Name of the condition, used for more debugging info. */
  vstring name;

public:
  /** Constructor. */
  Condition(const char* name);

  /** Destructor. */
  ~Condition();

  /** Enter the condition test. */
  void enterTest();

  /** Start waiting, if it appears that the test was false. */
  void wait();

  /** Leave the condition test. */
  void leaveTest();

  /** Signal the condition to the waiters. */
  void signal();
};

DUECA_NS_END
#endif



