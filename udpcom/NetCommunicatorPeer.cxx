/* ------------------------------------------------------------------   */
/*      item            : NetCommunicatorPeer.cxx
        made by         : Rene' van Paassen
        date            : 170912
        category        : body file
        description     :
        changes         : 170912 first version
        language        : C++
        copyright       : (c) 17 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define NetCommunicatorPeer_cxx

#include <exception>
#include <iomanip>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>

#include <boost/lexical_cast.hpp>
#include <boost/swap.hpp>

#define I_NET
#include <dueca-conf.h>
#include <debug.h>
#include <dassert.h>

#include "NetCommunicatorPeer.hxx"
#include "NetCommunicatorExceptions.hxx"
#include "UDPPeerInfo.hxx"
#include "UDPPeerConfig.hxx"

#define DEBPRINTLEVEL 1
#include <debprint.h>

#ifdef BUILD_TESTOPT
const static double test_failprob = 0.0001;
#endif

// constructor
NetCommunicatorPeer::NetCommunicatorPeer() :
  NetCommunicator(),
  master_address(""),
  master_url(""),
  override_data_url(""),
  conf_comm(),
  commbuf(),
  follow_id(0xffff),
  last_cycle(0),
  stop_commanded(false),
  follow_changes(3, "Peer to follow changes"),
  connection(false),
  trackingudpcycle(false),
  current_tick(0),
  i_nodeid(uint16_t(0xffff)),
  lastround_npeers(0),
  myturntosend(false)
{
  PacketCommunicatorSpecification::callback =
    common_callback(this, &NetCommunicatorPeer::unpackPeerData);
}

// destructor
NetCommunicatorPeer::~NetCommunicatorPeer()
{
  //
}



/*
   Do a blocking, timed read on the config socket, assemble in
   commbuf, and return the number of new bytes
*/
unsigned NetCommunicatorPeer::readConfigSocket(bool wait)
{
  if (wait) {
    auto res = conf_comm->receive();
    return res.second;
  }
  return conf_comm->checkup();
}

void NetCommunicatorPeer::receiveConfigMessage(MessageBuffer::ptr_type& buffer)
{
  commbuf.write(buffer->buffer, buffer->fill);
  conf_comm->returnBuffer(buffer);
}


void NetCommunicatorPeer::sendConfig(AmorphStore&s)
{
  unsigned filllevel = s.getSize();

  // check that the store has been filled at least somewhat
  if (filllevel == 0) {
    /* DUECA network.

       It is not possible to send configuration information from this
       network peer. Increase the size of the configuration
       buffers.
    */
    E_NET("config stores not big enough for single object");
    throw(dueca::AmorphStoreBoundary());
  }

  conf_comm->sendConfig(s);

  // reset the store
  s.reUse();
}

void NetCommunicatorPeer::setupConnection(Activity& activity)
{
  if (connection) return;

  // need to set up configuration connection
  if (!conf_comm) {

    PacketCommunicatorSpecification spec;
    if (!master_url.size()) {
      //DEB("Initial master url " << master_url);
      master_url = std::string("ws://") + master_address + std::string(":") +
        boost::lexical_cast<std::string>(master_port) + std::string("/config");
      DEB("Constructed master url " << master_url);
    }
    spec.url = master_url;
    spec.buffer_size = config_buffer_size;
    spec.port_re_use = true;
    spec.callback = common_callback
      (this, &NetCommunicatorPeer::receiveConfigMessage);
    spec.timeout = 2.0; // s
    conf_comm.reset(new WebsockCommunicatorPeerConfig(spec));

    if (!conf_comm->isOperational()) {
      conf_comm.reset();
      /* DUECA network.

         It was not possible to create a websocket connection to the
         communication master. Check network address, settings and
         connectivity. */
      W_NET("Cannot get a connection to " << master_url);
      throw(connectionfails());
    }
    /* 2a; send a welcome message? */
    clientSendWelcome();
  }

  /* 4: remainder is all with config instructions, repeat until end
     config flagged */
  while (!decodeConfigData()) {
    activity.logBlockingWait();
    readConfigSocket(true);
    activity.logBlockingWaitOver();
  }
  connection = true;

  // switch to wait for a cycle to jump in
  trackingudpcycle = false;
}

