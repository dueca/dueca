/* ------------------------------------------------------------------   */
/*      item            : NodeManager.cxx
        made by         : Rene' van Paassen
        date            : 990823
        category        : body file
        description     :
        changes         : 990823 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define NodeManager_cc

#include <dueca-conf.h>
#include "NodeManager.hxx"
#include <ScriptInterpret.hxx>
#include "Ticker.hxx"
#include "NameSet.hxx"
#include "Environment.hxx"
#include "DuecaView.hxx"

// localized development debugging
#define DEBPRINTLEVEL -1
#include "debprint.h"

#define I_SYS
#include <debug.h>

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

#define DO_INSTANTIATE
#include "Event.hxx"
#include "EventAccessToken.hxx"
#include "Callback.hxx"
#include <CriticalActivity.hxx>
#include <WrapSendEvent.hxx>

DUECA_NS_START

NodeManager* NodeManager::singleton = NULL;
unsigned int static_node_id = 0xffffffff;


NodeManager::NodeManager(int this_node, int no_of_nodes) :
  NamedObject(NameSet("dueca", "NodeManager", this_node)),
  this_node(this_node),
  no_of_nodes(no_of_nodes),
  node_state(no_of_nodes, NodeControlMessage::NodeLoose),
  interval(int(1.0 / Ticker::single()->getTimeGranule() + 0.5)),
  slow_query(false),
  t_control(getId(), NameSet("dueca", "NodeUpdates", ""),
            "NodeControlMessage", "", Channel::Events,
            Channel::OneOrMoreEntries, Channel::ReadReservation),
  w_control(getId(), NameSet("dueca", "NodeUpdates", ""),
            "NodeControlMessage", "", Channel::Events,
            Channel::OneOrMoreEntries, Channel::OnlyFullPacking,
            Regular, &cb4, no_of_nodes),
  cb1(this, &NodeManager::processMessages),
  cb2(this, &NodeManager::emitQuery),
  cb3(this, &NodeManager::stopDueca),
  cb4(this, &NodeManager::writeIsValid),
  process_message(getId(), "process node state message", &cb1,
                  PrioritySpec(0, 0)),
  emit_query(getId(), "send node state query", &cb2, PrioritySpec(0, 0)),
  just_stop(getId(), "nodemanager stops dueca", &cb3, PrioritySpec(0, 0)),
  query_alarm()
{
  if (singleton != NULL) {
    cerr << "Creation of second NodeManager attempted!";
    return;
  }
  singleton = this;
  static_node_id = this_node;

  int fast = int(0.2 / Ticker::single()->getTimeGranule() + 0.5);
  query_alarm.changePeriod(fast);

  // specify my activity to be started by the ticker and by
  // receipt of channel information
  process_message.setTrigger(t_control);
  process_message.switchOn(TimeSpec(0,0));
}

void NodeManager::writeIsValid(const TimeSpec& ts)
{
  if (this_node == 0) {
    emit_query.setTrigger(query_alarm);

    // emit_query.setTimeSpec(TimeSpec(0, interval));
    emit_query.switchOn(TimeSpec(0,0));
  }
}

NodeManager::~NodeManager()
{

}

bool NodeManager::isDuecaSynced()
{
  bool is_complete = true;
  for (int ii = no_of_nodes; ii--; ) {
    is_complete &= (node_state[ii] == NodeControlMessage::NodeSynced);
  }
  return is_complete;
}

bool NodeManager::isDuecaComplete()
{
  bool is_complete = true;
  for (int ii = no_of_nodes; ii--; ) {
    is_complete &= (node_state[ii] == NodeControlMessage::NodeJoined)
      || (node_state[ii] == NodeControlMessage::NodeSynced);
  }
  return is_complete;
}

bool NodeManager::isDuecaCompleting()
{
  bool is_completing = true;
  for (int ii = no_of_nodes; ii--; ) {
    is_completing &= (node_state[ii] == NodeControlMessage::NodeJoined)
      || (node_state[ii] == NodeControlMessage::NodeJoining)
      || (node_state[ii] == NodeControlMessage::NodeSynced);
  }
  return is_completing;
}

void NodeManager::emitQuery(const TimeSpec& time)
{
  if (w_control.isValid())
    wrapSendEvent(w_control,
                  new NodeControlMessage(NodeControlMessage::NodeQuery),
                  time.getValidityStart());
}

void NodeManager::stopDueca(const TimeSpec& time)
{
  static bool have_to_stop = true;
  just_stop.switchOff(TimeSpec(0,0));
  if (have_to_stop) {
    /* DUECA system.

       Information on decision to stop the current node.
     */
    I_SYS("stopping node, at time " << time);
    CSE.quit();
    have_to_stop = false;
  }
}

