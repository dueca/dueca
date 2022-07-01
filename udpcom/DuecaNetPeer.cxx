/* ------------------------------------------------------------------   */
/*      item            : DuecaNetPeer.cxx
        made by         : Rene' van Paassen
        date            : 171225
        category        : body file
        description     :
        changes         : 171225 first version
        language        : C++
*/

#define DuecaNetPeer_cxx

#include "DuecaNetPeer.hxx"
#include "UDPPeerConfig.hxx"
#include <arpa/inet.h>
#include <algorithm>
#include <dueca/ObjectManager.hxx>
#include <dueca/Ticker.hxx>
#include <dueca/Packer.hxx>
#include <dueca/Unpacker.hxx>
#include <dueca/FillPacker.hxx>
#include <dueca/FillUnpacker.hxx>
#include <dueca/ParameterTable.hxx>
#include <dueca/Environment.hxx>
//#define D_NET
//#define I_NET
#include <dueca/debug.h>
#include "NetCommunicatorExceptions.hxx"

#define DO_INSTANTIATE
#include <dueca/VarProbe.hxx>
#include <dueca/MemberCall.hxx>
#include <dueca/MemberCall2Way.hxx>
#include <dueca/Callback.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;

int DuecaNetPeer::sequence = 0;

const ParameterTable* DuecaNetPeer::getParameterTable()
{
  static const ParameterTable table[] = {
    // communication with DUECA system
    { "packer", new MemberCall2Way<_ThisClass_,ScriptCreatable>
      (&_ThisClass_::setPacker),
      "Packer that assembles and compacts to-be-transported data." },
    { "unpacker", new MemberCall2Way<_ThisClass_,ScriptCreatable>
      (&_ThisClass_::setUnpacker),
      "Unpacker that extracts and distributed data coming in." },
    { "fill-packer", new MemberCall2Way<_ThisClass_,ScriptCreatable>
      (&_ThisClass_::setFillPacker),
      "Packer that compacts low-priority (possibly bulk sized) data." },
    { "fill-unpacker", new MemberCall2Way<_ThisClass_,ScriptCreatable>
      (&_ThisClass_::setFillUnpacker),
      "Unpacker that extracts low-priority data." },

    // specific for UDP connections
    { "port-reuse", new VarProbe<_ThisClass_,bool>
      (&_ThisClass_::port_re_use),
      "Enable port re-use, only necessary in specific configurations where\n"
      "multiple DUECA nodes run on one physical computer and use UDP comm." },
    { "lowdelay", new VarProbe<_ThisClass_,bool>
      (&_ThisClass_::lowdelay),
      "Set lowdelay TOS on the sent packets. Default true."},
    { "socket-priority", new VarProbe<_ThisClass_,int>
      (&_ThisClass_::socket_priority),
      "Set socket priority on send socket. Default 6. Suggestion\n"
      "6, or 7 with root access / CAP_NET_ADMIN capability, -1 to disable." },

    { "if-address", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::interface_address),
      "IP address of the interface to use here. It is imperative to specify\n"
      "this when the computer has multiple options for Ethernet connection."},

    { "timeout", new VarProbe<_ThisClass_,double>
      (&_ThisClass_::timeout),
      "Timeout value [s], by default a high (2.0s) value is used, and the\n"
      "timeout setting is generally not critical for a peer." },

    { "config-url", new MemberCall<_ThisClass_,std::string>
      (&_ThisClass_::setMasterUrl),
      "URL of the configuration connection. Must be Websocket (start with ws\n"
      "includes port, and path, e.g., \"ws://myhost:8888/config\"" },
    { "override-data-url", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::override_data_url),
      "Option to override the data url sent by the master, in case network\n"
      "port translation is applied." },
    { "config-buffer-size", new VarProbe<_ThisClass_,uint32_t>
      (&_ThisClass_::config_buffer_size),
      "Configuration buffer size. This is the buffer used for initial\n"
      "connection to the master. The default (1024) is usually correct."},

    // priority and timing
    { "set-priority", new VarProbe<_ThisClass_,PrioritySpec>
      (&_ThisClass_::priority),
      "Priority for communication. Note no other activities can use this\n"
      "priority level on a peer.\n"},

    { "set-timing", new MemberCall<_ThisClass_,TimeSpec>
      (&_ThisClass_::setTimeSpec),
      "Time interval, needed when not running multi-threaded." },

    { NULL, NULL,
      "DUECA net communicator server, peer. Will connect to a server port\n"
      "on the setup-port specified. Then waits for instructions to connect\n"
      "data link and establishes a communication over UDP; multicast,\n"
      "broadcast or point-to-point, depending on the address configured in\n"
      "the server. Alternatively a websocket connection can be used." }
  };
  return table;
}

DuecaNetPeer::DuecaNetPeer() :
  Accessor(NameSet("dueca", "DuecaNetPeer",
                   1000*ObjectManager::single()->getLocation() +
                   sequence++), control_size, control_size),
  NetCommunicatorPeer(),
  priority(0, 0),
  fill_minimum(max(uint32_t(32), buffer_size/8)),
  commanded_stop(false),
  clock(),
  cb(this, &_ThisClass_::runIO),
  net_io(getId(), "net transport", &cb, priority)
{
  NetCommunicator::node_id = int(Accessor::getId().getLocationId());
}

bool DuecaNetPeer::complete()
{
  // this creates the server socket
  bool res = Accessor::complete();

  // tell the clock to expect sync messages
  Ticker::single()->noImplicitSync();

  // switch on
  net_io.changePriority(priority);
  net_io.setTrigger(clock);
  net_io.switchOn(TimeSpec(0,0));

  time_spec.forceAdvance(SimTime::now()+Ticker::single()->
                         getCompatibleIncrement());
  clock.requestAlarm(time_spec.getValidityStart());

  return res;
}