void NetCommunicatorPeer::_oneCycle(Activity& activity)
{
  while (true) {

#ifdef BUILD_TESTOPT
#warning "test options selected, simulated message receive missing added"

    // pretend to miss this single message
    if (int(random()/test_failprob/(RAND_MAX+1.0)) == 0) {

      DEB("Simulating a receive failure cycle " <<
          message_cycle);
      data_comm->setFailReceive();
    }
#endif

    // use select to check for data
    activity.logBlockingWait();
    // receive returns no of bytes and peer, and uses the
    // unpackPeerData callback to pass the data
    const auto result = data_comm->receive();
    activity.logBlockingWaitOver();
    ssize_t nbytes = result.second;
    int i_peer_id = result.first;

    // re-check the time
    current_tick = SimTime::getTimeTick();

    // result
    if (nbytes == 0) {
      // timeout occurred, repeat current data in next cycle
      /* DUECA network.

         Received a timeout on data communication. This may be a
         transient failure, but if this happens frequently, consider
         adjusting the data rate and/or the timeout times, or check
         the network. Note that in general peers can use long
         timeout values; the master is responsible for detecting
         missing peers and missing cycles, and re-initiating the
         communication.
       */
      W_NET("Data receive timeout, cycle " << message_cycle);

      // check up on master instructions, maybe a peer dropped out
      if (readConfigSocket(false) > 0) {
        decodeConfigData();
      }

      return;
    }

    // send data if it is my turn
    if (myturntosend) {

#if DEBPRINTLEVEL >= 1
      static bool firstmessage = true;
      if (firstmessage) {
        DEB("Sending first message, responding to " << follow_id <<
            " cycle " << message_cycle);
        firstmessage = false;
      }
#endif
      myturntosend = false;

      // from NetCommunicator parent
      codeAndSendUDPMessage(current_tick);
    }

    // after nodeid 0's turn, always check up on the configuration
    if (i_peer_id == 0) {

      // encode and send control messages and client config messages
      peerSendConfig();

      if(readConfigSocket(false) > 0) {
        decodeConfigData();
      }
    }

    // and once in the cycle, return?
    if (i_peer_id == follow_id) {
      return;
    }
  }
}

