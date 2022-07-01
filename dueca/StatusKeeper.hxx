/* ------------------------------------------------------------------   */
/*      item            : StatusKeeper.hxx
        made by         : Rene van Paassen
        date            : 010823
        category        : header file
        description     :
        changes         : 010823 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef StatusKeeper_hxx
#define StatusKeeper_hxx

#ifdef StatusKeeper_cxx
#endif

#include <memory>
#include <Summary.hxx>
#include <ModuleId.hxx>
#include <dueca_ns.h>
DUECA_NS_START

// forward definition
class DuecaView;

/** A class that keeps detailed status of modules and the Dusime class
    modules (if dusime enabled) among them. This is only used in the
    node 0 dueca process. It builds a tree-shaped representation of
    all entities, sub-entities and of the modules at the leaves, with
    a status proxy at each node. The status of the leaves is updated
    with status reports (both for the dusime and dueca parts, and
    there is expansion for other status bits and pieces. Status
    queries at the root call result in recursive updates. */
template<class S, class V = DuecaView>
class StatusKeeper
{
  /** The default status object, a composite object. */
  S default_stat;

  /** The root status object. */
  Summary<ModuleId, S, V > *root;

  /** Constructor. */
  StatusKeeper();

public:

  /** Destructor. */
  ~StatusKeeper();

  /** Singleton accessor. */
  static StatusKeeper& single();
  // { if (singleton == NULL) singleton = new StatusKeeper();
  //return *singleton;}

  /** Add a node to the keeper's tree. */
  bool addNode(const ModuleId& n, const S & s);

  /** Get access to the top node. */
  Summary<ModuleId, S, V >& getTop();

  /** Get acces to any other node, with an id as search object. */
  Summary<ModuleId, S, V >&
  findSummary(const ModuleId& i) const;

  /** Check the existence of a particular summary. */
  bool existsSummary(const ModuleId& i) const;
};



DUECA_NS_END
#endif
