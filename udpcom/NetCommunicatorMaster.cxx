/* ------------------------------------------------------------------   */
/*      item            : NetCommunicatorMaster.cxx
        made by         : Rene' van Paassen
        date            : 170912
        category        : body file
        description     :
        changes         : 170912 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define NetCommunicatorMaster_cxx
#include "NetCommunicatorMaster.hxx"

#include <exception>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/swap.hpp>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>

#define I_NET
#include <dueca-conf.h>
#include <dueca/debug.h>
#include <dueca/Ticker.hxx>

#include "WebsockCommunicator.hxx"
#include "NetCommunicatorExceptions.hxx"
#include <UDPPeerConfig.hxx>
#include "UDPPeerInfo.hxx"

#define DEBPRINTLEVEL -1
#include <debprint.h>

#ifdef BUILD_TESTOPT
static const double test_failprob = 0.0001;
#endif

DUECA_NS_START;

NetCommunicatorMaster::NetCommunicatorMaster() :
  // base class implementing the generic part of communication
  NetCommunicator(),

  connect_check_interval(1),
  config_url(""),
  public_data_url(""),
  communicating(false),
  server_needsconnect(0),
  conf_comm(),
  peers(),
  next_peer_id(0),
  peer_changes(),
  net_perbyte(0.1),     // assuming 100 Mbit
  net_permessage(10.0), // set up time
  net_tau1(0.0001),
  net_tau2(0.0001),
  last_cycle_time(0),
  last_cycle_bytes(0),
  current_tick(0),
  nreceived(0)
{
  // memset(&host_address, 0, sizeof(host_address));
  PacketCommunicatorSpecification::callback =
    common_callback(this, &NetCommunicatorMaster::unpackPeerData);
}

bool NetCommunicatorMaster::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */

  // interval for checking the connection requests
  connect_check_interval = max
    (TimeTickType(1),
     TimeTickType(round(2.0 / Ticker::single()->getTimeGranule())) /
     ts_interval);

  if (!startServer()) {
    server_needsconnect = connect_check_interval;
  }

  // these are members of the parent class; for a master they can be
  // configured here
  current_send_buffer = new MessageBuffer(buffer_size, control_size);
  backup_send_buffer = new MessageBuffer(buffer_size, control_size);

  communicating = true;

  return true;
}

// destructor
NetCommunicatorMaster::~NetCommunicatorMaster()
{
  if (current_send_buffer && backup_send_buffer) {
    current_send_buffer->release();
    delete current_send_buffer;
    backup_send_buffer->release();
    delete backup_send_buffer;
  }
}

// process configuration input for a slave type replicator
void NetCommunicatorMaster::flushStore(AmorphStore& s, unsigned peer_id)
{
  if (s.getSize() == 0) {
    /* DUECA network.

       Trying to flush data to the configuration connection, but store
       is empty. Indicates a problem with big objects being sent?
    */
    E_INT("Nothing to flush, maybe stores not big enough for single object?");
    throw(dueca::AmorphStoreBoundary());
  }
  conf_comm->sendConfig(s, peer_id);

  // reset the store
  s.reUse();
}

bool NetCommunicatorMaster::startServer()
{
  if (not conf_comm) {
    if (!config_url.size()) {
      /* DUECA network.

	 Config url needs to be supplied */
      W_NET("Config URL needs to be supplied");
      throw(connectionfails());
    }
    conf_comm.reset(new WebsockCommunicatorConfig
                    (config_url, timeout, common_callback
                     (this, &NetCommunicatorMaster::assignPeerId),
                     config_buffer_size, 3));
  }



  // also create the data connection
  if (not data_comm) {
    // correct for old-style network definitions
    if (!url.size()) {
      url = std::string("udp://") + peer_address + std::string(":") +
        boost::lexical_cast<std::string>(dataport);
    }

    std::string key = url.substr(0, url.find(":")) + std::string("-master");
    data_comm = PacketCommunicatorFactory::instance().create(key, *this);

    // URL with websocket will piggy-back on the existing config server
    auto data_comm_ws =
      boost::dynamic_pointer_cast<WebsockCommunicatorMaster,
                                  PacketCommunicator>(data_comm);

    if (data_comm_ws) {
      data_comm_ws->attachToMaster(conf_comm);
    }

    // flush any old messages, if applicable
    data_comm->flush();
  }

  return true;
}