DuecaNetPeer::~DuecaNetPeer()
{
  //
}

bool DuecaNetPeer::setTimeSpec(const TimeSpec& ts)
{
  time_spec = ts;
  return true;
}

void DuecaNetPeer::returnBuffer(MessageBuffer::ptr_type buffer)
{
  data_comm->returnBuffer(buffer);
}

void DuecaNetPeer::runIO(const TimeSpec& ts)
{
  if (Environment::getInstance()->runningMultiThread()) {
    /* DUECA network.

       Information that this peer is starting cyclic communication.
    */
    I_NET("cyclic start " << ts);
    setStopTime(MAX_TIMETICK);
    startCyclic(net_io);
  }
  else {
    DEB("one cycle " << ts);
    try {
      oneCycle(net_io);
    }
    catch(const connectionfails& e) {
      /* DUECA network.

         Tried to run a communication cycle, but could not make the
         websockets connection for start up. Will attempt to connect
         later. */
      W_NET("Will attempt new connection later");
    }

    // when not over to cyclic, but still stopping
    if (commanded_stop) {
      clearConnections();
      return;
    }
    time_spec.advance();
    clock.requestAlarm(time_spec.getValidityStart());
  }
}

void DuecaNetPeer::prepareToStop()
{
  /* DUECA network.

     Information that this peer is ending cyclic communication.
  */
  I_NET(getId() << " stopping communication");
  commanded_stop = true;
  setStopTime(0);
}

void DuecaNetPeer::clientSendConfig()
{
  //
}

void DuecaNetPeer::clientSendWelcome()
{
  static const UDPPeerConfig clientmark(UDPPeerConfig::ClientPayload);
  uint32_t nodeid = ObjectManager::single()->getLocation();
  uint32_t num_nodes = ObjectManager::single()->getNoOfNodes();
  // std::string name(inet_ntoa(host_address));

  const size_t confsize = 512;
  char buffer[confsize];
  AmorphStore s(buffer, confsize);
  ::packData(s, clientmark);
  ::packData(s, nodeid);
  ::packData(s, num_nodes);
  ::packData(s, interface_address);
  sendConfig(s);
}

void DuecaNetPeer::clientPackPayload(MessageBuffer::ptr_type buffer)
{
  assert(buffer->fill == control_size); // initial fill with control bytes

  // store for packing regular data
  AmorphStore store(buffer->buffer, buffer->capacity);
  store.setSize(control_size);

  // mark for "regular" message size
  StoreMark<uint32_t> regularsize = store.createMark(uint32_t());

  // fill up
  packer->packWork(store);

  // mark regular size and update buffer fill
  uint32_t breg = store.finishMark<uint32_t>(regularsize);
  buffer->fill += breg;

  // any significant room left for fill?
  if (fill_packer /* &&
                     buffer->capacity - buffer->fill > fill_minimum */) {
    // fill_packer->packWork();

    buffer->fill +=
      fill_packer->stuffMessage(&(buffer->buffer[buffer->fill]),
                                buffer->capacity - buffer->fill, buffer);
  }
  DEB("pack o=" << control_size + 4 <<
      " r=" << breg - 4 << " f=" << buffer->fill << " cycle=" << (buffer->message_cycle >> 4));

}

void DuecaNetPeer::clientIsConnected()
{
  DEB("DUECA Net peer initialising with ID " << peer_id);
  Accessor::input_packet_size = buffer_size;

  unpacker->initialiseStores();
  if (fill_unpacker) {
    fill_unpacker->initialiseStores
      (peer_id, ObjectManager::single()->getNoOfNodes());
  }
}

void DuecaNetPeer::clientDecodeConfig(AmorphReStore& s)
{
  // expecting:
  // - send order
  try {
    __attribute__((unused)) uint32_t order(s);
    // TODO: this is not the peer_id/send order, that has been configured
    // already.
    // peer_id = order;
    s.unPackData(group_magic);
    /* DUECA network.

       Information on the assigned send order in the data cycle. */
    //I_NET("Send order " << order << " received");
    //DEB("This peer was assigned send order " << order);
  }
  catch (const dueca::AmorphReStoreEmpty &e) {
    /* DUECA network.

       Failure to correctly decode the configuration sent by the
       master on start. Check that the DUECA versions are equal.
    */
    E_NET("failed to get client config from master");
    throw(e);
  }
}

void DuecaNetPeer::
clientUnpackPayload(MessageBuffer::ptr_type buffer, unsigned id,
                    TimeTickType current_tick, TimeTickType peertick,
                    int usecoffset)

{
  AmorphReStore store(buffer->buffer, buffer->fill);
  store.setIndex(control_size);

  if (id == 0U) {

    // master payload, estimated offset for arrival time
    // int32_t usecoffset(store);
    //if (Environment::getInstance()->runningMultiThread()) {
    Ticker::single()->dataFromMaster(peertick, usecoffset);
    //}
    //else {
    //DEB("time here " << current_tick << " at master " << peertick <<
    //    " offset [usecs] " << usecoffset);
    //}
    //buffer->offset = control_size + sizeof(usecoffset) + sizeof(uint32_t);
  }
  buffer->offset = control_size + sizeof(uint32_t);

  uint32_t regularsize(store);
  buffer->regular = regularsize;
  DEB1("unpack, tick " << peertick << " o=" << buffer->offset <<
       " r=" << buffer->regular << " f=" << buffer->fill);
  unpacker->acceptBuffer(buffer, current_tick);
  if (fill_unpacker && regularsize + buffer->offset < buffer->fill) {
    fill_unpacker->acceptBuffer(buffer, current_tick);
  }

  data_comm->returnBuffer(buffer);
}

DUECA_NS_END;
