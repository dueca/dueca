/* ------------------------------------------------------------------   */
/*      item            : UDPSocketCommunicator.cxx
        made by         : Rene' van Paassen
        date            : 200412
        category        : body file
        description     :
        changes         : 200412 first version
        language        : C++
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define UDPSocketCommunicator_cxx

#include <boost/lexical_cast.hpp>

#include <dueca-conf.h>
#define I_NET
#include <dueca/debug.h>

#include "UDPSocketCommunicator.hxx"
#include "NetCommunicatorExceptions.hxx"
#include "NetCommunicator.hxx"
#include <netinet/ip.h>
#include <dueca-udp-config.h>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;

// helper to determine multicast
static bool isMulticastAddress(in_addr_t s_addr)
{
  uint32_t address = ntohl(s_addr);
  return (address & 0xF0000000) == 0xE0000000;
}

// helper to determine broadcast
static bool isBroadcastAddress(in_addr_t s_host, in_addr_t s_mask,
                               in_addr_t s_addr)
{
  uint32_t host = ntohl(s_host);
  uint32_t address = ntohl(s_addr);
  uint32_t mask = ntohl(s_mask);

  return ((address & ~mask) == ~mask) && ((host & mask) == (address & mask));
}

UDPSocketCommunicator::UDPSocketCommunicator(
  const PacketCommunicatorSpecification &spec) :
  PacketCommunicator(spec),
  if_address(spec.interface_address),
  port_re_use(spec.port_re_use),
  lowdelay(spec.lowdelay),
  socket_priority(spec.socket_priority),
  peer_address(),
  dataport(7001),
  comm_send(-1),
  comm_recv(-1),
  connection_mode(Undetermined),
  default_timeout()
{
  // decode the url, e.g. "udp://myhost.mynet:8432"
  if (spec.url.substr(0, 6) != "udp://") {
    /* DUECA network.

       Cannot properly decode the udp connection URL, fix your
       dueca_cnf.py or dueca.cnf file. */
    E_NET("URL for UDP communication incorrect: " << spec.url);
    throw CFErrorConstruction("cannot create, URL incorrect");
  }
  try {
    // see if there is a colon for the port no
    size_t port = spec.url.find(":", 6);
    if (port != string::npos) {
      // assuming that this can be interpreted as a port number
      dataport = boost::lexical_cast<uint16_t>(spec.url.substr(port + 1));
      // what remains is the peer address
      peer_address = spec.url.substr(6, port - 6);
    }
    else {
      // no port given, rest is peer address or name
      peer_address = spec.url.substr(6);
    }
    /* DUECA network.

       Information message on the peer's address and network port.
    */
    I_NET("Decoded peer address " << peer_address << " dataport " << dataport);

    // set the timeout
    struct timeval to = { int(round(spec.timeout * 1e6)) / 1000000,
                          int(round(spec.timeout * 1e6)) % 1000000 };
    default_timeout = to;

    // initialize IP addressess
    memset(&host_address, 0, sizeof(host_address));
    memset(&host_netmask, 0, sizeof(host_netmask));
    memset(&target_address, 0, sizeof(target_address));
  }
  catch (const std::exception &e) {
    /* DUECA network.

       Could not interpret the URL for udp communication; the given
       address might not be correct, or the URL is not legal. Check
       your configuration and correct.
    */
    E_CNF("Failure parsing the UDP communication URL: " << spec.url);
    throw(CFErrorConstruction(e.what()));
  }
}

UDPSocketCommunicator::~UDPSocketCommunicator() { undoUDPConnection(); }

