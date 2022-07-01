/* ------------------------------------------------------------------   */
/*      item            : UDPSocketCommunicator.hxx
        made by         : Rene van Paassen
        date            : 200412
        category        : header file
        description     :
        changes         : 200412 first version
        language        : C++
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef UDPSocketCommunicator_hxx
#define UDPSocketCommunicator_hxx

#include <string>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>

#include <dueca/MessageBuffer.hxx>
#include <dueca/SharedPtrTemplates.hxx>

#include "PacketCommunicator.hxx"

DUECA_NS_START;

/** Common elements for master and slave UDP communication

    This implements UDP communication, point-to-point (2 nodes only),
    multicast or broadcast. The type of communication depends on the
    entered address.

    The url field in the PacketCommunicatorSpecification argument
    should have the form:

    @code
    udp://hostname-or-ipaddress:port
    @endcode

    For point-to-point, the master receives on (port), and sends on
    (port + 1), the peer receives on (port + 1) and sends on (port). For
    broadcast and multicast, a single port is used. 

    port_re_use is for testing purposes generally, running multiple
    DUECA on a single computer.
 */
class UDPSocketCommunicator: public PacketCommunicator
{
protected:

  /** Network configuration communication server port */
  uint16_t                            master_port;

  /** Own interface address */
  std::string                         if_address;

  /** Set port re-use, in general for testing */
  bool                                port_re_use;

  /** Try lowdelay option for TOS */
  bool                                lowdelay;

  /** Socket priority for sending */
  int                                 socket_priority;

  /** @defgroup configurationmaster Configuration values for master
      @{ */

  /** Target address for UDP connection, point-to-point, broadcast or
      multicast */
  std::string                         peer_address;

  /** Port for udp messages */
  uint16_t                            dataport;
  /** @} */

  /** Sending socket for udp packages */
  int                                 comm_send;

  /** Receiving socket for udp packages */
  int                                 comm_recv;

  /** Type of connection (deduced from target address), point-to-point,
      multicast or broadcast */
  enum ConnectionType {
    PointToPoint,    /** directed, limited to master-slave */
    MultiCast,       /** multicast address detected */
    BroadCast,       /** assumed when matching broadcast pattern */
    Undetermined     /** did not check yet */
  };

  /** Type of connection (deduced from target address), point-to-point,
      multicast or broadcast */
  ConnectionType                      connection_mode;

  /** Timeout value for message cycle */
  struct timeval                      default_timeout;

  /** Current host address struct, for binding */
  in_addr                             host_address;

  /** Host netmask */
  struct sockaddr_in                  host_netmask;

  /** Target address, will be filled through getaddrinfo */
  struct sockaddr                     target_address;

public:
  /** Structure for tagging a sender's internet details */
  struct SenderINET {
    uint32_t       address;
    uint16_t       port;
    SenderINET(uint32_t, uint16_t);
    bool operator < (const SenderINET& o) const;
    bool operator == (const SenderINET& o) const;
    std::ostream& print (std::ostream&) const;
  };
protected:

  /** Map with peer ID's */
  std::map<SenderINET,int>            peers;

protected:
  /** Constructor */
  UDPSocketCommunicator(const PacketCommunicatorSpecification& spec);

  /** Destructor */
  ~UDPSocketCommunicator();

  /** detect the host address to use */
  void configureHostAddress();

  /** complete function */
  void configureUDPConnection(bool is_master);

  /** undo udp connection */
  void undoUDPConnection();

public:
  /** Code and send new data, or re-send a previous message after
      failure */
  void send(MessageBuffer::ptr_type buffer) final;

  /** Block on and receive data

      @returns Number of bytes received */
  std::pair<int,ssize_t> receive() final;


  /** Flush initial data, to make sure no old messages taint this
      communication */
  void flush() override;
};


class UDPSocketCommunicatorMaster:
  public UDPSocketCommunicator
{
public:
  /** Constructor */
  UDPSocketCommunicatorMaster(const PacketCommunicatorSpecification& spec);

  /** Destructor */
  ~UDPSocketCommunicatorMaster();
};


class UDPSocketCommunicatorPeer:
  public UDPSocketCommunicator
{

public:
  /** Constructor */
  UDPSocketCommunicatorPeer(const PacketCommunicatorSpecification& spec);

  /** Destructor */
  ~UDPSocketCommunicatorPeer();
};


DUECA_NS_END;

PRINT_NS_START;
inline ostream & operator <<
(ostream&os,
 const dueca::UDPSocketCommunicator::SenderINET& si)
{ return si.print(os); }
PRINT_NS_END;

#endif
