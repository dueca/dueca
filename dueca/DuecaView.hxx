/* ------------------------------------------------------------------   */
/*      item            : DuecaView.hh
        made by         : Rene' van Paassen
        date            : 000721
        category        : header file
        description     :
        changes         : 000721 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DuecaView_hh
#define DuecaView_hh

#include <ModuleState.hxx>
#include <string>
#include <dueca_ns.h>
#include <StatusT1.hxx>

DUECA_NS_START

/** Base class for classes that produce visualisation of dueca and/or
    dusime parameters and controls. The base class is there to provide
    a hook for notifications to the visualisation by EntityManager
    et. al. */
class DuecaView
{
protected:
  /** Singleton pointer. The singleton is not created automatically,
      so users, beware. */
  static DuecaView* singleton;

  /** To indicate that whether this is the base class. */
  static bool is_base;

  /** To remember that a client does not want to stop */
  bool please_keep_running;

public:
  /** Constructor. */
  DuecaView(bool derived = false);

  /** Destructor. */
  virtual ~DuecaView();

  /** Obtain the single possible instance of this class. */
  static DuecaView* single();

  /** return a pointer to the list of entity entries. \todo Can this
      go out? */
  virtual void* getEntitiesList() { return NULL; }

  /** Something has changed in the state of entities, do a refresh. */
  virtual void refreshEntitiesView();

  /** Insert a new entity node.
      \param  name      name for the node
      \param  parent    pointer to the node's parent, to be re-cast to
                        the toolkit's objects that represent a parent
      \param  dueca_node dueca node number where the object lives.
      \param  obj       pointer to the object on the Dueca side.
      \returns          A pointer to the node on the toolkit side. */
  virtual void* insertEntityNode(const char* name, void* parent,
                                 int dueca_node, StatusT1* obj);

  /** Something has changed in the state of nodes, do a refresh. */
  virtual void refreshNodesView();

  /** Change in the state of a single node */
  virtual void syncNode(void* nid);

  /** update buttons entity control */
  virtual void updateEntityButtons(const ModuleState& confirmed_state,
                                   const ModuleState& command_state,
                                   bool emergency_flag);

  /** Control the switch-off buttons */
  virtual void requestToKeepRunning(bool keep_running)
  { please_keep_running = keep_running; }

};

DUECA_NS_END
#endif
