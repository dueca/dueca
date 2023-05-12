/* ------------------------------------------------------------------   */
/*      item            : DuecaNetMaster.cxx
        made by         : Rene' van Paassen
        date            : 171225
        category        : body file
        description     :
        changes         : 171225 first version
        language        : C++
*/

#define DuecaNetMaster_cxx
#include "DuecaNetMaster.hxx"

#include <algorithm>
#include <boost/lexical_cast.hpp>

#include <dueca/Environment.hxx>
#include <dueca/ObjectManager.hxx>
#include <dueca/Ticker.hxx>
#include <dueca/Packer.hxx>
#include <dueca/Unpacker.hxx>
#include <dueca/FillPacker.hxx>
#include <dueca/FillUnpacker.hxx>
#include <dueca/ParameterTable.hxx>
#include <dueca/WrapSendEvent.hxx>
//#define D_NET
#define I_NET
#include <dueca/debug.h>
#include <dueca/AmorphStore.hxx>

#include "UDPPeerConfig.hxx"
#include "NetTimingLog.hxx"
#include "NetCapacityLog.hxx"
#include "NetCommunicatorExceptions.hxx"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>

#define DO_INSTANTIATE
#include <dueca/VarProbe.hxx>
#include <dueca/MemberCall.hxx>
#include <dueca/MemberCall2Way.hxx>
#include <dueca/Callback.hxx>
#include <dueca/CallbackWithId.hxx>

#define   DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;

int DuecaNetMaster::sequence = 0;

const ParameterTable* DuecaNetMaster::getParameterTable()
{
  static const ParameterTable table[] = {
    // communication with DUECA system
    { "packer", new MemberCall2Way<_ThisClass_,ScriptCreatable>
      (&_ThisClass_::setPacker),
      "packer that compacts to-be-transported data" },
    { "unpacker", new MemberCall2Way<_ThisClass_,ScriptCreatable>
      (&_ThisClass_::setUnpacker),
      "unpacker that extracts data" },
    { "fill-packer", new MemberCall2Way<_ThisClass_,ScriptCreatable>
      (&_ThisClass_::setFillPacker),
      "packer that compacts low-priority (excess bw) data" },
    { "fill-unpacker", new MemberCall2Way<_ThisClass_,ScriptCreatable>
      (&_ThisClass_::setFillUnpacker),
      "fill-unpacker that extracts low-prio data" },

    // specific for UDP connections
    { "port-reuse", new VarProbe<_ThisClass_,bool>
      (&_ThisClass_::port_re_use),
      "Enable port re-use, only necessary in specific configurations where\n"
      "multiple DUECA nodes run on one physical computer" },
    { "lowdelay", new VarProbe<_ThisClass_,bool>
      (&_ThisClass_::lowdelay),
      "Set lowdelay TOS on the sent packets. Default true."},
    { "socket-priority", new VarProbe<_ThisClass_,int>
      (&_ThisClass_::socket_priority),
      "Set socket priority on send socket. Default 6. Suggestion\n"
      "6, or 7 with root access / CAP_NET_ADMIN capability, -1 to disable." },

    { "if-address", new VarProbe<_ThisClass_,vstring>
      (&_ThisClass_::interface_address),
      "IP address of the interface to use here" },

    // for WebSocket connections, can be used for UDP too
    { "data-url", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::url),
      "URL of the data connection, for both UDP and WebSocket connections\n"
      "UDP example: \"udp://hostname-or-ipaddress:data-port\"\n"
      "WS  example: \"ws://hostname-or-ipaddress:data-port/data\". If you are\n"
      "using websockets for data communication, these must be on the same\n"
      "port as the configuration URL, but at a different endpoint." },

    { "public-data-url", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::public_data_url),
      "Override the information on the data connection, in case clients\n"
      "connect through a firewall with port mapping. Provide a different\n"
      "client-side view of the connection." },

    // configuration URL
    { "config-url", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::config_url),
      "URL of the configuration connection. Must be Websocket (start with ws)\n"
      "includes port, and path, e.g., \"ws://myhost:8888/config\"" },

    // common packet properties
    { "timeout", new VarProbe<_ThisClass_,double>
      (&_ThisClass_::timeout),
      "timeout value [s]" },
    { "packet-size", new VarProbe<_ThisClass_,uint32_t>
      (&_ThisClass_::buffer_size),
      "data packet size" },
    { "n-logpoints", new VarProbe<_ThisClass_,uint32_t>
      (&_ThisClass_::n_logpoints),
      "Number of cycles to assemble for for histogram logs of timing\n"
      "and capacity." },
    { "node-list", new VarProbe<_ThisClass_,std::vector<int> >
      (&_ThisClass_::peer_nodeids),
      "List of nodes to connect" },

    // priority and timing
    { "set-priority", new VarProbe<_ThisClass_,PrioritySpec>
      (&_ThisClass_::priority),
      "Priority for communication" },
    { "set-timing", new VarProbe<_ThisClass_,TimeSpec>
      (&_ThisClass_::time_spec),
      "Time interval" },

    { NULL, NULL,
      "DUECA net communicator server, master. Will open a server port on the\n"
      "setup-port specified. Then waits for the nodes to join, in the\n"
      "specified order, and establishes a communication over UDP; multicast,\n"
      "broadcast or point-to-point, depending on the address specified" }
  };
  return table;
}