void NetCommunicatorMaster::stopServer()
{
  DEB("stopServer called");
  for (auto &p : peers) {
    char cbuf[16];
    AmorphStore s(cbuf, 16);
    UDPPeerConfig cmd(UDPPeerConfig::DeletePeer, p->send_id);
    DEB(cmd);
    cmd.packData(s);
    conf_comm->sendConfig(s, p->send_id);
  }

  // remove the communicator, resets the UDP or Websocket connections
  data_comm.reset();
  // conf_comm.reset();
}

// collection of information on a peer in the communication
NetCommunicatorMaster::CommPeer::CommPeer
(unsigned sendid, unsigned previd, const std::string& address) :
  state(Vetting),
  delta_time(0.0),
  send_id(sendid),
  follow_id(previd),
  commbuf(),
  address(address)
{
  //
}

NetCommunicatorMaster::CommPeer::~CommPeer()
{
  //
}

NetCommunicatorMaster::ChangeCycle::
ChangeCycle(uint32_t change_cycle, uint16_t peer, bool add) :
  change_cycle(change_cycle),
  peer(peer),
  addition(add)
{
  //
}

int NetCommunicatorMaster::assignPeerId(const std::string& address)
{
  // add a new peer to the list
  unsigned previous_peer_id = peers.size() ? peers.back()->send_id : 0;
  next_peer_id++;

  // reserve the room for the peer
  peers.push_back(std::shared_ptr<CommPeer>
                  (new CommPeer(next_peer_id, previous_peer_id,
                                address)));
  DEB("new peer from " << address << " id " << next_peer_id);

  // inform about the presence of a new peer
  clientInfoPeerJoined(address, next_peer_id, TimeSpec(current_tick));

  /* DUECA network.

     Information on a new connecting peer. */
  I_NET("Accepting a connection from " << address <<
        " peer id " << next_peer_id);
  return int(next_peer_id);
}

/*
   Send the current network configuration to a newly joining peer

   * send basic information, UDP connection information, who to
     follow, slave id
   * call client code for additional configuration
*/


void NetCommunicatorMaster::
sendCurrentConfigToPeer(const CommPeer& peer, uint32_t join_cycle)
{
  // send the current configuration to this peer
  char buffer[config_buffer_size];
  AmorphStore s(buffer, sizeof(buffer));

  try {


    // tell the previous ID in the sending sequence
    DEB(UDPPeerConfig
        (UDPPeerConfig::HookUp, peer.follow_id, join_cycle));
    ::packData(s, UDPPeerConfig
               (UDPPeerConfig::HookUp, peer.follow_id, join_cycle));

    // tell the new ID, also triggers info reading
    DEB(UDPPeerConfig
        (UDPPeerConfig::ConfigurePeer, peer.send_id));
    ::packData(s, UDPPeerConfig
               (UDPPeerConfig::ConfigurePeer, peer.send_id));

    // information on UDP network connection
    UDPPeerInfo pi
      (public_data_url.size() ? public_data_url : url, peer.address,
       buffer_size, join_cycle,
       Ticker::single()->getTimeGranule(), ts_interval);
    DEB("Information to peer " << pi);

    // pack the peer information
    DEB(pi);
    ::packData(s, pi);

    // give clients an option to pack extra data
    clientWelcomeConfig(s, peer.send_id);

    // mark initial config complete
    DEB(UDPPeerConfig(UDPPeerConfig::InitialConfComplete));
    ::packData(s, UDPPeerConfig(UDPPeerConfig::InitialConfComplete));
    flushStore(s, peer.send_id);
  }
  catch(const AmorphStoreBoundary& e) {

    // the peerinfo data is too large
    /* DUECA network.

       Cannot send the configuration data to the peer, because it is
       too large for the configuration buffer. Check that the peer
       welcome message is not larger than the buffer.
    */
    E_NET("Configuration data and peer information too large for TCP buffer");
    throw(e);
  }
  catch (const configconnectionbroken& e) {

    /* DUECA network.

       Unexpected error in setting up the connection with a new
       peer. */
    W_NET("Error in connection with new peer, from " << peer.address);
    clientInfoPeerLeft(peer.send_id, TimeSpec(join_cycle));
    peers.pop_back();
  }
}

/*
   Inform a peer that the UDP message should be in reply to another
   peer */
