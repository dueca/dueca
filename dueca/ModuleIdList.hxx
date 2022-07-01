/* ------------------------------------------------------------------   */
/*      item            : ModuleIdList.hh
        made by         : Rene' van Paassen
        date            : 980319
        category        : header file
        description     :
        notes           : object for keeping Module data in the registry.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ModuleIdList_hh
#define ModuleIdList_hh

#include "GlobalId.hxx"
#include "NameSet.hxx"
#include <iostream>

#include <dueca_ns.h>
DUECA_NS_START
class NamedObject;
// class names

/** This class defines a set of data needed for keeping an inventory
    of Modules in each participating host. */
class ModuleIdList
{
private:
  /** Name given to this module. */
  NameSet ns;

  /** A global id pointing to the object. */
  GlobalId object;

  /** Pointer to the object itself. */
  NamedObject* cmp;

public:
  /** Empty constructor. */
  ModuleIdList();

  /** creates the Module entry. */
  ModuleIdList(const GlobalId& id, const NameSet& ns, NamedObject* cmp);

  /** destructor. */
  ~ModuleIdList();

  /** copy constructor. */
  ModuleIdList(const ModuleIdList& l);

  /** assigment operator. */
  ModuleIdList& operator = (const ModuleIdList& l);

public:
  /** get the GlobalId data out. */
  const GlobalId& getGlobalId() const {return object;}

  /** Returns a reference to the name. */
  inline const NameSet& getNameSet() const {return ns;}

  /** Get the pointer to the module. */
  inline const NamedObject* getObject() const {return cmp;}

  /** comparison, needed for the set. */
  bool operator < (const ModuleIdList& s2) const
    { return ns < s2.ns; }

  /** comparison, needed for the set. */
  bool operator > (const ModuleIdList& s2) const
    { return ns > s2.ns; }
  /** comparison, needed for the set. */
  bool operator == (const ModuleIdList& s2) const
    { return ns == s2.ns; }
  /** comparison, needed for the set. */
  bool operator != (const ModuleIdList& s2) const
    { return ns != s2.ns; }

  /** Print to a stream. */
  friend std::ostream& operator << (std::ostream& os, const ModuleIdList& il);
};

DUECA_NS_END
#endif