void UDPSocketCommunicator::configureHostAddress()
{
  // initial start interface address
  memset(&host_address, 0, sizeof(host_address));
  host_address.s_addr = htonl(INADDR_ANY);

  // get all available interfaces
  struct ifaddrs *interfaces;
  if (getifaddrs(&interfaces)) {
    /* DUECA network.

       Failed to get access to the network interfaces. Connection will
       fail. */
    E_NET("Cannot get inet interfaces: " << strerror(errno));
    throw(connectionfails());
  }

  // if specific interface specified, check it is there
  if (if_address.size()) {

    // let getaddrinfo convert the interface address
    struct addrinfo *ha;
    int aires = getaddrinfo(if_address.c_str(), "0", NULL, &ha);
    if (aires != 0) {
      /* DUECA network.

         Failed to interpret the host interface address. Check your
         configuration. Connection will fail. */
      E_NET("Cannot interpret the host interface address " << if_address);
      throw(connectionfails());
    }

    // must match one of my interfaces
    for (struct ifaddrs *p = interfaces; p != NULL; p = p->ifa_next) {

      // IPV4 for now, also skip when not filled
      if (p->ifa_addr == NULL || p->ifa_addr->sa_family != AF_INET)
        continue;

      // extract, and compare
      union {
        struct sockaddr_in in;
        struct sockaddr gen;
      } ifa, ifb;
      ifa.gen = *(p->ifa_addr);
      ifb.gen = *(ha->ai_addr);
      if (ifa.in.sin_addr.s_addr == ifb.in.sin_addr.s_addr) {
        host_address.s_addr = ifa.in.sin_addr.s_addr;
        /* DUECA network.

           Information on which interface is used for UDP communication.
         */
        I_NET("Selected own interface " << if_address);
        break;
      }
    }

    if (!host_address.s_addr) {
      /* DUECA network.

         The given host address is not found among the computer's
         interface addresses. Check your configuration, connection
         will fail. */
      E_NET("Could not find " << if_address << " among own interfaces");
      throw(connectionfails());
    }
  }
  else {
    /* DUECA network.

       The own network interface address was not configured. This will
       work, as long as the default interface returned by the system
       is the correct one. Verify this if your DUECA processess cannot
       connect.
    */
    W_NET("Using default interface address");
  }

  // try to find the netmask
  memset(&host_netmask, 0, sizeof(host_netmask));

  // loop through own interface and compare with specified host if, or
  // take first if not specified
  for (struct ifaddrs *p = interfaces; p != NULL; p = p->ifa_next) {

    // IPV4 for now, also skip when not filled and skip local
    // interfaces when the if_address was not specified
    if (p->ifa_addr == NULL || p->ifa_netmask == NULL ||
        p->ifa_addr->sa_family != AF_INET ||
        ((p->ifa_flags & IFF_LOOPBACK) != 0 && if_address.size() == 0) ||
        (p->ifa_flags & IFF_UP) == 0)
      continue;

    // compare to host address
    if (host_address.s_addr == 0 ||
        host_address.s_addr ==
          reinterpret_cast<sockaddr_in *>(p->ifa_addr)->sin_addr.s_addr) {
      memcpy(&host_netmask, p->ifa_netmask, sizeof(sockaddr));
      // host_netmask = *reinterpret_cast<sockaddr_in*>(p->ifa_netmask);

      // when automatically selected, warn about used interface
      if (host_address.s_addr == 0) {
        /* DUECA network.

           Using an automatically selected interface address. */
        W_NET("Automatically selected interface " << p->ifa_name);
        host_address = reinterpret_cast<sockaddr_in *>(p->ifa_addr)->sin_addr;
      }

      // done checking interfaces
      break;
    }
  }

  // check we have a netmask now
  if (host_netmask.sin_addr.s_addr == 0) {
    /* DUECA network.

       Cannot find the netmask corresponding to the given or
       automatically selected host interface. Connection will fail.
    */
    E_NET("Could not find netmask for host interface");
    throw(connectionfails());
  }
}

