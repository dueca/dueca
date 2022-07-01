/* ------------------------------------------------------------------   */
/*      item            : SchemeObject.cxx
        made by         : Rene' van Paassen
        date            : 990707
        category        : body file
        description     :
        changes         : 990707 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define SchemeObject_cc
#include "SchemeObject.hxx"
#include <dueca/ScriptCreatable.hxx>
#include <dueca/ModuleCreator.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

SchemeObject::SchemeObject(ScriptCreatable* r) :
  self(),
  referred(SCM_EOL),
  object(r)
{
  DEB("creating scheme object, object=" <<
      reinterpret_cast<void*>(object.get()) <<
      " (" << r->getTypeName() << ")");
}

SchemeObject::SchemeObject(ModuleCreator* r) :
  self(),
  referred(scm_list_n(SCM_UNDEFINED)),
  module(r)
{
  DEB("creating scheme object, module=" <<
      reinterpret_cast<void*>(module.get()) <<
      " (" << r->getType() << ")");
}


SchemeObject::~SchemeObject()
{
  if (object.get()) {
    DEB("destructing scheme object, SCM=" << self <<
        " object=" << reinterpret_cast<void*>(object.get()) );
  }
  else {
    DEB("destructing scheme object, SCM=" << self <<
        " module=" << reinterpret_cast<void*>(module.get()) );
  }
}

// preferred format now
/*void SchemeObject::addReferred(SchemeObject* r)
{
  referred.push_back(r->getSCM());
  }*/

// old style
void SchemeObject::addReferred(SCM r)
{
  //referred.push_back(r);
  if (object.get()) {
    DEB("Object " << self << " (" << object->getTypeName() <<
        ") adding referred " << r);
  }
  else {
    DEB("Module " << self << " (" << module->getType() <<
        ") adding referred " << r);
  }
  referred = scm_cons(r, referred);
  //scm_append_x(referred, r)
}

void SchemeObject::setSCM(SCM obj)
{
  self = obj;
  DEB("Setting SCM=" << self);
}

SCM SchemeObject::getSCM() const
{
  return self;
}

void SchemeObject::markReferred()
{
  if (object.get()) {
    DEB("Object " << self << " (" << object->getTypeName() <<
        ") mark cycle");
  }
  else {
    DEB("Module " << self << " (" << module->getType() <<
        ") mark cycle");
  }
  if (referred == SCM_EOL) return;
  DEB("Marking referred");
  scm_gc_mark (referred);
  DEB("Marking referred done");
  /*  for (list<SCM>::iterator ii = referred.begin();
       ii != referred.end(); ii++) {
    scm_gc_mark (*ii);
    }*/
}

DUECA_NS_END