void NetCommunicatorPeer::unpackPeerData(MessageBuffer::ptr_type& buffer)
{
  // check the message size, should at least contain control bytes
  if (size_t(buffer->fill) < control_size) {
    /* DUECA network.

       A really small, possibly spurious message was received from a
       peer. If this happens regularly, it might indicate a bug in the
       DUECA network code.
     */
    W_NET("Message from peer " << buffer->origin << " too small, " <<
          buffer->fill);
    data_comm->returnBuffer(buffer);
    return;
  }

  // decode header data
  try {
    ControlBlockReader i_(buffer);

    // check that crc is good, if not treat as missed message
    if (!i_.crcgood) {
      /* DUECA network.

         A checksum failure on the data in a message. This should not
         occur.
     */
      W_NET("CRC failure in message, cycle=" << i_.cycle);
      data_comm->returnBuffer(buffer);
      return;
    }

    // check that this is from the current communication group
    if (i_.group_magic != group_magic) {
      /* DUECA network.

         Rogue message came through! Old DUECA's interfering? Second
         DUECA process running?
      */
      W_NET("Network message likely from another DUECA group");
      data_comm->returnBuffer(buffer);
      return;
    }

    // check that it is not my own message being presented
    if (i_.peer_id == peer_id) {
      DEB2("my own message, cycle " << i_.cycle);
      data_comm->returnBuffer(buffer);
      return;
    }

    DEB2("message from " << i_.peer_id << " cycle " <<
        i_.cycle << " n=" << buffer->fill);

    // mark the buffer with the cycle number
    buffer->message_cycle = i_.cycle.cycleCount();

    // process when message from master comes in
    if (i_.peer_id == 0) {

      // logic for checking previous cycle OK (and therefore my flag
      // in current cycle), performed after 0 sending

      // when this is not my first cycle, check the number of
      // received messages matches number of peers as indicated by
      // master
      if (trackingudpcycle) {

        // count how many peers' messages are up to date, matching or
        // surpassing the latest cycle flagged by the send master
        unsigned nreceived = 0U;
        for (peer_cycles_type::iterator pp = peer_cycles.begin();
             pp != peer_cycles.end(); pp++) {

          // checks whether the latest received cycle from this peer
          // is just behind the indicated new cycle (normal case),
          // equal to or even one ahead (in case of back-tracking)
          if (pp->second.cycleIsUpToDate(i_.cycle)) nreceived++;
        }

        // if this is equal to the number of peers as predicted for
        // the *previous* cycle, all is well
        if (nreceived == npeers) {

          // assuming my message will say no error
          errorbit = 0;

          // if there are more peers in the list than indicated by
          // master, remove those ones that no longer update
          if (npeers < peer_cycles.size()) {
            for (peer_cycles_type::iterator pp = peer_cycles.begin();
                 pp != peer_cycles.end(); ) {
              if (pp->second + 1 < i_.cycle) {
                peer_cycles_type::iterator toerase = pp;
                /* DUECA network.

                   Information on removing a peer from the list of
                   peers, possible in communication where peers can
                   leave.
                */
                I_NET("Peer " << pp->first << " at " << pp->second <<
                      " no update, removing at " << i_.cycle);
                pp++; peer_cycles.erase(toerase);
              }
              else {
                pp++;
              }
            }
          }

        }
        else if (nreceived > npeers) {

          // this would be possible if we count rogue messages???
          /* DUECA network.

             Received too many messages in a communication
             cycle. Should happen only sporadically, e.g., after
             recovering from a network error or when peers leave in
             unexpected fashion.
          */
          W_NET("got too many messages, node 0 cycle " << i_.cycle <<
                " recvd " << nreceived << " peers " << npeers);
          errorbit = 0;
        }
        else {

          // not all peer data up-to-date. Ask for repeat
          /* DUECA network.

             Received too few messages in a communication
             cycle. Should happen only sporadically, typically due to
             a network error or when a peer unexpectedly leaves. The
             return message to the master will include a request for
             repeat of the cycle.
          */
          W_NET("got too few messages (" << nreceived <<
                "), peers=" << npeers <<
                " asking repeat, node 0 cycle " << i_.cycle);
          for (peer_cycles_type::iterator pp = peer_cycles.begin();
               pp != peer_cycles.end(); pp++) {
            /* DUECA network.

               Information on peer status after repeat.
             */
            I_NET("peer " << pp->first << " at " << pp->second << " " <<
                  pp->second.cycleIsUpToDate(i_.cycle));
          }
          errorbit = 0x8000;
        }

        // take the master's cue on message cycles
        message_cycle = i_.cycle;

        // new, determine sendstate to match the no0 cycle
        if (packed_cycle.cycleIsNext(message_cycle)) {

          // normal case, cycle has advanced, to pack and send new message
          sendstate = Normal;
        }
        else if (packed_cycle.cycleIsCurrent(message_cycle)) {

          /* DUECA network.

             Information on recovery logic. */
          I_NET("master's cycle " << message_cycle <<
                " is equal to previously packed, to Stasis");
          // repeat the current buffer
          sendstate = Stasis;
        }
        else if (packed_cycle.cycleIsPrevious(message_cycle)) {

          /* DUECA network.

             Information on recovery logic. */
          I_NET("master's cycle " << message_cycle <<
                " traveled back, to Recover");

          // send the backup buffer
          sendstate = Recover;
        }

        // number of peers for this cycle
        if (npeers != i_.npeers) {
          DEB("Cycle " << message_cycle << " number of peers change, from "
              << npeers << " to " << i_.npeers);
        }

        // only if from peer 0 && tracking the cycle
        if (message_cycle.cycleIsNext(message_cycle)) {
          lastround_npeers = npeers;
        }
        npeers = i_.npeers;

      }

      // message from 0, and *not yet* tracking the cycle
      else if (message_cycle.cycleIsCurrent(i_.cycle)) {

        // starting to track the communication at this cycle from no 0
        message_cycle = i_.cycle;

        // only place where packed_cycle is initialized
        packed_cycle = i_.cycle - 0x10;
        sendstate = Normal;
        errorbit = uint16_t(0);
        trackingudpcycle = true;
        npeers = i_.npeers;
        /* DUECA network.

           Information message, this node will enter communication at
           the specified cycle.
        */
        I_NET("Entering send cycle " << i_.cycle << " npeers=" << npeers);
      }

      // changes in follow ID, due to planned leaving, effective in the
      // cycle *following* the one indicated in the change
      // Catch: if this cycle needs to be repeated somehow!
      while (follow_changes.notEmpty() &&
             follow_changes.front().target_cycle <= message_cycle) {
        follow_id = follow_changes.front().peer_id;
        DEB("Changing follow ID as planned to " << follow_id << " at " <<
            message_cycle);
        follow_changes.pop();
      }
    } // end of master (#0) special processing

    // if we are not in the cycle yet, return now.
    if (trackingudpcycle) {

      // remember when to send, send only if message_cycle and repeat
      // index are correct
      myturntosend = i_.peer_id == follow_id &&
        message_cycle == i_.cycle;

        /* && message_cycle == i_.cycle */;

    }
    else {
      data_comm->returnBuffer(buffer);
      return;
    }

    // decode only if this message number, from this peer, has not
    // passed by yet
    peer_cycles_type::iterator pp = peer_cycles.find(i_.peer_id);

    if (pp == peer_cycles.end()) {

      // this peer has not been in the party before, take the cycle
      // as lead, and extend the peer_cycles map

      /* DUECA network.

           Information message, a peer will be added at the specified
           cycle.
      */
      W_NET("Adding peer " << i_.peer_id <<
            " to tracking, at cycle " << i_.cycle);
      clientUnpackPayload(buffer, i_.peer_id,
                          current_tick, i_.peertick, i_.usecs_offset);
      peer_cycles[i_.peer_id] = i_.cycle;

    }
    else {

      // a known peer, determine if this cycle has new data

      if (pp->second.cycleIsNext(i_.cycle)) {
        clientUnpackPayload(buffer, i_.peer_id,
                            current_tick, i_.peertick, i_.usecs_offset);
        // this adds or updates the peer cycle count
        pp->second = i_.cycle;
      }
      else if (pp->second.cycleIsCurrentOrPast(i_.cycle)) {
        /* DUECA network.

           A message from this peer at the given cycle has already
           been processed, and will now be ignored.
        */
        W_NET("Peer " << i_.peer_id << " already processed cycle_p "
              << pp->second << " i_.cycle " << i_.cycle <<
              " cycle " << message_cycle);
        data_comm->returnBuffer(buffer);
      }
      else {
        /* DUECA network.

           Cannot follow the changes in peer/master cycles, message
           ignored.

           @todo fix, this can easily happen with stuck messages
        */
        E_NET("Peer " << i_.peer_id << " cycles missed, cycle_p "
              << pp->second << " i_cycle " << i_.cycle <<
              " cycle " << message_cycle);
        data_comm->returnBuffer(buffer);
      }
    }
  }

  //    catch (const std::exception& e) {
  catch(const dueca::AmorphStoreBoundary& e) {
    /* DUECA network.

       Cannot decode the data in a given UDP message. Might indicate a
       significant failure, or different DUECA / software version
       between the other peer and this node.
    */
    E_NET("Error decoding UDP message " << e.what());
    data_comm->returnBuffer(buffer);
    throw(e);
  }

}