void UDPSocketCommunicator::configureUDPConnection(bool is_master)
{
  // configure the UDP address for sending
  struct addrinfo *ta;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  int aires = getaddrinfo(peer_address.c_str(),
                          boost::lexical_cast<std::string>(dataport).c_str(),
                          &hints, &ta);
  DEB("Address info for peer/target address "
      << peer_address << " check "
      << inet_ntoa(reinterpret_cast<sockaddr_in *>(ta->ai_addr)->sin_addr)
      << " port "
      << htons(reinterpret_cast<sockaddr_in *>(ta->ai_addr)->sin_port));

  // this should result in one address!
  if (aires || ta->ai_next != NULL) {
    /* DUECA network.

       Failed to get the address information on a given UDP
       peer. Check that the address is correct. */
    E_NET("Cannot get address info on UDP peer " << peer_address << ":"
                                                 << dataport << ", error "
                                                 << gai_strerror(aires));
    throw(connectionfails());
  }

  // check the address type
  if (isMulticastAddress(
        reinterpret_cast<sockaddr_in *>(ta->ai_addr)->sin_addr.s_addr)) {
    connection_mode = MultiCast;
  }
  else if (isBroadcastAddress(
             host_address.s_addr, host_netmask.sin_addr.s_addr,
             reinterpret_cast<sockaddr_in *>(ta->ai_addr)->sin_addr.s_addr)) {
    connection_mode = BroadCast;
  }
  else {
    connection_mode = PointToPoint;
  }
  DEB("Connection mode " << connection_mode);

  if (connection_mode == PointToPoint && is_master) {

    // free the memory, re-load the address
    freeaddrinfo(ta);

    // run again, now we know comm type, we know socket number; for
    // peer-to-peer using 2 different port numbers
    aires = getaddrinfo(peer_address.c_str(),
                        boost::lexical_cast<std::string>(dataport + 1).c_str(),
                        &hints, &ta);
    assert(aires == 0);

    DEB("Increased port number for master "
        << htons(reinterpret_cast<sockaddr_in *>(ta->ai_addr)->sin_port));
  }

  // create send socket
  comm_send = socket(ta->ai_family, ta->ai_socktype, ta->ai_protocol);
  if (comm_send == -1) {
    /* DUECA network.

       Unexpected failure to open an UDP socket. Check permissions and
       network.
    */
    W_NET("Could not open UDP socket, tried " << ta->ai_canonname << ": "
                                              << strerror(errno));
    throw(connectionfails());
  }

  // if applicable, add broadcast capability to send socket
  if (connection_mode == BroadCast) {
    int on = 1;
    if (setsockopt(comm_send, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) != 0) {
      /* DUECA network.

         It was not possible to add broadcast capability to the
         sending socket. Check permissions and interface capabilities.
      */
      W_NET("Could not add broadcast to socket: " << strerror(errno));
      throw(connectionfails());
    }
  }

  // if applicable, multicast to the sending socket
  if (connection_mode == MultiCast) {

    // request to join multicast group for this host address
    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr = reinterpret_cast<sockaddr_in *>(ta->ai_addr)->sin_addr;
    mreq.imr_interface = host_address;

    if (setsockopt(comm_send, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_interface,
                   sizeof(in_addr))) {
      /* DUECA network.

         It was not possible to add multicast capability to the
         sending socket. Check permissions and interface capabilities.
      */
      W_NET("Could not enable multicast for send socket: " << strerror(errno));
      throw(connectionfails());
    }
  }

  // priority setting
#if defined(SYMBOL_SO_PRIORITY)
  if (socket_priority > 0 && socket_priority < 15) {
    if (setsockopt(comm_send, IPPROTO_IP, SO_PRIORITY, &socket_priority,
                   sizeof(socket_priority))) {
      /* DUECA network.

         Cannot set the socket priority. Non-fatal.
      */
      W_NET("Could not set priority "
            << socket_priority << " on send socket: " << strerror(errno));
    }
  }
#else
#warning "Could not find SO_PRIORITY, no means to set sending socket priority"
#endif

  // ip tos setting
  if (lowdelay) {
    uint8_t tos_val = IPTOS_LOWDELAY;
    if (setsockopt(comm_send, IPPROTO_IP, IP_TOS, &tos_val, sizeof(tos_val))) {
      /* DUECA network.

         Cannot set the TOS (Type/Quality of Service) for the socket to
         lowdelay. Not fatal.
      */
      W_NET("Could not set lowdelay on send socket: " << strerror(errno));
    }
  }

  // bind this socket to a source address
  union {
    struct sockaddr_in in;
    struct sockaddr gen;
  } source_address;
  memset(&source_address, 0, sizeof(source_address));
  source_address.in.sin_addr = host_address;
  source_address.in.sin_family = AF_INET;
  source_address.in.sin_port = 0;
  DEB("Using host address " << inet_ntoa(host_address) << " for bind");

  if (::bind(comm_send, &source_address.gen, sizeof(source_address))) {
    /* DUECA network.

       Could not bind the sending UDP socket to the specified source
       address. Will next attempt to bind to *any* source address.
    */
    W_NET("Could not bind sending UDP socket: " << strerror(errno));

    // try with zero??
    memset(&source_address, 0, sizeof(source_address));
    if (::bind(comm_send, &source_address.gen, sizeof(source_address.in))) {
      /* DUECA network.

         Could not bind the sending UDP socket to *any* source
         address. This means the connection will fail. Check
         networking and permissions.
      */
      E_NET("Could not bind sending UDP socket: " << strerror(errno));
      throw(connectionfails());
    }
  }

  // remember the target address
  target_address = *(ta->ai_addr);

  // configure the receive address
  struct addrinfo *ra;
  uint16_t dataport2 = dataport;
  if (!is_master && connection_mode == PointToPoint) {
    dataport2++;
  }

  aires = getaddrinfo(peer_address.c_str(),
                      boost::lexical_cast<std::string>(dataport2).c_str(),
                      &hints, &ra);
  assert(aires == 0);
  comm_recv = socket(ra->ai_family, ra->ai_socktype, ra->ai_protocol);
  DEB(
    "Receive address configured as "
    << inet_ntoa(reinterpret_cast<sockaddr_in *>(ra->ai_addr)->sin_addr)
    << " port " << htons(reinterpret_cast<sockaddr_in *>(ra->ai_addr)->sin_port)
    << " family " << reinterpret_cast<sockaddr_in *>(ra->ai_addr)->sin_family);

  // if applicable, set re-use
  if (port_re_use) {
    DEB("selecting re-use on the receive socket");

    /* DUECA network.

       Information message that receive socket re-use is
       selected. Configure this only for multiple DUECA nodes on a
       single computer, otherwise do not enable re-use. */
    I_CNF("selecting re-use on the receive socket");
    int on = 1;
    if (setsockopt(comm_recv, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
                   sizeof(on)) != 0) {
      /* DUECA network.

         It was not possible to configure re-use on the receive
         socket. Will continue attempting to communicate, note that
         this will fail if multiple DUECA processes on the same
         computer need to read from this same socket. */
      W_NET("Cannot set re-use: " << strerror(errno));
    }
  }

  if (connection_mode == MultiCast) {

    // request to join multicast group for this host address
    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    inet_aton(peer_address.c_str(), &mreq.imr_multiaddr);
    mreq.imr_interface = host_address;

    if (setsockopt(comm_recv, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   reinterpret_cast<char *>(&mreq), sizeof(mreq)) != 0) {
      /* DUECA network.

         Failed to join the multicast group for the reading
         socket. Check permissions and network
         configuration. Connection will fail.
      */
      W_NET("Could not join multicast group: " << strerror(errno));
      throw(connectionfails());
    }
  }

  if (::bind(comm_recv, ra->ai_addr, ra->ai_addrlen) == -1) {
    /* DUECA network.

       Failed to bind the UDP reception socket. Check permissions and
       network configuration. Connection will fail.
    */
    W_NET("Could not bind UDP reception socket: " << strerror(errno));

    throw(connectionfails());
  }

  static const char *conmode[] = { "PointToPoint", "MultiCast", "BroadCast",
                                   "Undetermined" };
  /* DUECA network.

     Information on the opened UDP socket connection. */
  I_NET("Opened UDP socket pair as "
        << conmode[int(connection_mode)] << " send: "
        << inet_ntoa(reinterpret_cast<sockaddr_in *>(ta->ai_addr)->sin_addr)
        << ":" << ntohs(reinterpret_cast<sockaddr_in *>(ta->ai_addr)->sin_port)
        << " receive: "
        << inet_ntoa(reinterpret_cast<sockaddr_in *>(ra->ai_addr)->sin_addr)
        << ":"
        << ntohs(reinterpret_cast<sockaddr_in *>(ra->ai_addr)->sin_port));

  // free the memory
  freeaddrinfo(ta);
  freeaddrinfo(ra);
}

