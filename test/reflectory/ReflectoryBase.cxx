/* ------------------------------------------------------------------   */
/*      item            : ReflectoryBase.cxx
        made by         : Rene' van Paassen
        date            : 160928
        category        : body file
        description     :
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ReflectoryBase_cxx
#include "ReflectoryBase.ixx"
#include "ReflectoryViewBase.hxx"
#include "TimeSpec.hxx"

#define DO_INSTANTIATE
#include "AsyncList.hxx"

DUECA_NS_START;

void intrusive_ptr_add_ref(const ReflectoryParent* t)
{ t->intrusive_refcount++; }

void intrusive_ptr_release(const ReflectoryParent* t)
{ if (--(t->intrusive_refcount) == 0) { delete t; } }

ReflectoryParent::ReflectoryParent() :
  intrusive_refcount(0U)
{ }
ReflectoryParent::~ReflectoryParent()
{ }

template class ReflectoryBase<dueca::TimeTickType>;

DUECA_NS_END;
