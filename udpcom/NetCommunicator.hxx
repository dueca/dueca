/* ------------------------------------------------------------------   */
/*      item            : NetCommunicator.hxx
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

#ifndef NetCommunicator_hxx
#define NetCommunicator_hxx

#include <map>
#include <netinet/in.h>

#include <memory>

#include <dueca.h>
#include <dueca/MessageBuffer.hxx>
#include <dueca/AsyncList.hxx>
#include "PacketCommunicator.hxx"
#include "CycleCounter.hxx"

DUECA_NS_START;

/** Generic TCP/IP + packet (UDP or websocket) communication base class.

    This maintains communication with one or more peers, verifying
    message arrival, with a re-send option when messages are lost.

    Derived classess NetCommunicatorMaster and NetCommunicatorPeer
    implement master functionality and peer functionality. The master
    opens a TCP/IP service to transmit configuration requests, and
    manages connections and start instructions for participation in
    the data transmission.

    Real-time data communication is over UDP or Websocket,
    with robustness against lost packages.

    @bug The implementation is not yet robust against a combination of
    aggressively creating and deleting entries (as stress-tested in
    TestMultiStream), and numerous lost packages (stress-tested with
    the -DBUILD_TESTOPT=ON option).

    Headers in the package have the following data:

    - uint16_t crc check
    - uint32_t group magic number
    - int32_t  sending offset, microseconds
    - uint32_t cycle counter
    - uint16_t (sending) peer id
    - uint16_t number of peers in this message cycle
    - uint32_t peer tick, time indication of peer

    Any header data beyond that is dependent on the client implementation.

    Sending logic and recovery is as follows:
    - on timeout at the master, the sending enters a recovery cycle where
      the last set of data is repeated. A nibble of the cycle counter is
      used to rotate through the repeats.
    - on receive failure at a peer (incomplete set of messages), a recovery
      request is sent. The master repeats the *previous* message cycle, and
      a flag in that message indicates that the peers should cycle back as
      well, until that gets a confirm, and then continues with the message
      cycle in which the recovery was requested, then continues to
      normal sending.

    Sending error scenarios

    - message arrives late at a peer
      * the sending cycle is not completed in time, part of the messages
        may still be sent
      * the master sends a repeat cycle, with an update in the nibble
      * peers start reacting to the repeated cycle initiated by the master
      * all peers will react with or without error flag. Once the master
        has messages from all peers, normal sending continues if no error
        flag, or recovery as in the next description

    - message does not arrive at a peer
      * if an intermediate peer, and the message just before sending,
        the message cycle breaks, and we are back to a recovery scenario
        as above
      * if a peer that did not wait on this message for sending misses
        the message, it is detected at the next cycle, the peer sends
        a missed data flag
      * the master cycles back to the backup buffer (previous message),
        the message before that was received and confirmed, since the
        backup buffer only became backup after getting confirmation
        on that cycle. backup buffer is sent with an error flag
      * all nodes receiving the master message also switch to the
        backup buffer and send that.
      * if the cycle times out, the backup buffer is simply re-sent again
      * if the cycle does not time out (and new error flags from peers must
        be impossible), the buffer from the cycle in which the error was
        flagged is re-sent. Error recovery to backup is again possible
        if the error is flagged again.

    - peer crashes
      * one of the peers crashes, and will not send in a specific cycle
      * since the message is not sent, the master has a timeout
      * the master repeats the current message
      * the peer is connected through a configuration connection, and
        its breakage will be noticed by the master
      * when disappearance of the peer is noted, the number of peers in
        the master is reduced. peers will follow the master's lead on this
      * the master now concludes there are enough responses, and if no
        error is flagged on the previous cycle, the master advances to
        normal sending again

    Derived classes need to finalize virtual functions
    clientPackPayload and clientUnpackPayload, these receive and fill
    the UDP/Websocket/?? data. Additional functions to implement and
    extend the configuration communication over the TCP/IP server are
    defined in derived classes NetCommunicatorMaster and
    NetCommunicatorPeer.

    Derived classes DuecaNetMaster and DuecaNetPeer implement objects
    for communications between DUECA nodes in the same process/simulation.

    ChannelReplicatorMaster and ChannelReplicatorPeer implement
    communication between DUECA processes, i.e., an HLA-like service.
 */
