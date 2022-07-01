// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : Reflectory.cxx
        made by         : Rene' van Paassen
        date            : 151024
        category        : body file
        description     :
        changes         : 151024 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#ifndef Reflectory_ixx
#define Reflectory_ixx
#include "Reflectory.hxx"

DUECA_NS_START;

template<class DATA, typename TICK>
Reflectory<DATA,TICK>::Reflectory() :
  ReflectoryLocal<TICK>()
{ }

template<class DATA, typename TICK>
Reflectory<DATA,TICK>::
Reflectory(typename ReflectoryBase<TICK>::ref_pointer root,
           const std::string path,
           typename ReflectoryBase<TICK>::child_change& childhandler,
           typename ReflectoryBase<TICK>::node_change& nodehandler) :
  ReflectoryLocal<TICK>(path)
{
  // let the base class now register with root
  this->registerWithParent(root);
};


DUECA_NS_END;

#endif
