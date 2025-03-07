/* ------------------------------------------------------------------   */
/*      item            : NodeManager.hh
        made by         : Rene' van Paassen
        date            : 990823
        category        : header file
        description     :
        changes         : 990823 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NodeManager_hh
#define NodeManager_hh

#ifdef NodeManager_cc
#endif

#include "NamedObject.hxx"
#include "NodeControlMessage.hxx"
#include "PeriodicAlarm.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include <dueca/ChannelReadToken.hxx>
#include <dueca/ChannelWriteToken.hxx>
#include <vector>
using namespace std;
#include <dueca_ns.h>
DUECA_NS_START

extern unsigned int static_node_id;

/** DUECA main object that coordinates state with other nodes. */
class NodeManager: public NamedObject
{
  /** The node id of the current nodes. Node id's start at zero, and
      are positive. Node id 0 will be the administrative node. */
  int this_node;

  /** The total number of nodes in the DUECA process. */
  int no_of_nodes;

  /** We keep a tab on the state of each node. */
  vector<NodeControlMessage::NodeState> node_state;

  /** The interval (in integer time) with which other nodes are
      interrogated by node 0. */
  int interval;

  /** Flag to indicate that we have slowed down the interrogation. */
  bool slow_query;

  /** Counter to once-in-a-while force node status display */
  unsigned once_in_a_while;

  /** Read token for the channel with node control messages. */
  ChannelReadToken                      t_control;

  /** Write token for that same channel with node control messages. */
  ChannelWriteToken                     w_control;

  /** Callback objects \{ */
  Callback<NodeManager>                 cb1, cb2, cb3, cb4; /// \}

  /** Activity for reading and processing a query message. */
  ActivityCallback                      process_message;

  /** Activity for sending out a query, only activated by node 0. */
  ActivityCallback                      emit_query;

  /** Activity for stopping dueca. */
  ActivityCallback                      just_stop;

  /** This object is a singleton. */
  static NodeManager*                   singleton;

  /** Alarm for the query */
  PeriodicAlarm                         query_alarm;

public:

  /** Constructor. This constructor is normally called from Scheme.
      \param this_node    Id of the current nodes
      \param no_of_nodes  Total number of nodes. */
  NodeManager(int this_node, int no_of_nodes);

  /// Destructor
  ~NodeManager();

  /** Returns a reference to the singleton. Note that there is no
      check/creation, we assume that the creation script adheres to
      the proper creation order. */
  inline static NodeManager* single()
  {return singleton;}

  /** Handle the messages coming from (other) nodes. */
  void processMessages(const TimeSpec& time);

  /** Send out messages to inquire about the status of other
      nodes. Only node 0 uses this. */
  void emitQuery(const TimeSpec& time);

  /** Stop dueca running. */
  void stopDueca(const TimeSpec& time);

  /** Token is valid, can start querying. */
  void writeIsValid(const TimeSpec& time);

  /** If complete, all nodes have joined and have
      communication. Service to the other DUECA main objects and
      myself. */
  bool isDuecaComplete();

  /** If completing, this node has communication, but not all nodes
      have been detected/confirmed. Service to the other DUECA main
      objects and myself. */
  bool isDuecaCompleting();

  /** If synced, timing between all nodes has been
      coordinated. Service to the other DUECA main objects and
      myself. */
  bool isDuecaSynced();

  /** Return object type. */
  ObjectType getObjectType() const {return O_Dueca;};

  /** Return number of nodes, as given in configuration script. */
  inline int getNoOfNodes() { return no_of_nodes;}

  /** Return the state of a node. */
  NodeControlMessage::NodeState getNodeState(int i);

  /** Return the id of the current node. */
  inline const int getThisNodeNo() const {return this_node;}

  /** Command to break up communication with other DUECA nodes.
      \todo Does not work yet, use Ctrl-c to stop the programs. */
  void breakUp();

  /** \name Query calls.
      Query of all node information in this node manager.  With
      the following calls, one can start a query of all objects in the
      manager. Normally, the query must be completed "in one go",
      otherwise new information can come in, and a "QueryInterrupted"
      exception will be thrown. */
  //@{
  /** A simple integer suffices as iterator over node information. */
  typedef int query_iterator;

  /** Start a new query of all node information. */
  inline query_iterator startQuery() {return 0;}

  /** Returns true if there is no more information to obtain. */
  inline bool isQueryComplete(query_iterator i)
  {return i >= no_of_nodes;}

  /** Updates the iterator. Returns true if there is more information
      to be obtained, false if at end of query. */
  inline bool goQueryNext(query_iterator& i) {return ++i < no_of_nodes;}

  /** Return the name of a node. */
  const vstring getNodeName(query_iterator i) const;

  /** Return the status of a node. */
  const char* const getNodeStatus(query_iterator i) const;

  /** Upon an emergency, set all critical activities to safe mode. */
  void emergency();
  //@}
};

DUECA_NS_END
#endif

