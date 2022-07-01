/* ------------------------------------------------------------------   */
/*      item            : PrioritySpec.hh
        made by         : Rene' van Paassen
        date            : 990805
        category        : header file
        description     :
        changes         : 990805 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef PrioritySpec_hh
#define PrioritySpec_hh

#ifdef PrioritySpec_cc
#endif

#include "ScriptCreatable.hxx"
#include <iostream>

#include <dueca_ns.h>
DUECA_NS_START
struct ParameterTable;

/** Priority specification. DUECA usually runs with several
    threads. A PrioritySpec defines in which thread an activity will
    run, and how important the activity is relative to other
    activities in that same thread. Priority 0 is the administrative
    priority, this is also the priority that runs any (if present)
    graphics libraries. The highest priority is configurable in the
    dueca.cnf file.

    A PrioritySpec is normally used to specify the priority of a
    module. It can be constructed from scheme with the following call:

    \verbinclude priority-spec.scm

    Where prio is the priority, and thus the selection for a thread,
    and order specifies the relative importance within that
    thread. The higher the order, the more important the activity with
    respect to other activities in the same thread.
*/
class PrioritySpec:
  public ScriptCreatable
{
  /** Defines which ActivityManager should handle activities with this
      specification. */
  int priority;

  /** Defines the importance relative to other PrioritySpecs with the
      same priority. The higher the better. */
  int order;

public:
  /** For printing error messages */
  static const char* classname;

  /** Constructor. */
  PrioritySpec();

  /** Complete constructor.
      \param   priority  selection of priority (thread) the activity
               will run in. Priority 0 is the lowest priority, the
               highest is user-configurable.
      \param   order     relative importance to other activities in
               the same thread or priority. This may be a negative
               number. Higher means more important. At the same time,
               activities become more important as they are
               older. With currently selected parameters, a difference
               of 100 in order is equal to 1 integer time step. */
  PrioritySpec(int priority, int order);

  /** Complete method, called after constructor and supply of
      parameters, has to check the validity of the parameters. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Obtain a pointer to the parameter table. */
  static const ParameterTable* getParameterTable();

  /** Destructor. */
  virtual ~PrioritySpec();

  /** This class is scheme callable, this macro defines all necessary
      Scheme stuff. */
  SCM_FEATURES_DEF;

  /** Return the priority. */
  inline int getPriority() const {return priority;}

  /** Return the order. */
  inline int getOrder() const {return order;}

  /** Print to stream, for debugging. */
  friend std::ostream& operator << (std::ostream& os, const PrioritySpec&);
};

DUECA_NS_END
#endif
