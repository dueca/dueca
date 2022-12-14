/* ------------------------------------------------------------------   */
/*      item            : Condition.hh
        made by         : Rene' van Paassen
        date            : 000209
        category        : header file
        description     :
        api             : DUECA_API
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

/** Wait and resume object */
class Condition
{
  /** Reference to the OS-dependent data */
  ConditionData* my;

  /** Name of the condition, used for more debugging info. */
  vstring name;

public:
  /** Constructor.

      @param name   Identifying name.
  */
  Condition(const char* name);

  /** Destructor. */
  ~Condition();

  /** Enter the condition test.

      Call this before checking to see whether waiting is needed, or call it
      when changing the conditions, and signalling a resume.
   */
  void enterTest();

  /** Start waiting, if it appears that the test was false, call this
      after an enterTest.  */
  void wait();

  /** Leave the condition test, after enterTest or wait. */
  void leaveTest();

  /** Signal the condition to any waiters. */
  void signal();
};

DUECA_NS_END
#endif