DuecaNetMaster::DuecaNetMaster() :
  Accessor(NameSet("dueca", "DuecaNetMaster",
                   1000*ObjectManager::single()->getLocation() +
                   sequence++), control_size, control_size),
  NetCommunicatorMaster(),
  priority(0,0),
  time_spec(0,Ticker::single()->getCompatibleIncrement()),
  fill_minimum(max(uint32_t(32), buffer_size/8)),
  peer_nodeids(),
  metainfo(),
  send_order_counter(1),
  keep_tick(0),
  n_logpoints(0),
  cycle_span(1),
  log_capacity(),
  log_timing(NULL),
  w_logtiming(),
  w_logcapacity(),
  clock(),
  cb(this, &_ThisClass_::runIO),
  cbup(this, &_ThisClass_::whenUp),
  net_io(getId(), "net transport", &cb, priority)
{
  PacketCommunicatorSpecification::peer_id = 0;
  NetCommunicator::node_id = int(Accessor::getId().getLocationId());
  Environment::getInstance()->informWhenUp(&cbup);
}

bool DuecaNetMaster::complete()
{
  // this creates the server socket
  bool res = NetCommunicatorMaster::complete();
  res = res && Accessor::complete();

  if (peer_nodeids.size() == 0) {

    /* Automatic nodeids if not supplied */
    /* DUECA network.

       A node id list defining the order of transmission has not been
       supplied in the configuration. An automatic order will now be
       created.
    */
    I_NET("Automatic send id / node id list");
    for (int ii = 0; ii < ObjectManager::single()->getNoOfNodes(); ii++) {
      if (ii != ObjectManager::single()->getLocation()) {
        peer_nodeids.push_back(ii);
      }
    }
  }
  else {

    /* Check nodeids given */
    std::vector<int> pcheck = peer_nodeids;
    pcheck.push_back(ObjectManager::single()->getLocation());
    std::sort(pcheck.begin(), pcheck.end());
    bool ordered = true;
    for (size_t nn = 0; nn < pcheck.size(); nn++) {
      if (pcheck[nn] != int(nn)) ordered = false;
    }
    if (!ordered ||
        int(pcheck.size()) != ObjectManager::single()->getNoOfNodes()) {
      /* DUECA network.

         The number of peer ID's need to match the number of nodes
         configured. Adjust your configuration file.
      */
      E_CNF("send order list incorrect");
      return false;
    }
  }

  unpacker->initialiseStores();
  if (fill_unpacker) {
    fill_unpacker->initialiseStores
      (0, ObjectManager::single()->getNoOfNodes());
  }
  Accessor::input_packet_size = buffer_size;

  // switch on
  clock.changePeriodAndOffset(time_spec);
  ts_interval = time_spec.getValiditySpan();
  net_io.changePriority(priority);
  net_io.setTrigger(clock);
  net_io.switchOn(time_spec);

  cycle_span = int64_t(time_spec.getDtInSeconds()*1e6);

  // read 4 random bytes
  int fr = open("/dev/random", O_RDONLY);
  if (fr < 0) {
    /* DUECA network.

       Could not open /dev/random to get a random group magic number,
       beware of old DUECA processess potentially polluting communication.
    */
    W_MOD("Could not open /dev/random, using a non-random group id");
  }

  ssize_t nread = read(fr, &group_magic, sizeof(group_magic));
  if (nread != sizeof(group_magic)) {
    /* DUECA network.

       Could not read /dev/random to get a random group magic number,
       beware of old DUECA processess potentially polluting communication.
    */
    W_MOD("Could not read /dev/random, using a non-random group id");
  }

  return res;
}

