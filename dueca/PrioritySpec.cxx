/* ------------------------------------------------------------------   */
/*      item            : PrioritySpec.cxx
        made by         : Rene' van Paassen
        date            : 990805
        category        : body file
        description     :
        changes         : 990805 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define PrioritySpec_cc
#include "PrioritySpec.hxx"
#include "ActivityManager.hxx"
#include <iostream>
#include "dueca_assert.h"
#include "ParameterTable.hxx"
#include "debug.h"
#define DO_INSTANTIATE
#include "VarProbe.hxx"
DUECA_NS_START
#include <debprint.h>

// The parameter table describes the optional parameters that may be
// supplied in a creation script
const ParameterTable* PrioritySpec::getParameterTable()
{
  static const ParameterTable table[] = {
    { "priority", new VarProbe<PrioritySpec,int>
      (REF_MEMBER(&PrioritySpec::priority)),
      "Priority level, >= 0, determines the thread in which, and the\n"
      "ActivityManager by which, an activity is run" },
    { "order", new VarProbe<PrioritySpec,int>
      (REF_MEMBER(&PrioritySpec::order)),
      "Determines relative importance of activities in one prio level\n"
      "a smaller number means to come first, a larger one comes later.\n"
      "Logical time of an activity also plays a role. A difference of 100 in\n"
      "order is equivalent to one time granule." },
    { NULL, NULL,
      "Defines the priority of an Activity" }
  };

  return table;
}

// name for printing purposes
const char* PrioritySpec::classname = "PrioritySpec";

PrioritySpec::PrioritySpec() :
  priority(ActivityManager::getMinPrio()),
  order(0)
{
  //
}

bool PrioritySpec::complete()
{
  if (priority < 0 || priority > ActivityManager::getMaxPrio()) {
    /* DUECA activity.

       Cannot create this priority specification, as specified,
       priority adjusted. */
    W_CNF("Warning, requested non-existing prio level " << priority);
    priority = max(0, min(ActivityManager::getMaxPrio(), priority));
  }
  return true;
}

const char* PrioritySpec::getTypeName()
{
  return "PrioritySpec";
}

PrioritySpec::PrioritySpec(int priority, int order) :
  priority(priority),
  order(order)
{
  complete();
}


PrioritySpec::~PrioritySpec()
{
  //
}

DUECA_NS_END