void NetCommunicatorMaster::changeFollowId(const CommPeer& peer, uint32_t target_cycle)
{
  char cbuf[16];
  AmorphStore s(cbuf, 16);
  DEB(UDPPeerConfig(UDPPeerConfig::HookUp, peer.follow_id));
  ::packData(s, UDPPeerConfig
             (UDPPeerConfig::HookUp, peer.follow_id, target_cycle));
  conf_comm->sendConfig(s, peer.send_id);
}

/* Instruct the peer following the one with this send id, to follow the one
   previously followed by the blablabla */
void NetCommunicatorMaster::correctFollowId(uint32_t send_id,
                                            uint32_t follow_id)
{
  for (auto &p2: peers) {
    if (p2->state <= CommPeer::Active &&
        p2->follow_id == send_id) {
      p2->follow_id = follow_id;
      if (p2->state > CommPeer::Vetting) {
        changeFollowId(*p2);
        /* DUECA network.

           Information on changes in the send order. */
        I_NET("Correcting follow order, instructing peer " <<  p2->send_id <<
              " to drop " << send_id << " and follow " << follow_id);
      }
      else {
        /* DUECA network.

           Information on changes in the send order. */
         I_NET("Correcting follow order, changing inactive peer "
              << p2->send_id <<
              " to drop " << send_id << " and follow " << follow_id);
      }
      break; // from for loop!
    }
  }
}

/*
  Configuration updates step 1

  * reads any acknowledgement information / external vetting
  * implement planned joining/leaving
  * work the state logic for all peers; react to planned changes,
    broken connections
  * check for configuration data from each of the peers
  * process entry removal and addition requests

  Receive the configuration requests from slaves, and add or remove
  entries. This runs after the UDP messages have been exchanged
 */
void NetCommunicatorMaster::checkAndUpdatePeerStates(const TimeSpec& ts)
{
  // update planned peer inclusion/exclusion
  while (peer_changes.size() &&
         message_cycle.cycleIsCurrentOrPast
         (peer_changes.front().change_cycle)) {

    // find the relevant peer by matching against send id
    peerlist_type::iterator pp = peers.begin();
    while (pp != peers.end() &&
           (*pp)->send_id != peer_changes.front().peer) pp++;

    // should not happen?
    if (pp == peers.end()) {
      /* DUECA network.

         A peer has disappeared from the list of sending peers. */
      W_NET("Peer " << peer_changes.front().peer <<
            " disappeared from list");
      peer_changes.pop_front();
      continue;
    }

    // implement the change
    if (peer_changes.front().addition) {
      (*pp)->state = CommPeer::Active;
      if (npeers == 0) {
        current_send_buffer->message_cycle = message_cycle - 0x10;
      }
      npeers++;
    }
    else {
      clientInfoPeerLeft((*pp)->send_id, ts);
      peers.erase(pp);
      npeers--;
    }

    // clear the planned change
    peer_changes.pop_front();
  }

  // buffer for request configation data from peers
  //char peerreq[1024];

  // receive peer configuration messages
  MessageBuffer::ptr_type msgbuf = conf_comm->receiveConfig(false);
  while (msgbuf) {
    for (auto &p: peers) {
      if (p->send_id == msgbuf->origin) {
        if (msgbuf->fill == 0) {

          // signal for a broken connection, peer will be removed next
          p->state = CommPeer::Broken;
          /* DUECA network.

             The communication with a given peer has an error.
          */
          W_NET("Communication with peer " << p->send_id << " broken");
        }
        else {
          p->commbuf.write(msgbuf);
        }
        break;
      }
    }
    conf_comm->returnBuffer(msgbuf);
    msgbuf = conf_comm->receiveConfig(false);
  }

  // the anychanges flag ensures that peer vetting is re-checked when
  // a peer is approved, so all peers can be approved for the same cycle
  // and given their proper order
  bool anychanges = true;
  while (anychanges) {

    // check for state changes from all peers
    anychanges = false;
    for (peerlist_type::iterator pp = peers.begin();
         pp != peers.end(); ) {

      // peer state?
      switch((*pp)->state) {

      case CommPeer::Vetting: {

        switch (clientAuthorizePeer(*(*pp), ts)) {
        case Reject: {
          /* DUECA network.

             Client code rejected a peer attempting to join. Check
             your configuration, and check your network for spurious
             messages. */
          W_NET("Rejecting peer from " << url);
          correctFollowId((*pp)->send_id, (*pp)->follow_id);
          clientInfoPeerLeft((*pp)->send_id, ts);
          pp = peers.erase(pp);
          continue;
        }
        case Accept: {
          if (npeers == 0) {

            // if we have no UDP communication yet, kickstart
            sendCurrentConfigToPeer(**pp, message_cycle.cycleIncrement(2));
            (*pp)->state = CommPeer::Wait;
            peer_changes.push_back
              (ChangeCycle(message_cycle.cycleIncrement(2), (*pp)->send_id, true));
          }
          else {
            // already in a cycle with peers. make a planned change;
            // the peer is configured to hook up to currently the last
            // sender
            sendCurrentConfigToPeer(**pp, message_cycle.cycleIncrement(10));
            (*pp)->state = CommPeer::Wait;
            peer_changes.push_back
              (ChangeCycle(message_cycle.cycleIncrement(10), (*pp)->send_id, true));
          }
          anychanges = true;

          // read the config in the next cycle
          pp++; continue;
        }
          break;
        case Delay:
          // no action yet
          break;
        }

      }

        // wait and active need no actions
      case CommPeer::Wait:
      case CommPeer::Active: break;

        // broken is after configuration connection breaks
      case CommPeer::Broken:

        // sudden client disconnect, find the peer following this one, if
        // present, and reconfigure the send chain
	/* DUECA network.

	   A sudden disconnect by a peer is detected. The communication
	   chain will be reconfigured to skip the peer.
	*/
        W_NET("Sudden disconnect from peer " << (*pp)->send_id);
        correctFollowId((*pp)->send_id, (*pp)->follow_id);

        // inform the client process
        clientInfoPeerLeft((*pp)->send_id, ts);

        // remove the peer from the list, and reduce number of connected peers
        pp = peers.erase(pp);
        npeers--;

        // over to next peer, and skip processing the configuration data
        pp++;
        continue;

      };   // end of the peer state switch

      // read the configuration
      decodeConfigData(**pp);

      // over to the next peer
      pp++;

    } // to process next peer's messages
  } // until no peer additions left
}