void DuecaNetMaster::whenUp(const TimeSpec &ts)
{
  if (n_logpoints) {
    // label has following information:
    // - no log points
    // - time interval
    // - send buffer capacity
    std::stringstream ldata;
    ldata << n_logpoints << " " << time_spec.getDtInSeconds()
          << " " << buffer_size;
    auto *cbv = new CallbackWithId<_ThisClass_,const std::string>
      (this, &_ThisClass_::cbValid, "timing");
    w_logtiming.reset(new ChannelWriteToken
                      (getId(), NameSet("NetCommLog://dueca"),
                       NetTimingLog::classname, ldata.str(),
                       Channel::Events, Channel::OneOrMoreEntries,
                       Channel::MixedPacking, Channel::Bulk, cbv));
    log_timing = new NetTimingLog();
    w_logcapacity.resize(ObjectManager::single()->getNoOfNodes());
    log_capacity.resize(ObjectManager::single()->getNoOfNodes());
    auto *cbv2  = new CallbackWithId<_ThisClass_,const std::string>
      (this, &_ThisClass_::cbValid, "capacity");
    for (int ii = 0; ii < ObjectManager::single()->getNoOfNodes(); ii++) {
      w_logcapacity[ii] = new ChannelWriteToken
        (getId(), NameSet("dueca", "NetCommLog", ""),
         NetCapacityLog::classname,
         std::string("net use node ") + boost::lexical_cast<std::string>(ii),
         Channel::Events,
         Channel::OneOrMoreEntries, Channel::MixedPacking,
         Channel::Bulk, cbv2);
      if (ii) {
        log_capacity[ii] = new NetCapacityLog(peer_nodeids[ii-1]);
      }
      else {
        log_capacity[ii] = new NetCapacityLog
          (ObjectManager::single()->getLocation());
      }
    }
  }
}

void DuecaNetMaster::cbValid(const TimeSpec& ts, const std::string& name)
{
  DEB("Token valid for " << name);
  /* DUECA network.

     Information on validating the channel token for sending timing
     and capacity data.
  */
  W_MOD("Validated write token for net timing/capacity data " << name);
}

DuecaNetMaster::~DuecaNetMaster()
{
  //
}

void DuecaNetMaster::returnBuffer(MessageBuffer::ptr_type buffer)
{
  data_comm->returnBuffer(buffer);
}

void DuecaNetMaster::runIO(const TimeSpec& ts)
{
  // quick return if there is a build-up of activations
  if (net_io.noScheduledBehind()) return;
  DEB2("master run for time " << ts);
  keep_tick = ts.getValidityStart();
  doCycle(ts, net_io);
  DEB2("master run done");
}

DuecaNetMaster::PeerMeta::
PeerMeta(uint32_t nodeid, const std::string& name, uint32_t sendorder) :
  nodeid(nodeid),
  send_order(sendorder),
  name(name)
{
  //
}

