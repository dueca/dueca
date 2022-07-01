/* ------------------------------------------------------------------   */
/*      item            : NetCommunicatorMaster.hxx
        made by         : Rene van Paassen
        date            : 170912
        category        : header file
        description     :
        changes         : 170912 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NetCommunicatorMaster_hxx
#define NetCommunicatorMaster_hxx

#include "NetCommunicator.hxx"
#include "ConfigBuffer.hxx"
#include <UDPPeerAcknowledge.hxx>

DUECA_NS_START;

class WebsockCommunicatorConfig;

/** Master in the communication */
class NetCommunicatorMaster: public NetCommunicator
{
protected:
  /** @defgroup configurationmaster Configuration value for master
      @{ */

  /** Interval for checking socket completion */
  unsigned                            connect_check_interval;

  /** URL for configuration messages */
  std::string                         config_url;

  /** Optional override for data message URL */
  std::string                         public_data_url;
  /** @} */

private:

  /** Is communication active? */
  volatile bool                       communicating;

  /** Is the config socket completed? */
  unsigned                            server_needsconnect;

  /** Connection for configuration messages */
  std::shared_ptr<WebsockCommunicatorConfig>
                                      conf_comm;
protected:

  /** Information about communication peer */
  struct CommPeer {

    /** Remember state of peer */
    enum PeerState {
      Vetting, /** Needs to be vetted first */
      Wait,    /** Wait some cycles before participating  */
      Active,  /** Active */
      Broken   /** No connection to configuration */
    };

    /** State of this peer */
    PeerState                         state;

    /** filtered time difference */
    double                            delta_time;

    /** peer sender ID */
    uint32_t                          send_id;

    /** peer sender ID */
    uint32_t                          follow_id;

    /** Communication buffer, one per peer */
    ConfigBuffer                      commbuf;

    /** Peer's internet address, for confirmation */
    std::string                       address;

    /** Constructor */
    CommPeer(unsigned sendid, unsigned previd,
             const std::string& address);

    /** Destructor, closes the socket. */
   ~CommPeer();
  };

  /** Type for list of peers */
  typedef std::list<std::shared_ptr<CommPeer> >  peerlist_type;

  /** Map for communication peers */
  peerlist_type                       peers;

private:

  /** The node id that sends the last message */
  unsigned                            next_peer_id;

  /** Number of peers change plan cycle information */
  struct ChangeCycle {

    /** Cycle when number of peers changes */
    uint32_t                          change_cycle;

    /** Involved peer */
    uint16_t                          peer;

    /** Add or remove */
    bool                              addition;

    /** Constructor */
    ChangeCycle(uint32_t change_cycle, uint16_t peers, bool add);
  };

  /** List type, for FIFO. */
  typedef std::list<ChangeCycle>     peerchange_type;

  /** The list, used as FIFO */
  peerchange_type                    peer_changes;

protected:

  /** Estimate of time per byte, usecs */
  double net_perbyte;

  /** Estimate of setup time per message, usecs */
  double net_permessage;

  /** Time constant perbyte estimation */
  double net_tau1;

  /** Time constant per message estimation */
  double net_tau2;

private:
  /** previous cycle's time */
  int last_cycle_time;

  /** previous cycle's bytes */
  int last_cycle_bytes;

  /** current cycle tick */
  TimeTickType current_tick;

  /** number of received messages in current cycle */
  unsigned nreceived;

protected:
  /** Constructor */
  NetCommunicatorMaster();

  /** Completion with configured parameters */
  bool complete();

  /** Destructor */
  ~NetCommunicatorMaster();

private:

  /** Configure data connections and assign peer ID

      @param address   The network address as detected by websock server
      @return          The assigned peer id
   */
  int assignPeerId(const std::string& address);

  /** For the master node, decode and process connect requests

      Upon a new connection, the clientInfoPeerJoined function is called.

      @param ts    TimeSpec for the current cycle
  */
  void checkNewConnections(const TimeSpec& ts);

  /** Send the configuration as is now

      The clientWelcomeConfig callback can add application-dependent information

      @param peer   Information on the peer.
      @param join_cycle Cycle at which the communication is to be joined.
   */
  void sendCurrentConfigToPeer(const CommPeer& peer,
                               TimeTickType join_cycle);

  /** modify which sender to follow */
  void changeFollowId(const CommPeer& peer, uint32_t target_cycle=0U);

  /** check configuration requests from peers */
  void checkAndUpdatePeerStates(const TimeSpec& ts);

protected:
  /** send config to all peers */
  void distributeConfig(AmorphStore& s);

private:
  /** instruct a following peer when one in the middle drops out

      @param send_id    ID of the peer dropping out
      @param follow_id  New peer to be followed
   */
  void correctFollowId(uint32_t send_id, uint32_t follow_id);

  /** Start TCP server for connections */
  bool startServer();

  /** Stop TCP server and created connections */
  void stopServer();

  /** Decode configuration data from a peer */
  void decodeConfigData(CommPeer& peer);

protected:

  /** helper function, sends out set of data over TCP connection */
  void flushStore(AmorphStore& s, unsigned peer_id);

  /** @defgroup clientcalls Calls for using this class, except for data
      pack/unpack, defined in NetCommunication.hxx

      @{
  */

  /** Information on events for descendants */
  virtual void clientInfoPeerJoined(const std::string& address, unsigned id,
                                    const TimeSpec& ts);

  /** Information on events for descendants */
  virtual void clientInfoPeerLeft(unsigned id, const TimeSpec& ts);

  /** Additional peer data to add to welcome message

      Pack data in the store, when full, flush the store to the given tcp socket.

      @param s      Store
      @param id     Numeric send id for the peer
      @param tcp_socket  Socket for flushing full stores
 */
  virtual void clientWelcomeConfig(AmorphStore& s, unsigned id) = 0;

  /** decode configuration payload.

      Decode one object or message; when decode fails, restore the config buffer.

      @param s      Buffer with config data
      @param id     Id for the sending client
  */
  virtual void clientDecodeConfig(AmorphReStore&s, unsigned send_id) = 0;

  /** encode configuration payload.

      Encode multiple objects or messages. Encode each object preceded
      by a UDPPeerConfig::ClientPayload message. When the buffer
      overflows, reset to previous state, and send with the sendConfig
      call. Then continue with cleared buffer.

      @param s      Buffer with config data
      @param id     ID for the target, if 0, send to all connected
 */
  virtual void clientSendConfig(const TimeSpec& ts, unsigned id) = 0;

  /** Vet peer joining */
  enum VettingResult {
    Delay,  /**< result not in, keep */
    Reject, /**< remove */
    Accept  /**< accept */
  };

  /** Approve or decline peer joining.

      @param id  Peer ID */
  virtual VettingResult clientAuthorizePeer(CommPeer& peer,
                                            const TimeSpec& ts);

  /** @} end of the group */

  /** Do a single IO cycle

      @param act    activity, used for signaling blocking actions
  */
  void doCycle(const TimeSpec& ts, Activity& act);

  /** Unpack the data

      @param buffer Buffer with data
   */
  void unpackPeerData(MessageBuffer::ptr_type& buffer);

  /** Break links */
  void breakCommunication();

  /** add communicator timing */
  void communicatorAddTiming(ControlBlockWriter &cb) final;
};

DUECA_NS_END;

#endif