/*
   Take a configuration update buffer, and send it out to all
   currently connected peers.
 */
void NetCommunicatorMaster::distributeConfig(AmorphStore& s)
{
  // check that the store has been filled at least somewhat
  if (s.getSize() == 0) {

    /* DUECA network.

       A message to be packed for the peer configuration is too large
       to fit in the communication buffer. */
    E_NET("stores not big enough for single object");
    throw(dueca::AmorphStoreBoundary());
  }
  conf_comm->sendConfig(s);

  // reset the store
  s.reUse();
}

void NetCommunicatorMaster::doCycle(const TimeSpec& ts, Activity& activity)
{
  // current tick as per ts:
  current_tick = ts.getValidityStart();
  int start_offset = Ticker::single()->getUsecsSinceTick(current_tick);
  int cycle_time = 0;
  int cycle_bytes = 0;
  trigger_recover = false;

  if (npeers) {
    cycle_bytes = codeAndSendUDPMessage(current_tick);
  } else {
    // keep up the packed_cycle, for the check
    packed_cycle = message_cycle;
  }

  // Receive the data from the peers; use select and a timeout
  // nreceived will be updated in the data unpack
  nreceived = 0;
  while (nreceived < npeers) {

#ifdef BUILD_TESTOPT
#warning "test options selected, simulated message receive missing added"

    // pretend to miss this single message
    if (int(random()/test_failprob/(RAND_MAX+1.0)) == 0) {

      DEB("Simulating a receive failure cycle " << message_cycle);
      data_comm->setFailReceive();
    }
#endif

    // block and wait for the next message. When data comes in, this is
    // passed through unPackPeerData
    activity.logBlockingWait();
    const auto result = data_comm->receive();
    activity.logBlockingWaitOver();
    ssize_t nbytes = result.second;

    // recover flag transmitted by peer
    if (trigger_recover) break;

    // 0 bytes means a timeout
    if (nbytes == 0) {

      /* DUECA network.

         There is a timeout in a communication cycle. This may be due
         to a temporary network failure, or to a peer dropping out of
         the conversation. The master timeout should be short enough
         to enable a quick recovery after a missed message, but if
         many timeouts occur, consider increasing the timeout value
         and possibly the cycle interval. */
      W_NET("Timeout cycle " << message_cycle);

      // only a timeout, no repeat request, go to Stasis mode and
      // keep repeating the current message
      // if already in Recover, stay in recover, and to another round
      // of sending
      if (sendstate != Recover) {
        DEB("Timeout, to Stasis");
        sendstate = Stasis;
      }

      // modify the repeat nibble of the cycle counter
      message_cycle.cycleRepeatIncrement();

      // skip the recovery state logic, run by the TCP check-up,
      // (maybe peer crash?) then continue
      goto connection_checkup;
    }

    if (size_t(nbytes) < control_size) {
      /* DUECA network.

         Very small message received from a peer. Check that the DUECA
         versions are compatible, and check for interfering
         communication. */
      W_NET("Message from peer " << result.first << " too small, " << nbytes);
      continue;
    }
    DEB("received " << nbytes);

    // determine round_trip delay from the first received message
    if (cycle_time == 0) {
      cycle_time = Ticker::single()->getUsecsSinceTick(current_tick) -
        start_offset;
      cycle_bytes += nbytes;
      DEB("trip delay " << cycle_time);

      // update per-byte time if enough variation
      if (std::abs(cycle_bytes - last_cycle_bytes) >
          std::min(10, int(buffer_size) / 20)) {
        net_perbyte = (1.0 - net_tau1)*net_perbyte + net_tau1*
          double(cycle_time-last_cycle_time)/double(cycle_bytes-last_cycle_bytes);
      }
      // and per-message time
      net_permessage = (1.0 - net_tau2)*net_permessage + net_tau2*0.5*
        (double(cycle_time) - double(cycle_bytes)*net_perbyte);
      last_cycle_time = cycle_time;
      last_cycle_bytes = cycle_bytes;
    }

  }
  DEB2("udp receive over");

  // erase deleted peers
  if (npeers < peer_cycles.size()) {
    for (peer_cycles_type::iterator pp = peer_cycles.begin();
         pp != peer_cycles.end(); ) {
      if (message_cycle.cycleIsCurrentOrPast(pp->second)) {
        peer_cycles_type::iterator toerase = pp;
        /* DUECA network.

           Information message on a peer that no-longer responds and
           will be deleted from the cycle.
        */
        W_NET("Peer " << pp->first << " at " << pp->second <<
              " no update, removing at " << message_cycle);
        pp++; peer_cycles.erase(toerase);
      }
      else {
        pp++;
      }
    }
  }

  // at this point, either correctly exited and nreceived == npeers,
  // or exited due to trigger_recover
  if (trigger_recover) {
    // recovery request received. Should go to previous cycle number, and
    // send the backup buffer
    assert(sendstate != Recover);
    message_cycle.cycleBack();
    DEB("Recovery request, cycling back, Recover, to " << message_cycle);
    sendstate = Recover;
  }
  else if (sendstate == Recover) {
    message_cycle = message_cycle.cycleIncrement();
    DEB("From Recover, progressing to Stasis, to " << message_cycle);
    sendstate = Stasis;
  }
  else {
    message_cycle = message_cycle.cycleIncrement();
    DEB2("Normal state to " << message_cycle);
    sendstate = Normal;
    errorbit = 0;
  }

#if DEBPRINTLEVEL > 0
  if (message_cycle.cycle_counter == 0U) {
    DEB("ROLLOVER with " << sendstate);
  }
#endif

  // now check the requests for new clients, configuration changes
  // from clients & push configuration changes around
 connection_checkup:

  if (!communicating) {
    stopServer();
    return;
  }

  checkAndUpdatePeerStates(ts);
  clientSendConfig(ts, 0);
  if (server_needsconnect) {
    if (--server_needsconnect == 0 && !startServer()) {
      server_needsconnect = connect_check_interval;
    }
    return;
  }
  //checkNewConnections(ts);
}


