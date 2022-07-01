/* ------------------------------------------------------------------   */
/*      item            : ReflectoryBase.hxx
        made by         : Rene van Paassen
        date            : 160928
        category        : header file
        description     : Base class for distributed configuration nodes
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectoryBase_hxx
#define ReflectoryBase_hxx

#include <boost/function.hpp>
#include <limits>
#include <AsyncList.hxx>
#include <map>
#include <vector>
#include <dueca_ns.h>
#include <boost/intrusive_ptr.hpp>
#include <dueca/SharedPtrTemplates.hxx>
#include "ReflectoryData.hxx"
#include "ReflectoryExceptions.hxx"

DUECA_NS_START;


/** Advance definition */
template<typename TICK> class ReflectoryViewBase;
class ReflectoryHandler;

/** Id type */
typedef uint16_t reflectory_id;

class ReflectoryParent
{
protected:
  /** Reference count */
  mutable unsigned intrusive_refcount;
  friend void intrusive_ptr_add_ref(const ReflectoryParent* t);
  friend void intrusive_ptr_release(const ReflectoryParent* t);
public:
  ReflectoryParent();
  virtual ~ReflectoryParent();
};

/** Base class for the nodes of a distributed configuration tree

    @tparam TICK  counter type, to express the configuration generations
 */
template<typename TICK>
class ReflectoryBase: public ReflectoryParent
{
  /** access to the master handler. */
  ReflectoryHandler *base;

public:
  unsigned getNodeId() const {return 0U;}

public:
  typedef TICK ticktype;
  typedef typename boost::intrusive_ptr<ReflectoryBase<TICK> > ref_pointer;
  typedef ReflectoryBase<TICK>* pointer;
  typedef const ReflectoryBase<TICK>* const_pointer;
  typedef ReflectoryBase<TICK> parenttype;

protected:
  /** State logic */
  enum State {
    Created,         /**< Initial state */
    WaitConfirm,     /**< Waiting until confirmed to join as slave */
    Active,          /**< Node is active and communicating */
    Pruning,         /**< Waiting until all children removed */
    Pruned           /**< Node is no longer relevant. */
  };

protected:
  /** Node state */
  State state;

  /** Node birth cycle */
  TICK birth;

protected:

  /** Timed data */
  struct DataChange {
    /** Time at which data becomes valid */
    TICK target_time;

    /** Data to accept */
    const void* value;

    /** Constructor

        @param val    new value
        @param tt     validity time
    */
    DataChange(const void* val = NULL, const TICK& tt = 0) :
      target_time(tt), value(val) { }
  };

  /** List of future configuration changes */
  mutable dueca::AsyncList<ReflectoryData> planned_config;

  /** list of future data changes */
  mutable dueca::AsyncList<DataChange>     planned_value;

  /** client ref */
  typedef typename ReflectoryViewBase<TICK>::const_ref_pointer client_type;

  /** List of view clients */
  mutable std::list<client_type> clients;

  /** Waiting clients; name and ref to the viewbase */
  struct WaitingClient {

    /** name */
    std::string path;

    /** pointer to the client */
    client_type client;

    WaitingClient(client_type client,
                  std::string path) :
      path(path), client(client) { }
  };

  /** List of waiting view clients, for these clients no data/leaf is
      present yet. */
  mutable std::list<WaitingClient> waiting_clients;

  /** Current value of the data, in anonymised form. */
  mutable const void* current;

  /** Time for the next planned action */
  mutable TICK action_time;

  /** Numeric id of self */
  reflectory_id selfid;

  /** Name of this node */
  std::string name;

  /** Also keep path, avoid reconstructing it */
  std::string path;

  /** Point to the parent */
  boost::intrusive_ptr<const ReflectoryBase> parent;

  /** Keep the children of this node accessed in two ways; this is the
      first, a map, mapping item names to the items. */
  // mutable std::map<std::string,boost::intrusive_ptr<const ReflectoryBase> > slots_map;

  typedef typename std::vector<boost::intrusive_ptr<ReflectoryBase> > childvec_type;

