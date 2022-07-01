/* ------------------------------------------------------------------   */
/*      item            : ScriptCreatable.cxx
        made by         : Rene' van Paassen
        date            : 030508
        category        : body file
        description     :
        changes         : 030508 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ScriptCreatable_cxx
#include "ScriptCreatable.hxx"

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

CODE_REFCOUNT(ScriptCreatable)

ScriptCreatable::ScriptCreatable() :
  INIT_REFCOUNT_COMMA
  cstate(Initial)
{
  DEB("ScriptCreatable constructor for " << getTypeName());
}

ScriptCreatable::ScriptCreatable(const ScriptCreatable& o) :
  INIT_REFCOUNT_COMMA
  cstate(o.cstate)
{
  DEB("ScriptCreatable copy constructor for " << getTypeName());
}

ScriptCreatable& ScriptCreatable::operator = (const ScriptCreatable& o)
{
  DEB("ScriptCreatable assignment for " << getTypeName());
  if (this != &o) {
    this->cstate = o.cstate;
  }
  return *this;
}

ScriptCreatable::~ScriptCreatable()
{
  DEB("ScriptCreatable destructor for " << getTypeName());
}

bool ScriptCreatable::complete()
{
  return true;
}

const char* ScriptCreatable::getTypeName()
{
  return "ScriptCreatable";
}

const ParameterTable* ScriptCreatable::getParameterTable()
{
  return NULL;
}

void ObsoleteObject::addReferred(unsigned dum) {}
unsigned ObsoleteObject::getSCM() { return 0U; }

bool ScriptCreatable::checkComplete()
{
  if (cstate == Initial) {
    if (complete()) {
      cstate = Completed;
    }
    else {
      cstate = CompletionError;
    }
  }
  return cstate == Completed;
}

template<> const char* core_creator_name<ScriptCreatable>(const char*)
{ return "ScriptCreatable"; }

DUECA_NS_END