void NodeManager::processMessages(const TimeSpec& time)
{
  bool changed = false;

  // otherwise process messages
  if (!t_control.isValid())
    return;

  //  t_control.getNextEvent(e, TimeSpec::end_of_time);
  DataReader<NodeControlMessage, VirtualJoin>
    d(t_control, TimeSpec::end_of_time);

  DEB("NodeManager, " << d.data() << ' ' << d.origin());
  // const NodeControlMessage *d = e->getEventData();

  DEB("at time " << time << " ncm " << d.data() << ' ' << d.origin());

  // process
  switch(d.data().state) {
  case NodeControlMessage::NodeLoose:
  case NodeControlMessage::NodeJoined:
  case NodeControlMessage::NodeJoining:
  case NodeControlMessage::NodeSynced:
  case NodeControlMessage::NodeBreaking:
    if (d.origin().getLocationId() > no_of_nodes) {
      /* DUECA system.

         Received a node control message that pretends to be from a
         node that is not configured in this DUECA process. May
         indicate a communication failure.
       */
      W_SYS("Got a message from out of bounds node "
            << int(d.origin().getLocationId()));
    }
    else {
      changed = (node_state[d.origin().getLocationId()] != d.data().state);
      node_state[d.origin().getLocationId()] = d.data().state;
#ifdef I_SYS_ACTIVE
      if (changed && this_node == 0) {
        /* DUECA system.

           Information on a state change for a specific node.
         */
        I_SYS("Node " << int(d.origin().getLocationId()) <<
              " changed to state " << d.data().state);
      }
#endif
      if (this_node == 0) {
        DuecaView::single()->refreshNodesView();
      }
    }
    break;

  case NodeControlMessage::NodeQuery:
    if (!w_control.isValid()) break;
    if (isDuecaComplete() && Ticker::single()->isSynced()) {
      wrapSendEvent(w_control,
                    new NodeControlMessage(NodeControlMessage::NodeSynced),
                    time.getValidityStart());
      DEB1("replying NodeSynced");
    } else if (isDuecaCompleting()) {
      wrapSendEvent(w_control,
                    new NodeControlMessage(NodeControlMessage::NodeJoined),
                    time.getValidityStart());
      DEB1("replying NodeJoined");
    }
    else {
      wrapSendEvent(w_control,
                    new NodeControlMessage(NodeControlMessage::NodeJoining),
                    time.getValidityStart());
      DEB1("replying NodeJoining");
    }
    break;

  case NodeControlMessage::NodeBreak:
    // this is the command to break up the system.
    // start an activity, ticker activated, that will stop dueca just
    // a little bit later (and at the same time as all other nodes)

    // confirm we are "loose"
    wrapSendEvent(w_control,
                  new NodeControlMessage(NodeControlMessage::NodeBreaking),
                  time.getValidityStart());

    just_stop.setTrigger(*Ticker::single());
    just_stop.switchOn(time);
    /* DUECA system.

       Information on a scheduled node stop.
     */
    I_SYS("scheduling node stop at " << time);
    break;

  case NodeControlMessage::Emergency:
    /* DUECA system.

       Received a critical error message, setting critical error for
       the activities.
     */
    W_SYS("Got emergency message, setting critical error");
    CriticalActivity::criticalErrorNodeWide();
    break;

  default:
    /* DUECA system.

       Received a node control message that could not be
       interpreted. Indicates a communication failure or programming
       error in DUECA.
     */
    W_SYS("Node control message not understood");
  }

  // slow down query when we are complete
  if (this_node == 0 && !slow_query && isDuecaComplete()) {
    /* DUECA system.

       All DUECA nodes in this process have joined. Slowing down the
       node query to save bandwidth, but keep monitoring of node
       status.
     */
    I_SYS("slowing down node query at time " << time);
    slow_query = true;
    query_alarm.changePeriod(interval);
    //    emit_query.setTimeSpec
    //  (PeriodicTimeSpec(time.getValidityEnd(), interval));
  }
}

void NodeManager::breakUp()
{
  TimeTickType stop_time = SimTime::getTimeTick() +
    int(3.0 / Ticker::single()->getTimeGranule() + 0.5);

  ScriptInterpret::single()->writeQuit();
  wrapSendEvent(w_control,
                new NodeControlMessage(NodeControlMessage::NodeBreak),
                stop_time);
}

void NodeManager::emergency()
{
  if (this_node == 0) {
    wrapSendEvent(w_control,
                  new NodeControlMessage(NodeControlMessage::Emergency),
                  SimTime::getTimeTick());
    CriticalActivity::criticalErrorNodeWide();
  }
}

NodeControlMessage::NodeState NodeManager::getNodeState(int i)
{
  if (i >= no_of_nodes || i < 0)
    return NodeControlMessage::NodeLoose;
  return node_state[i];
}

const vstring NodeManager::getNodeName(query_iterator i) const
{
#ifdef HAVE_SSTREAM
  ostringstream st;
  st << "node " << i << std::ends;
  return vstring(st.str());
#else
  char cbuf[10]; ostrstream st(cbuf, 10);
  st << "node " << i << '\000';
  return vstring(cbuf);
#endif
}

const char* const NodeManager::getNodeStatus(query_iterator i) const
{
  return getString(node_state[i]);
}

DUECA_NS_END