void UDPSocketCommunicator::undoUDPConnection()
{
  switch (connection_mode) {
  case MultiCast: {
    struct in_addr ifaddr;
    memset(&ifaddr, 0, sizeof(ifaddr));
    ifaddr.s_addr = INADDR_ANY;
    if (setsockopt(comm_send, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr,
                   sizeof(ifaddr))) {
      /* DUECA network.

         Failed to unset the multicast capability from the socket. */
      W_NET("Could not unset multicast interface");
    }

    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    inet_aton(peer_address.c_str(), &mreq.imr_multiaddr);
    mreq.imr_interface = host_address;
    if (setsockopt(comm_recv, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                   reinterpret_cast<char *>(&mreq), sizeof(mreq)) != 0) {

      /* DUECA network.

         Failed to leave the multicast group. */
      W_NET("Could not leave multicast group: " << strerror(errno));
    }
  } break;

  case BroadCast: {
    int on = 0;
    if (setsockopt(comm_send, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) != 0) {
      /* DUECA network.

         Failed to unset the broadcast capability from the socket. */
      W_NET("Could not remove broadcast from socket: " << strerror(errno));
    }
  } break;
  case Undetermined:
  case PointToPoint:
    break;
  }
  close(comm_send);
  close(comm_recv);
  comm_recv = comm_send = -1;
}