void NetCommunicatorPeer::oneCycle(Activity& activity)
{
  // set-up the connection
  setupConnection(activity);
  _oneCycle(activity);
}


void NetCommunicatorPeer::startCyclic(Activity& activity)
{
  // set-up the connection
  setupConnection(activity);

  /* 5: now do cyclic work, block & read UDP messages, */
  current_tick = SimTime::getTimeTick();
  do {
    _oneCycle(activity);
  } while (message_cycle < last_cycle);

  /* after this, clear the connections */
  clearConnections();
}

void NetCommunicatorPeer::clearConnections()
{
  /* DUECA network.

     Information that the connection will be broken as planned. */
  I_NET("undoing connection");

  resetClientConfiguration();

  /* done. Reset for possible re-connection */
  peer_cycles.clear();

  current_send_buffer->release();
  backup_send_buffer->release();
  delete current_send_buffer; current_send_buffer = NULL;
  delete backup_send_buffer; backup_send_buffer = NULL;

  // reset the communication devices
  data_comm.reset();
  conf_comm.reset();

  message_cycle = 0;
  follow_id = 0xffff;
  sendstate = Normal;
  npeers = 0; errorbit = 0;
  connection = false;
}

void NetCommunicatorPeer::peerSendConfig()
{
  if (stop_commanded) {
    char cbuf[8];
    AmorphStore s(cbuf, 8);
    UDPPeerConfig cmd(UDPPeerConfig::DeletePeer, peer_id);
    DEB("Sending payld  " << cmd);
    cmd.packData(s);
    sendConfig(s);
    stop_commanded = false;
  }
  clientSendConfig();
}