void DuecaNetMaster::clientDecodeConfig(AmorphReStore& s, unsigned peer_id)
{
  // expecting:
  // - DUECA node id
  // - number of configured nodes (as a check)
  // - host name or address
  try {
    uint32_t nodeid(s);
    uint32_t num_nodes(s);
    std::string hostname(s);

    if (num_nodes != uint32_t(ObjectManager::single()->getNoOfNodes())) {
      /* DUECA network.

         A peer node claims configuration for a different number of
         DUECA nodes. Please match your dueca_cnf.py (or dueca.cnf)
         files. */
      E_NET("peer " << hostname << " node " << nodeid <<
            " has wrong number of nodes configured.");
      throw(configconnectionbroken());
    }

    // find send order for this node
    unsigned sendorder = 0;
    for (size_t ii = 0; ii < peer_nodeids.size(); ii++) {
      if (peer_nodeids[ii] == int(nodeid) ) {
        sendorder = ii + 1;
      }
    }
    if (!sendorder) {
      /* DUECA network.

         A configuration request has been received for a peer that is
         not included in the send list. The ID may have already been
         used. Check your configuration files, check that you are not
         starting mis-configured peers.
      */
      W_NET("peer " << hostname << " node " << nodeid <<
            " not configured, in send order list.");
      throw(configconnectionbroken());
    }

    // store info on the peer
    metainfo[peer_id] = PeerMeta(nodeid, hostname, sendorder);
    DEB("metainfo, id=" << peer_id << " node=" << nodeid <<
        " host=" << hostname << " sendorder=" << sendorder);
  }
  catch (const dueca::AmorphReStoreEmpty &e) {
    /* DUECA network.

       Failed to decode a client's configuration message. Check that
       DUECA versions used are compatible. */
    E_NET("failed to get client config from peer with send order "
          << peer_id);
    throw(e);
  }
}

void DuecaNetMaster::clientSendConfig(const TimeSpec& ts, unsigned id)
{
  //
}

DuecaNetMaster::VettingResult DuecaNetMaster::
clientAuthorizePeer(CommPeer& peer, const TimeSpec& ts)
{
  if (metainfo.size() == peer_nodeids.size()) {
    if (metainfo.find(peer.send_id) == metainfo.end()) {
      /* DUECA network.

         Rejecting a peer. */
      W_NET("rejecting peer with node id " << metainfo[peer.send_id].nodeid <<
            " have no cycle spot " << peer.send_id);
      return Reject;
    }
    if (metainfo[peer.send_id].send_order == send_order_counter) {
      /* DUECA network.

         Accepting a peer. */
      I_NET("accepting peer with node id " << metainfo[peer.send_id].nodeid <<
	    ", send_id " << peer.send_id);
      send_order_counter++;
      return Accept;
    }
  }
  DEB("delaying acceptance of peer with node id " <<
      metainfo[peer.send_id].nodeid);
  return Delay;
}

template<class D>
D limit(D low, D val, D hig)
{ return low < val ? val : ( hig > val ? hig : val); }