void NetCommunicatorMaster::unpackPeerData(MessageBuffer::ptr_type& buffer)
{
  // check the message size,should at least contain control bytes
  if (size_t(buffer->fill) < control_size) {
    /* DUECA network.

       Very small message received from a peer. Check that the DUECA
       versions are compatible, and check for interfering
       communication. */
    W_NET("Message from peer " << buffer->origin << " too small, " <<
          buffer->fill);
    data_comm->returnBuffer(buffer);
    return;
  }

  // decode data header
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
    if (i_.peer_id == 0) {
      DEB2("my own message, cycle " << i_.cycle);
      data_comm->returnBuffer(buffer);
      return;
    }

    DEB("message from " << i_.peer_id << " cycle " <<
          i_.cycle << " n=" << buffer->fill);

    // mark the buffer with the cycle number
    buffer->message_cycle = i_.cycle.cycleCount();

    // at this point, decode the data if this has not yet been processed
    // for this peer, figure out the cycle count for the peer
    peer_cycles_type::iterator pp = peer_cycles.find(i_.peer_id);

    if (pp == peer_cycles.end()) {
      // have not yet seen this peer. If the response is to the current
      // cycle, include in the peer list
      if (i_.cycle == message_cycle) {
        clientUnpackPayload(buffer, i_.peer_id,
                            current_tick, i_.peertick, i_.usecs_offset);
        peer_cycles[i_.peer_id] = message_cycle;
      }
      else {
        /* DUECA network.

           Minor start problem with a peer, starting at a cycle not
           commanded.
        */
        I_NET("Peer " << i_.peer_id << " erroneous start i_cycle " <<
              i_.cycle << " cycle " << message_cycle);
        data_comm->returnBuffer(buffer);
      }
    }
    else {
      // know this peer already, unpack only if this is new data
      if (pp->second.cycleIsNext(i_.cycle)) {
        clientUnpackPayload(buffer, i_.peer_id,
                            current_tick, i_.peertick, i_.usecs_offset);
        pp->second = i_.cycle;
      }
      else if (pp->second.cycleIsCurrentOrPast(i_.cycle)) {
        /* DUECA network.

           The message from the indicated peer has already been
           processed. This can happen when a message cycle had to be
           repeated. */
        I_NET("Peer " << i_.peer_id << " already processed cycle_p "
              << pp->second << " i_cycle " << i_.cycle <<
              " cycle " << message_cycle);
        data_comm->returnBuffer(buffer);
      }
      else {
        /* DUECA network.

           Cannot follow the changes in peer/master cycles, message
           ignored.
        */
        E_NET("Peer " << i_.peer_id << " cycles messed up, cycle_p "
              << pp->second << " i_cycle " << i_.cycle <<
              " cycle " << message_cycle);
        data_comm->returnBuffer(buffer);
      }
    }

    // only count the exact response as a received message, ignoring
    // stuck/older responses
    if (i_.cycle == message_cycle) {

      // count the number of exact replies
      nreceived++;

      if (i_.errorflag) {
        /* DUECA network.

           One of the peers requested a repeat on a message cycle,
           most likely due to missing/incomplete on the cycle before
           that. Recovery code will be entered, in which the requested
           cycle will be repeated until confirmed by all peers.
        */
        W_NET("Peer " << i_.peer_id <<
              " recover asked in message on cycle " << i_.cycle);

        // but do not do this on the very first cycle!
        if (sendstate != Recover) {
          trigger_recover = backup_send_buffer->fill > 0;
        }
      }
    }
    else {
      /* DUECA network.

         Received a message that did not exactly match the requested
         cycle
      */
      W_NET("Peer " << i_.peer_id << " received i_cycle " << i_.cycle <<
            " not matching requested " << message_cycle);
    }

    // done read one, for the right repeat and cycle
    DEB("Num received " << nreceived << " from peers " << npeers);
  }
  catch(const AmorphReStoreEmpty& e) {
    /* DUECA network.

       Unexpected failure in decoding data from one of the
       peers. Verify that the DUECA versions are identical. */
    W_NET("Cannot decode data from peer " << buffer->origin);
    data_comm->returnBuffer(buffer);
    throw(e);
  }
}