bool NetCommunicatorPeer::decodeConfigData()
{
  // decode the new data
  AmorphReStore s = commbuf.getStore();
  size_t storelevel = s.getIndex();

  try {
    while (s.getSize()) {
      size_t storelevel0 = s.getIndex();
      UDPPeerConfig cmd(s);   // decode the config command
      DEB("Unpack payld  " << cmd);
      storelevel = s.getIndex();
      switch(cmd.mtype) {

      case UDPPeerConfig::ConfigurePeer: {
        peer_id = cmd.peer_id;
        UDPPeerInfo pi(s);
        /* DUECA network.

           Information on peer number supplied by master.
        */
        I_NET("Configured as peer number " << peer_id <<
              " communication on " << pi.url << " packet size " <<
              pi.message_size);

        if (override_data_url.size()) {
          /* DUECA network.

             If my assumption is correct, you supplied an overide for the
             data url. This is generally only needed when the master is
             located on a network with network address translation.
           */
          W_NET("Overriding master's data url, from " << pi.url << " to " <<
                override_data_url <<
                ", only appropriate for network translation!");
          pi.url = override_data_url;
        }
        if (interface_address.size() == 0) {
          /* DUECA network.

             The own interface address was not explicitly configured,
             and an interface address has been automatically detected
             by the network master. If this address is not correct,
             explicitly supply the network address of the interface
             that you want to use.
          */
          W_NET("Own interface not configured, using default " << pi.peer_if <<
                " supplied by master");
          interface_address = pi.peer_if;
        }
        DEB(pi);

        // check compatibility of timing
        if (fabs(pi.time_granule - Ticker::single()->getTimeGranule()) >
            1e-12) {
          /* DUECA network.

             The present node and the master in the communication have
             incompatible timing configurations, adjust your
             configuration */
          E_CNF("Timing settings do not match master");
          conf_comm.reset();
          throw(connectionfails());
        }

        // remember the master's expected cycle time
        ts_interval = pi.interval;

        // create the data connection
        std::string key = pi.url.substr
          (0, pi.url.find(":")) + std::string("-peer");

        // copy the relevant data to the specification
        url = pi.url;
        buffer_size = pi.message_size;

        // create the communicaiton
        data_comm = PacketCommunicatorFactory::instance().create(key, *this);

        // flush any old messages, if applicable
        data_comm->flush();

        // create buffers
        current_send_buffer = new MessageBuffer(pi.message_size, control_size);
        backup_send_buffer = new MessageBuffer(pi.message_size, control_size);

        // start cycle is in the message
        message_cycle = pi.join_cycle;

        // set it for the current send buffer
        current_send_buffer->message_cycle = message_cycle - 0x10;

        // inform connection is established
        this->clientIsConnected();
      }
        break;

      case UDPPeerConfig::HookUp:
        if (cmd.target_cycle > message_cycle) {
          follow_changes.push_back(cmd);
          DEB("Planned changed follow ID to " << cmd.peer_id << " at " <<
              dueca::CycleCounter(cmd.target_cycle) << " now at " <<
              message_cycle);
        }
        else {
          follow_id = cmd.peer_id;
          DEB("Immediate change follow ID to " << cmd.peer_id << " at " <<
              message_cycle);
        }
        break;

      case UDPPeerConfig::DeletePeer:
        if (cmd.peer_id == peer_id) {
          last_cycle = cmd.target_cycle;
        }
        break;

      case UDPPeerConfig::ClientPayload:
        try {
          clientDecodeConfig(s);
          storelevel = s.getIndex();
        }
        catch (const dueca::AmorphReStoreEmpty &e) {
          // reset to before the ClientPayload flag!
          storelevel = storelevel0;
          throw(e);
        }

        break;

      case UDPPeerConfig::InitialConfComplete:

        commbuf.saveForLater(storelevel);
        return true;

      default:
        /* DUECA network.

           Unexpected error, configuration command that could not be
           processed. Indicates data corruption or an error in DUECA
           programming. */
        E_NET("Peer received impossible config command " << cmd.mtype);
      }
    }

    // remaining size is zero now, this will reset the commbuf
    commbuf.saveForLater(storelevel);
  }
  catch (const dueca::AmorphReStoreEmpty &e) {

    // config data did not result in a complete config.
    // inform the commbuf of the unused bytes in the store
    commbuf.saveForLater(storelevel);
  }

  return false;
}

void NetCommunicatorPeer::clientSendWelcome()
{
  //
}

void NetCommunicatorPeer::resetClientConfiguration()
{
  //
}

void NetCommunicatorPeer::setStopTime(const TimeTickType& last_tick)
{
  if (last_tick == MAX_TIMETICK) {
    last_cycle = std::numeric_limits<uint32_t>::max();
    stop_commanded = false;
  }
  else {
    DEB("Commanding stop " << last_tick);
    stop_commanded = true;
  }
};