class NetCommunicator: public PacketCommunicatorSpecification
{
protected:
  /** @defgroup configurationboth Configuration values for peer and
      master @{ */

  /** cycle timing in granule increments */
  TimeTickType                        ts_interval;

  /** Network configuration communication server port */
  uint16_t                            master_port;

  /** Buffer size */
  uint32_t                            config_buffer_size;

  /** Communication packet implementation */
  boost::intrusive_ptr<PacketCommunicator> data_comm;

  /** Target address for UDP connection, point-to-point, broadcast or
      multicast, compatibility for older configuration */
  std::string                         peer_address;

  /** Port for udp messages, for compatibility for older configuration
      files */
  uint16_t                            dataport;

  /** @} */

public:

  /** @defgroup controldata Definition of the control data in the data
      packets @{ */

  /** Size of control block */
  static const size_t                 control_size;

  /** Group magic number, to reduce the chance that multiple DUECA 
      processes interfere. */
  uint32_t                            group_magic;

  /** Object to write the data information for data packet

      Note that any changes to the control block invalidate communication
      with other versions. Also, keep the Writer and Reader in sync. */
  class ControlBlockWriter
  {
    /** Keep reference to the buffer where the packet is written */
    MessageBuffer::ptr_type buffer;

    /** Store for packing the control block */
    AmorphStore             s;

    /** Remember the location of the checksum data */
    StoreMark<uint16_t>     checksum_mark;

    /** Remember the location for the timing offset */
    StoreMark<int32_t>      usecsoffset_mark;

    /** Remember the tick corresponding to this message */
    TimeTickType            tick;
  public:
    /** Constructor for control data writing.

        Writes the supplied data in the buffer's first bytes, and
        reserves room for the CRC check value.

        @param buffer      Buffer for data
        @param group_magic Identifying number for communication group
        @param cycle       Tick number/data counter number
        @param peer_id     Identification of sender
        @param npeers      Number of peers in the given cycle
        @param tick        DUECA time tick of the cycle
        @param error       Add an error flag (missed message, timeout,
                           etc.), requesting a re-send.
    */

    ControlBlockWriter(MessageBuffer::ptr_type buffer,
                       uint32_t group_magic,
                       const dueca::CycleCounter& cycle,
                       uint16_t peer_id, uint16_t npeers,
                       TimeTickType tick, bool error);

    /** Mark the offset time */
    void markTimeOffset(double net_permessage, double net_perbyte);

    /** Mark the send time */
    void markSendTime();

    /** Destructor for control data writing.

        This checks the fill level of the buffer, calculates the CRC
        checksum over all data except the first two bytes, and writes
        the CRC. */
    ~ControlBlockWriter();
  };

  /** Decode a control block */
  struct ControlBlockReader
  {
    /** Buffer for decoding */
    AmorphReStore r;

    /** CRC value from data */
    uint16_t crcvalue;

    /** Sending time offset in microseconds */
    int32_t usecs_offset;

    /** Group magic number */
    uint32_t group_magic;

    /** Tick number/data counter number */
    dueca::CycleCounter cycle;

    /** ID for sending peer */
    uint16_t peer_id;

    /** Number of peers in the given cycle */
    uint16_t npeers;

    /** DUECA time tick of the cycle, for the given peer */
    TimeTickType peertick;

    /** Error flagged (missed data, timeout) */
    bool errorflag;

    /** Error in CRC decoding */
    bool crcgood;

    /** Create an object with the message control data

        @param buffer     Buffer with data
    */
    ControlBlockReader(MessageBuffer::ptr_type buffer);

    /** Only decode the peer ID

        @param buffer     Buffer with data
    */
    static uint16_t decodePeerId(MessageBuffer::ptr_type buffer);
  };
  /** @} */

protected:
  /** Type for map with current read message cycles */
  typedef std::map<uint16_t,dueca::CycleCounter> peer_cycles_type;

  /** Map keeping message read cycles for messages from all peers,
      ensuring that messages are processed in sequence, and read only
      once.
  */
  peer_cycles_type                    peer_cycles;