void UDPSocketCommunicator::send(MessageBuffer::ptr_type buffer)
{
  DEB1("Node " << peer_id << " sending size " << buffer->fill << " cycle "
               << buffer->message_cycle);
  sendto(comm_send, buffer->buffer, buffer->fill, 0, &target_address,
         sizeof(target_address));
}

UDPSocketCommunicator::SenderINET::SenderINET(uint32_t address, uint16_t port) :
  address(address),
  port(port)
{}

std::ostream &UDPSocketCommunicator::SenderINET::print(std::ostream &os) const
{
  in_addr inaddr;
  inaddr.s_addr = address;
  os << inet_ntoa(inaddr) << ":" << port;
  return os;
}

bool UDPSocketCommunicator::SenderINET::operator<(const SenderINET &o) const
{
  if (address < o.address) {
    return true;
  }
  else if (address > o.address) {
    return false;
  }
  return (port < o.port);
}

bool UDPSocketCommunicator::SenderINET::operator==(const SenderINET &o) const
{
  return address == o.address && port == o.port;
}

void UDPSocketCommunicator::flush()
{
  // set-up for select
  fd_set socks;
  FD_ZERO(&socks);
  FD_SET(comm_recv, &socks);
  struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };

  // dummy spaces for receive
  MessageBuffer::ptr_type buffer = getBuffer();
  union {
    struct sockaddr_in in;
    struct sockaddr gen;
  } peer_ip;
  socklen_t peer_ip_len = sizeof(peer_ip.in);

  // use select to check for data
  int sres = select(comm_recv + 1, &socks, NULL, NULL, &timeout);

  // read (a fraction of the data) while no timeout
  while (sres) {
    ssize_t nbytes = recvfrom(comm_recv, buffer->buffer, buffer->capacity, 0,
                              &peer_ip.gen, &peer_ip_len);

    if (nbytes == -1) {
      /* DUECA network.

         Unexpected run-time receive error on UDP data, while trying
         to flush old messages, check the network. */
      W_NET("UDP receive error for flush: " << strerror(errno));
      throw(packetcommunicationfailure(strerror(errno)));
    }

    if (nbytes) {
      int i_peer_id = NetCommunicator::ControlBlockReader::decodePeerId(buffer);
      SenderINET id(peer_ip.in.sin_addr.s_addr, htons(peer_ip.in.sin_port));
      DEB("Initial flushing UDP message from host/port: "
          << id << " claiming id: " << i_peer_id);
    }

    sres = select(comm_recv + 1, &socks, NULL, NULL, &timeout);
  }
  returnBuffer(buffer);
}