  /** Each child will be numbered, and keep quick access in a vector. */
  mutable childvec_type slots;

public:
  /** Constructor for a root */
  ReflectoryBase();

protected:
  /** Constructor */
  ReflectoryBase(const std::string& path);

  /** register this reflectory with the parent, can only be done at end of
      child's constructor.

      This can result in two typical outcomes; immediate registration
      if the parent is local to this node, or a delayed registration if
      the parent is local to another copy.

      @param root   Pointer to the reflectory root. */
  virtual void registerWithParent(ref_pointer root);
public:

  /** Destructor */
  virtual ~ReflectoryBase();
  // add a client

  // add a client
  void addView(const ReflectoryViewBase<TICK>* client) const;

  // reconstruct the path?
  const std::string& getPath() const {return path;}

  // data class
  virtual const char* getDataClass() const
  { throw(reflectory_nodata()); }

  /** Add a client for a node that is not present yet */
  void addWaitingClient(typename ReflectoryViewBase<TICK>::ref_pointer client,
                        const std::string& path) {
    waiting_clients.push_back(WaitingClient(client, path));
  }

  // remove again from the list of viewing clients
  void removeView(const ReflectoryViewBase<TICK>* client) const
  { remove(clients.begin(), clients.end(), client); }

  /** Access the data for reading */
  const void* data(TICK tick) const
  {
    while (planned_value.notEmpty() &&
           tick >= planned_value.front().target_time) {
      current = planned_value.front().value;
      planned_value.pop();
    }
    if (!current) throw(reflectory_nodata());
    return current;
  }

  /** Insert new data in the list
      @param newdata     New data point
      @param target_time When is this valid */
  void newData(const void* newdata,
               TICK target_time)
  {
    planned_value.push_back(DataChange(newdata, target_time));
  }
public:

  /** Define a type for callback functions */
  typedef typename
  boost::function<void (ReflectoryData::ItemState,unsigned)> node_change;

  /** For now, the same type for change function?? */
  typedef typename
  boost::function<void (ReflectoryData::ItemState,unsigned)> child_change;

  /** Root of the reflectory is a singleton */
  static boost::intrusive_ptr<const ReflectoryBase> root(void);

  /** get a reflectory to read */
  boost::intrusive_ptr<const ReflectoryBase> operator [] (unsigned idx) const;

  /** get a reflectory by its name */
  boost::intrusive_ptr<const ReflectoryBase>
  operator [] (const std::string& name) const;

protected:

  void passConfigChange(ReflectoryData& d) const;

  /** Utility function, provide the default response to a child

      - Issues an id and accepts the child if the name has not yet been taken
      - Rejects if the name has been taken, or some limit reached

      @param istate
   */
  void defaultResponseChild(const ReflectoryData::ItemState& istate,
                            const std::string& childname,
                            unsigned id, boost::intrusive_ptr<const ReflectoryBase> child);

  /** Add a child to the list of children

      Checks whether the child name is free, then adds the child.
      Throws an exception if child name is already taken.
      The acceptance of the child is issued.

      The implementation depends on whether the reflectory node is
      local or remote.

      @param child   reference to the new child. */
  virtual reflectory_id addChild(ref_pointer child) const = 0;

public:
  /** Add a join request for replication.

      @param nid     node that needs the extension. */
  virtual void extendJoinRequest(unsigned nid) = 0;

  /** Get this node's name */
  const std::string& getName() const { return name; }

  /** Get the ID chain
      @param ancestry  get n-th parent id instead. */
  reflectory_id getId(unsigned ancestry=0);

  /** Actions for a handling child requests */
  void acceptChild(boost::intrusive_ptr<const ReflectoryBase> parent,
                   unsigned assigned_id);

public:
  /** Function to process upcoming configuration and data changes.

      @param tick    Cycle/time for which to process updates
      @returns       Earliest next cycle.
   */
  virtual TICK update(const TICK& tick, unsigned nodeid) = 0;
};

DUECA_NS_END;

#endif