  /** Global communication cycle counter. The master updates it's
      message cycle if confirmation has been received that all peers
      completed a message, the peers follow the cycle
      indicated/transmitted by the master.
  */
  dueca::CycleCounter                 message_cycle;

  /** This remembers the cycle for which the last message was packed
      on this node. It never counts back; and depending on the
      difference between the requested recovery, and this cycle,
      either the normal buffer is sent, or the recovery. */
  dueca::CycleCounter                 packed_cycle;

  /** Current buffer location */
  MessageBuffer::ptr_type             current_send_buffer;

  /** Back-up buffer location, in preparation for recovery cycles. */
  MessageBuffer::ptr_type             backup_send_buffer;

  /** Testing modes (Note: testing variables could be removed from the
      class when testing is not compiled in, but the potential for
      memory corruption with preprocessor-induced class changes is so
      large ...
  */
  enum class FailureSimulation {
    NoFailure,              /**< All Ok */
    ExtraDelay,             /**< Simulate some additional delay in sending */
    FailSend,               /**< Simulate a failed/missing packet */
    Coalesce,               /**< Coalesce the packet with one other */
    CoalesceTwice,          /**< With two others */
    CoalesceTriple          /**< With three others */
  };

  /** Simulated failure. */
  FailureSimulation failure;

  /** Backlog of messages for testing coalescing */
  AsyncList<MessageBuffer::ptr_type>  coalescing_backlog;

  /** Used, spare buffers for the coalescing backlog */
  AsyncList<MessageBuffer::ptr_type>  coalescing_reserve;

public:
  /** Logic for sending end */
  enum SendState {
    Normal,          /**< normal operation; pack a new buffer and send */
    Recover,         /**< re-send the back-up buffer, after backup request */
    Stasis,          /**< re-send the current buffer */
    AfterNormal      /**< after a normal send, stasis-like */
  };

protected:
  /** Current send state */
  SendState                           sendstate;

  /** Flag cycle error (timeout, peer flags missed data), to re-start
      recovery */
  bool                                trigger_recover;

  /** Number of current peers in UDP communication */
  unsigned                            npeers;

  /** This entry's send id */
  uint16_t                            node_id;

  /** Error bit indicating request for resend */
  uint16_t                            errorbit;

protected:
  /** Constructor */
  NetCommunicator();

  /** Destructor */
  ~NetCommunicator();

protected:

  /** Code and send new data, or re-send a previous message after
      failure */
  size_t codeAndSendUDPMessage(TimeTickType current_tick);

protected:
  /** @defgroup clientcalls Callbacks for filling and extracting payload data.

     @{
  */

  /** Pack payload into send buffer

      Pack the payload. The client is responsible for handling the
      maximum buffer size, and possibly stacking up or ignoring excess
      data.

      @param buffer Payload buffer object
   */
  virtual void clientPackPayload(MessageBuffer::ptr_type buffer) = 0;

  /** Accept a loaded buffer for unpacking

      This receives the data communication buffer. The client payload
      is located in the buffer from NetCommunicator::control_size
      onwards. The buffer is initially assigned to one unpacking
      client process. With a call to the claim method, the buffer may
      be passed to a secondary process. Use the returnBuffer command
      to return the buffer, for each additional claim, an additional
      returnBuffer command is needed, enabling returning access to the
      buffer from multiple threads.

      Override this function in your derived class to implement the
      application's data processing.

      @param buffer       Payload buffer object
      @param peer_id      Peer sending the buffer's data
      @param current_tick Own process tick value
      @param peertick     Peer's tick value
  */
  virtual void clientUnpackPayload(MessageBuffer::ptr_type buffer,
                                   unsigned peer_id,
                                   TimeTickType current_tick,
                                   TimeTickType peertick,
                                   int usecoffset) = 0;

  /** Add timing information to the message */
  virtual void communicatorAddTiming(ControlBlockWriter &cb);
};



DUECA_NS_END;

/** Print the sendstate as a readable string */
std::ostream& operator << (std::ostream& os,
                           const NetCommunicator::SendState& x);

#endif