void DuecaNetMaster::clientPackPayload(MessageBuffer::ptr_type buffer)
{
  assert(buffer->fill == control_size); // initial fill 18

  AmorphStore store(buffer->buffer, buffer->capacity);
  store.setSize(control_size);

  // mark for time offset
  //StoreMark<int32_t> usecsoffset = store.createMark(int32_t());
  //buffer->fill += sizeof(int32_t);

  // mark for "regular" message size
  StoreMark<uint32_t> regularsize = store.createMark(uint32_t());

  uint32_t breg = 0;

  // only add data if all peers present
  if (npeers == metainfo.size()) {

    // fill up
    packer->packWork(store);

    // mark regular size and update buffer fill
    breg = store.finishMark<uint32_t>(regularsize);
    buffer->fill += breg;

    // any significant room left for fill?
    if (fill_packer /* &&
                       buffer->capacity - buffer->fill > fill_minimum*/) {
      //fill_packer->packWork();

      buffer->fill +=
        fill_packer->stuffMessage(&(buffer->buffer[buffer->fill]),
                                  buffer->capacity - buffer->fill, buffer);
    }
  }
  else {
    // mark regular size and update buffer fill
    breg = store.finishMark<uint32_t>(regularsize);
    buffer->fill += breg;
  }
  DEB1("pack o=" << control_size + 8 <<
       " r=" << breg - 4 << " f=" << buffer->fill);
#if 0
  int64_t usecpast = Ticker::single()->getUsecsSinceTick(keep_tick);
  int32_t usecpred =
    int32_t(limit(double(numeric_limits<int32_t>::min()),
                  double(usecpast +
                         net_permessage +
                         net_perbyte * buffer->fill),
                  double(numeric_limits<int32_t>::max()) ));
  DEB("tick " << keep_tick << " calc " << usecpast << "+" << net_permessage <<
        "+" << net_perbyte << "*" << buffer->fill << "=" <<
        usecpast + net_permessage +
        net_perbyte * buffer->fill);
  store.finishMark<int32_t>(usecsoffset, usecpred);
#endif

  if (w_logtiming) {
    log_capacity[0]->histoLog(breg, buffer->fill, buffer->capacity);
  }
}

void DuecaNetMaster::
clientUnpackPayload(MessageBuffer::ptr_type buffer, unsigned id,
                    TimeTickType current_tick, TimeTickType peertick,
                    int usecs_offset)

{
  // create unpacking store, set to skip the control data
  AmorphReStore store(buffer->buffer, buffer->fill);
  store.setIndex(control_size);

  uint32_t regularsize(store);
  buffer->regular = regularsize;
  buffer->offset = control_size + sizeof(uint32_t);

  DEB("unpack, tick " << peertick << " o=" << buffer->offset <<
        " r=" << buffer->regular << " f=" << buffer->fill);
  unpacker->acceptBuffer(buffer, current_tick);
  if (fill_unpacker && regularsize + buffer->offset < buffer->fill) {
    fill_unpacker->acceptBuffer(buffer, current_tick);
  }

  // last one, log cycle duration
  if (w_logtiming){
    log_capacity[id]->histoLog(regularsize, buffer->fill, buffer->capacity);
    if (id == npeers) {
      int64_t cycle_tx = Ticker::single()->getUsecsSinceTick(current_tick);
      log_timing->histoLog(cycle_tx, cycle_span);

      if (log_timing->n_points == n_logpoints) {
        log_timing->net_permessage = net_permessage;
        log_timing->net_perbyte = net_perbyte;
        swapLogs(current_tick);
      }
    }
  }
  data_comm->returnBuffer(buffer);
}

void DuecaNetMaster::swapLogs(TimeTickType current_tick)
{
  wrapSendEvent(*w_logtiming, log_timing, current_tick);
  log_timing = new NetTimingLog();
  for (unsigned ii = log_capacity.size(); --ii; ) {
    wrapSendEvent(*w_logcapacity[ii], log_capacity[ii], current_tick);
    log_capacity[ii] = new NetCapacityLog(peer_nodeids[ii-1]);
  }
  wrapSendEvent(*w_logcapacity[0], log_capacity[0], current_tick);
  log_capacity[0] = new NetCapacityLog(ObjectManager::single()->getLocation());
}

void DuecaNetMaster::prepareToStop()
{
  /* DUECA network.

     Information on planned stop of the communication. */
  I_NET(getId() << " stopping communication");
  net_io.switchOff(TimeSpec(keep_tick + 5*time_spec.getValiditySpan()));
  NetCommunicatorMaster::breakCommunication();
}

void DuecaNetMaster::clientWelcomeConfig(AmorphStore& s, unsigned peer_id)
{
  if (peer_id == 0) return;
  static const UDPPeerConfig clientmark(UDPPeerConfig::ClientPayload);
  ::packData(s, clientmark);
  s.packData(metainfo[peer_id].send_order);
  s.packData(group_magic);
}

DUECA_NS_END;
