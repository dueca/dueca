/* ------------------------------------------------------------------   */
/*      item            : ModuleIdList.cxx
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

#include "ModuleIdList.hxx"
#include <iostream>
DUECA_NS_START

ModuleIdList::ModuleIdList() :
  ns(NameSet("", "", "")),
  object(),
  cmp(NULL)
{
  //
}

ModuleIdList::
ModuleIdList(const GlobalId& id,  const NameSet& ns, NamedObject* cmp) :
  ns(ns), object(id), cmp(cmp)
{
  // nothing else
}


ModuleIdList::
~ModuleIdList()
{
  // seems there is nothing to do
}

ModuleIdList::
ModuleIdList(const ModuleIdList& l) :
  ns(l.ns), object(l.object), cmp(l.cmp)
{
  // nothing else
}

ModuleIdList& ModuleIdList::operator = (const ModuleIdList& l)
{
  ns = l.ns;
  object = l.object;
  cmp = l.cmp;
  return *this;
}

std::ostream& operator << (std::ostream& os, const ModuleIdList& il)
{
  return os << "ModuleIdList(ns=" << il.ns << ",object=" << il.object
            << ')';
}
DUECA_NS_END
