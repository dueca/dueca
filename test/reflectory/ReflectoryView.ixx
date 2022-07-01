// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : ReflectoryView.cxx
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

#ifndef ReflectoryView_ixx
#define ReflectoryView_ixx
#include "ReflectoryView.hxx"

DUECA_NS_START;


template <class DATA, typename TICK>
ReflectoryView<DATA,TICK>::ReflectoryView
(typename ReflectoryBase<TICK>::ref_pointer root,
 const std::string& path,
 data_change& cb_change) :
  ReflectoryViewBase<TICK>(root, path),
  cb_change(cb_change)
{
  //
}

template <class DATA, typename TICK>
ReflectoryView<DATA,TICK>::~ReflectoryView()
{
  //
}

DUECA_NS_END;
#endif
