/* ------------------------------------------------------------------   */
/*      item            : StatusKeeper.cxx
        made by         : Rene' van Paassen
        date            : 010823
        category        : body file
        description     :
        changes         : 010823 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define StatusKeeper_cxx
#include "StatusKeeper.hxx"

#include <SimulationState.hxx>
#include <ModuleState.hxx>
#include <StatusT1.hxx>
#include <DuecaView.hxx>
DUECA_NS_START

//template<class S, class V>
//StatusKeeper<S,V>* StatusKeeper<S,V>::singleton = NULL;
template<class S, class V>
StatusKeeper<S,V>& StatusKeeper<S,V>::single()
{
  // if (singleton == NULL) singleton = new StatusKeeper<S,V>();
  static StatusKeeper<S,V> singleton;
  return singleton;
}


template<class S, class V>
StatusKeeper<S,V>::StatusKeeper() :
  root(new Summary<ModuleId, S, V >
       (&ModuleId::create(vector<vstring>(), GlobalId()),
        new S() ))
{
  //
}

template<class S, class V>
StatusKeeper<S,V>::~StatusKeeper()
{
  //
}

template<class S, class V>
bool StatusKeeper<S,V>::addNode(const ModuleId& n,
                              const S& s)
{
  return root->insertLinkAndStatus(n, s);
}

template<class S, class V>
Summary<ModuleId, S, V >&
StatusKeeper<S,V>::getTop()
{
  return *root;
}

template<class S, class V>
Summary<ModuleId, S, V >
& StatusKeeper<S,V>::findSummary(const ModuleId& i) const
{
  return root->findSummary(i);
}

template<class S, class V>
bool StatusKeeper<S,V>::existsSummary(const ModuleId& i) const
{
  return root->existsSummary(i);
}

// explicit instantiation for DUECA/DUSIME status check and view
template class StatusKeeper<StatusT1, DuecaView>;

DUECA_NS_END