void NetCommunicatorMaster
::clientInfoPeerJoined(const std::string& address, unsigned id,
                       const TimeSpec& ts)
{
  /* DUECA network.

     Information on a new peer joining.
  */
  I_NET("new peer, id " << id << " from " << address);
}

void NetCommunicatorMaster::
clientInfoPeerLeft(unsigned id, const TimeSpec& ts)
{
  /* DUECA network.

     Information on a new peer leaving.
  */
  I_NET("peer leaving, id " << id);
}

NetCommunicatorMaster::VettingResult
NetCommunicatorMaster::clientAuthorizePeer(CommPeer& peer,
                                           const TimeSpec& ts)
{
  /* DUECA network.

     A new peer requesting joining has been authorized.
  */
  I_NET("authorizing peer, id " << peer.send_id);
  return Accept;
}

void NetCommunicatorMaster::decodeConfigData(CommPeer& peer)
{
  // decode the new data
  AmorphReStore s = peer.commbuf.getStore();
  size_t storelevel = s.getIndex();

  try {
    while (s.getSize()) {
      size_t storelevel0 = s.getIndex();
      UDPPeerConfig cmd(s);   // decode the config command
      storelevel = s.getIndex();
      DEB("Unpack payld  " << cmd);

      switch(cmd.mtype) {

      case UDPPeerConfig::DeletePeer: {

	/* DUECA network.

	   A network peer requested to leave. */
        W_NET("Acting on requested delete from peer " << peer.send_id);
        // note a wish to leave ASAP, give it 3 cycles (we already
        // updated the cycle counter to start the next round)
        // peer_changes are processed after the noted cycle, after the
        // noted cycle, the peer will not be included, npeers will be
        // reduced.
        peer_changes.push_back
          (ChangeCycle(message_cycle.cycleIncrement(4), peer.send_id, false));

        // and immediate message on the planned cycle for deletion,
        // peer knows to leave after message_cycle+3
        char cbuf[8];
        AmorphStore s2(cbuf, 8);
        UDPPeerConfig cmd(UDPPeerConfig::DeletePeer, peer.send_id,
                          message_cycle.cycleIncrement(3));
        DEB(cmd);
        cmd.packData(s2);
        flushStore(s2, peer.send_id);

        // find out if some other peer is sending after this peer, advance notice of
        // changing the hookup; is also sent later when deletion in database complete
        correctFollowId(peer.send_id, peer.follow_id);
      }
        break;

      case UDPPeerConfig::ClientPayload:
        try {
          clientDecodeConfig(s, peer.send_id);
          storelevel = s.getIndex();
        }
        catch (const dueca::AmorphReStoreEmpty &e) {
          // reset to before the ClientPayload flag!
          storelevel = storelevel0;
          DEB("Payload flagged, but failed to unpack");
          throw(e);
        }
        break;

      case UDPPeerConfig::DuecaVersion:

	try {
	  uint16_t vmajor(s);
	  uint16_t vminor(s);
	  uint16_t revision(s);
	  storelevel = s.getIndex();

	  if (vmajor != DUECA_VERMAJOR || vminor != DUECA_VERMINOR ||
	      revision != DUECA_REVISION) {
	    /* DUECA network.

	       A peer dueca process reports a different DUECA version
	       than what is running here. Please update all DUECA
	       nodes to the same version.
	    */
	    W_NET("Peer " << cmd.peer_id <<
		  " reports a different DUECA version " << vmajor <<
		  "." << vminor << "." << revision);
	  }
	}
	catch (const dueca::AmorphReStoreEmpty &e) {
	  /* DUECA network.

	     A peer dueca process has no information on the DUECA
	     version in the welcome message. Update all DUECA nodes to
	     the same version.
	  */
          DEB("Cannot unpack version information");
	  storelevel = storelevel0;
	  throw(e);
	}

      default:
        /* DUECA network.

           Unknown configuration request from a given peer. Check that
           the DUECA versions used are compatible.
        */
        E_NET("peer with send id " << peer.send_id
              << " unhandled command " << cmd.mtype);
      }
    }

    // remaining size is zero now, this will reset the commbuf
    peer.commbuf.saveForLater(storelevel);
  }
  catch (const dueca::AmorphReStoreEmpty &e) {

    // config data did not result in a complete config.
    // inform the commbuf of the unused bytes in the store
    peer.commbuf.saveForLater(storelevel);
  }
}

void NetCommunicatorMaster::breakCommunication()
{
  stopServer();

  communicating = false;    // flags comm break for next cycle

  // npeers = 0;               // stop UDP messages
}

void NetCommunicatorMaster::communicatorAddTiming(ControlBlockWriter &cb)
{
  cb.markTimeOffset(net_permessage, net_perbyte);
}


DUECA_NS_END;