std::pair<int, ssize_t> UDPSocketCommunicator::receive()
{
  // set-up for select
  fd_set socks;
  FD_ZERO(&socks);
  FD_SET(comm_recv, &socks);
  struct timeval timeout = default_timeout;

  // use select to check for data
  int sres = select(comm_recv + 1, &socks, NULL, NULL, &timeout);

  // timeout, no data
  if (sres == 0) {
    return std::make_pair(int(-1), ssize_t(0));
  }

  // get a buffer
  MessageBuffer::ptr_type buffer = getBuffer();

  // preparation for getting the sender's address
  union {
    struct sockaddr_in in;
    struct sockaddr gen;
  } peer_ip;
  socklen_t peer_ip_len = sizeof(peer_ip.in);

  // get the actual data
  ssize_t nbytes = recvfrom(comm_recv, buffer->buffer, buffer->capacity, 0,
                            &peer_ip.gen, &peer_ip_len);

  // check on OK?
  if (nbytes == -1) {
    /* DUECA network.

       Unexpected run-time receive error on UDP data, check the
       network. */
    W_NET("UDP receive error: " << strerror(errno));

    returnBuffer(buffer);
    throw(packetcommunicationfailure(strerror(errno)));
  }

  // check if this is connected to previous sender
  SenderINET id(peer_ip.in.sin_addr.s_addr, ntohs(peer_ip.in.sin_port));
  auto pp = peers.find(id);

  // first message, detect sender ID from message
  if (pp == peers.end() && buffer->capacity >= 6) {

    // all messages should have cycle count + node id as first entries
    int i_peer_id = NetCommunicator::ControlBlockReader::decodePeerId(buffer);

    // check for trouble
    for (auto const &p : peers) {
      if (p.second == i_peer_id) {
        /* DUECA network.

           Multiple UDP senders are using the same send ID. This may
           indicate multiple DUECA processes using the same network
           address, or leftover nodes from an old run continuing to
           send. Check the network traffic, clear old nodes, or change
           to another UDP network address.
        */
        E_NET("UDP receive multiple senders with ID "
              << i_peer_id << " existing " << p.first << " new: " << id);
        throw(packetcommunicationfailure("Multiple senders with same ID"));
      }
    }

    /* DUECA network.

       Indication that a first message from a specific peer has been
       received. */
    I_NET("First message from peer " << i_peer_id << " at " << id);

    DEB("Found new peer " << inet_ntoa(peer_ip.in.sin_addr) << ":" << id.port
                          << " node ID=" << i_peer_id);

    // remember this network address and peer ID
    pp = peers.insert(std::make_pair(id, i_peer_id)).first;
  }

  buffer->fill = nbytes;
  buffer->origin = pp->second;

  // callback with the buffer
  if (pass_data && nbytes) {
    DEB1("Node " << peer_id << " receiving size " << buffer->fill << " from "
                 << pp->second);
    (*callback)(buffer);
  }
  else if (pass_data) {
    // probably a true timeout
    DEB("Node " << peer_id << " timeout, receiving no data ");
    returnBuffer(buffer);
    return std::make_pair(int(-1), ssize_t(0));
  }
  else {
    pass_data = true;
    return this->receive();
  }

  return std::make_pair(pp->second, nbytes);
}

UDPSocketCommunicatorMaster::UDPSocketCommunicatorMaster(
  const PacketCommunicatorSpecification &spec) :
  UDPSocketCommunicator(spec)
{
  configureHostAddress();
  configureUDPConnection(true);
}

UDPSocketCommunicatorMaster::~UDPSocketCommunicatorMaster()
{
  // all performed by parent
}

UDPSocketCommunicatorPeer::UDPSocketCommunicatorPeer(
  const PacketCommunicatorSpecification &spec) :
  UDPSocketCommunicator(spec)
{
  configureHostAddress();
  configureUDPConnection(false);
}

UDPSocketCommunicatorPeer::~UDPSocketCommunicatorPeer()
{
  // all performed by parent
}

DUECA_NS_END;
