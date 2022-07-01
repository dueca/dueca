/* ------------------------------------------------------------------   */
/*      item            : IPAccessor.hh
        made by         : Rene' van Paassen
        date            : 990611
        category        : header file
        description     :
        changes         : 990611 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef IPAccessor_hh
#define IPAccessor_hh

#include "Trigger.hxx"
#include "Activity.hxx"
#include "TimeSpec.hxx"
#include "SimTime.hxx"
#include "CyclicInt.hxx"
#include "Callback.hxx"
#include "AsyncQueueMT.hxx"
#include "MessageBuffer.hxx"
//#ifdef HAVE_SYSTIME_H
#include <sys/time.h>
//#endif
#include "Accessor.hxx"
#include <fstream>
class ActivityManager;

#define MAXHOSTNAME 256

#include <dueca_ns.h>
DUECA_NS_START

class Packer;
class Unpacker;
class FillPacker;
class FillUnpacker;
class PeriodicTimeSpec;
class TransportDelayEstimator;
class TimingCheck;


/** A base class for communication accessors that somehow use IP
    communication (broadcast, unicast, multicast). */
class IPAccessor :
  public Accessor,
  private TriggerPuller
{
  /** A sequence number, so different accessors get unique names. */
  static int sequence;

protected:
#if 0
  /** The object that fills the data storage that has to be sent. */
  Packer        *packer;

  /** If there is any space left, the fill packer might fill it with
      low-priority data. */
  FillPacker    *fill_packer;

  /** The object that unpacks the data that has come in. */
  Unpacker      *unpacker;

  /** Unpacks the data sent by other fill packers. */
  FillUnpacker  *fill_unpacker;
#endif

  /** Storage area for packing the to-be-sent data and sending it. */
  char **output_store;

  /** Status flags for each of the sending areas. */
  int *output_status;

  /** A cyclic counter that keeps track of the sending area currently
      in use. \todo Currently not being updated, see the store_to_send
      variable. */
  CyclicInt store_in_use;

  /** Integer to keep track of the store to be sent. Updated by the
      packer. */
  int store_to_send;

  /** Number of bytes to send from the current store. */
  int bytes_to_send;

  /** For incoming data, the number of data bytes in a message, as
      calculated from the message size. */
  uint16_t regular_bytes;

  /** The number of data bytes in a message, as coded in the message. */
  uint16_t data_says_regular_bytes;

  /** Sizes for the input and output stores. \{ */
  int output_store_size, input_store_size;   /// \}

  /** Sizes for the input and output packets, maximum. \{ */
  int output_packet_size;  /// \}

  /** Number of input buffers. */
  int n_input_stores;

  /** Number of output buffers. */
  int n_output_stores;

  /** Pointer, to allocate an array of current packet counter, one for
      each party in the communication. */
  uint32_t *current_packet_no;

  /** Order in the send sequence, and the total number of sending
      parties. \{ */
  int send_order, no_of_senders;  /// \}

  /** Highest socket file descriptor, for select operation. */
  int sockfd_high;

  /** Timeout value, in microseconds. */
  int timeout;

  /** Pointer to allocate an array of file descriptors. */
  int *sockfd;

  /** Sending address, for sending the data. */
  union {
    sockaddr_in set;
    sockaddr    get;
  } target_address_data;

  /** Sending address, for sending the confirmation. */
  union {
    sockaddr_in set;
    sockaddr    get;
  } target_address_confirm;

  /** Timing specification for the send cycle. */
  PeriodicTimeSpec time_spec;

  /** Counter to determine in what phase of the IO cycle we are (who
      is sending/confirming. */
  int io_cycle_phase;

  /** No of bytes received in the last packet. */
  int bytes;

  /** Flags to keep up with the logic of IO cycles. \{ */
  bool i_missed_data, all_ok;  /// \}

  /** Flags determined from the data. \{ */
  bool data_says_data_sending_phase, data_says_i_missed_data;  /// \}

  /** Send number as coded in the data. */
  uint16_t data_says_send_number;

  /** Packet number as coded in the data. */
  uint32_t data_says_current_packet_no;
#ifdef LOG_COMMUNICATIONS
  using Accessor::log_communications;
  using Accessor::commlog;
#endif
#ifdef LOG_PACKING
  using Accessor::log_packing;
#endif


public:
  SCM_FEATURES_DEF;

private:
  /** Boolean, to indicate functioning or not of the system. Set to
      false when DUECA signals system is going to be stopped. */
  bool running;

  /** Enumeration for the communication logic */
  enum CommState {
    SyncOnZero,    /**< Waiting for master node to send. */
    DataReceive,   /**< Receiving data from other than master node. */
    DataSend,      /**< Transmit data onto the net. */
    WaitConfirm,   /**< Wait until it is my turn to confirm. */
    SendConfirm,   /**< Send my confirmation. */
  };

  /** Variable controlling communication mode switches for non-master
      nodes. */
  CommState comm_state;

  /** Flag that urges nodes to clean up extra messages from no 0
      sending. */
  bool purge_extra_messages;

  /** Sends the data. */
  void sendMyData(const TimeSpec& t);

  /** Sends a confirmation message. */
  void sendMyConfirm(const TimeSpec& t);

  /** Find out who sent a message in the set, the expected sender or
      no 0, who re-initiated on a timeout. */
  int findSender(const fd_set& set);

  /** Read the data from sender no sender. */
  void readData(int sender, char* store);

  /** Read any messages that may have been stuck in the IP stack. */
  bool spinToLast(int sender, char* store);

  /** Read the control data from a message, modify the message length
      ("bytes") to indicate how much data remains.
      \param sender      Send order of the sending node.
      \param store       Buffer with the data. */
  bool getControlData(int sender, char* store, const TimeSpec& ts);

  /** Extract the payload data from a message. Note that
      getControlData has to be called beforehand, to get a correct
      data length indication in the variable bytes.
      \param ts    Time of tick.
      \param store Pointer to buffer to unpack.
      \returns     true if indeed new data unpacked, and this send
                   cycle handled. */
  bool getDuecaData(const TimeSpec& ts, MessageBuffer::ptr_type store);

  /** This method uses select to wait for a packet on a socket.
      \param fildes      File descriptor for the socket
      \returns           0 if there is no data on the socket, nonzero
                         if there is data. */
  int waitPacket(int fildes);

  /** Activity method, reads data from the net for all nodes, except
      the node with send order 0. */
  void despatch1toN(const TimeSpec& ts);

  /** Activity method, reads data from the net for the node with send
      order 0. */
  void despatch0(const TimeSpec& ts);

  /** Generic activity method. */
  void runIO(const TimeSpec& ts);

  /** Helper, prepares everything on master node for round with error
      correction. */
  void setForCleaningRound();


  /** Small packers for adding timing data to #0's packet. */
  AmorphStore time_info_store;

  /** A packer to add control data to a packet. */
  AmorphStore control_info_store;

  /** Unpackers for reading control and timing data. \{  */
  AmorphReStore time_info_restore, control_info_restore; /// \}

  /** Timing data. \{ */
  int64_t tsend, treceive; /// \}

  /** Timeout value used on initial contact. */
  timeval initial_tv_wait;

  /** Timeout value used in real-time running. */
  timeval realtime_tv_wait;

  /** A helper that can estimate the transport delay from the send and
      return times in the communication between no 0 and no 1. This is
      only used if this node is the first sender. */
  TransportDelayEstimator* tdelay_estimator;

  /** Callback function. */
  Callback<IPAccessor> cb;

  /** Activity of reading and sending. */
  ActivityCallback   net_io;

protected:
  /** Constructor. Note that this constructor is always called from
      derived classes, the IPAccessor by itself cannot exist. */
  IPAccessor();

  /* GenericPacker* transporter,
             FillPacker *fill_packer,
             Unpacker* unpacker,
             FillUnpacker *fill_unpacker,
             int store_size, int n_stores,
             int input_size, int n_inputs,
             int timeout,
             int no_senders, int send_order,
             const PeriodicTimeSpec& time_spec,
             const PrioritySpec& priority_spec,
             TransportDelayEstimator* tdelay_estimator = NULL); */

  /** Function called after creation. */
  virtual bool complete();

  /** Destructor. */
  virtual ~IPAccessor();


  /** Specify the delay estimater. */
  bool setDelayEstimator(ScriptCreatable &p, bool in);

  /** Specify the priority. */
  bool adjustPriority(const PrioritySpec &p);

private:
  /** The copy constructor is not implemented, since this is an
      accessor that accesses communication protocols that have no copy. */
  IPAccessor(const IPAccessor&);

public:
  /** For the information of the system, this is a dueca object. */
  ObjectType getObjectType() const {return O_CommAccessor;};

  /** Stop the activity, at end of program. */
  void prepareToStop();
};

DUECA_NS_END
#endif








