/* ------------------------------------------------------------------   */
/*      item            : DuecaNetMaster.hxx
        made by         : Rene van Paassen
        date            : 171225
        category        : header file
        description     :
        changes         : 171225 first version
        language        : C++
*/

#ifndef DuecaNetMaster_hxx
#define DuecaNetMaster_hxx

#include "NetCommunicatorMaster.hxx"
#include <boost/scoped_ptr.hpp>
#include <dueca/ScriptCreatable.hxx>
#include <dueca/NamedObject.hxx>
#include <dueca/Accessor.hxx>

DUECA_NS_START;

struct NetTimingLog;
struct NetCapacityLog;

/** Communication master

    Uses netcommunicator to implement standard node-to-node
    communication for a DUECA process. The master node implements a
    TCP/IP server for handing out configuration data. Peer nodes
    connect to the master, then receive instructions on data
    connection, with either UDP or WebSockets.

    @verbinclude dueca-net-master.scm
 */
class DuecaNetMaster:
  public Accessor,
  public NetCommunicatorMaster
{
  /** Define shorthand for class */
  typedef DuecaNetMaster _ThisClass_;

  /** A sequence number, to get unique names. */
  static int sequence;

  /** Priority of activity */
  PrioritySpec priority;

  /** Timing of communication */
  TimeSpec time_spec;

  /** Minimum size of fill section */
  size_t fill_minimum;

  /** List of nodes to connect to. This node is master, and therefore
      peer 0; the order of node_id's in this list determines the
      sequence of the other peers. */
  std::vector<int> peer_nodeids;

  struct PeerMeta {
    uint32_t nodeid;
    uint32_t send_order;
    std::string name;
    PeerMeta(uint32_t nodeid = 0, const std::string& name = "",
             uint32_t sendorder = 0);
  };

  /** Map with peer information */
  std::map<uint32_t,PeerMeta> metainfo;

  /** send order counter, for handing out next send order approve */
  unsigned send_order_counter;

  /** To remember current tick value */
  TimeTickType keep_tick;

  /** number of log points */
  uint32_t n_logpoints;

  /** time span value */
  int64_t cycle_span;

  /** Logging object timing */
  std::vector<NetCapacityLog*> log_capacity;

  /** Logging object capacity */
  NetTimingLog *log_timing;

  /** Logging channel */
  boost::scoped_ptr<ChannelWriteToken> w_logtiming;

  /** Logging channels capacity */
  std::vector<ChannelWriteToken*> w_logcapacity;

  /** Clock for starting timing */
  PeriodicAlarm clock;

  /** Callback */
  Callback<_ThisClass_> cb, cbup;

  /** Activity */
  ActivityCallback      net_io;

public:
  SCM_FEATURES_DEF;

  /** Constructor */
  DuecaNetMaster();

  /** Destructor */
  ~DuecaNetMaster();

  /** Complete creation */
  bool complete();

  /** Return table with configuration parameters */
  static const ParameterTable* getParameterTable();

  /** Object classification */
  ObjectType getObjectType() const {return O_CommAccessor;};

  /** Return one of the message buffers, needed for compatibility Unpackers */
  void returnBuffer(MessageBuffer::ptr_type buffer) final;

private:

  /** Main activity */
  void runIO(const TimeSpec& ts);

  /** send and reinstall logs */
  void swapLogs(TimeTickType tick);

  /** initialise when DUECA is up */
  void whenUp(const TimeSpec& ts);

  /** valid token */
  void cbValid(const TimeSpec& ts, const std::string& name);
  
private:

  /** Additional peer data to add to welcome message, none in this case */
  void clientWelcomeConfig(AmorphStore& s, unsigned id) final;

  /** decode configuration payload.

      Client payload consists of node id and hostname, using these to initiate meta info

      @param s          Buffer with config data
      @param is         Peer id
  */
  void clientDecodeConfig(AmorphReStore& s, unsigned id) final;

  /** encode configuration payload.

      Configuration reply is send order in message (info only).

      @param s      Buffer with config data
      @param id     ID for the target, if 0, send to all connected
 */
  void clientSendConfig(const TimeSpec& ts, unsigned id) final;

  /** Approve or decline peer joining.

      Peers are delayed until all have called in, then accepted in send order

      @param id  Peer ID */
  VettingResult clientAuthorizePeer(CommPeer& peer, const TimeSpec& ts) final;

  /** Pack payload into send buffer

      Uses the packers to fill the buffer. Packing only starts after all
      clients complete.

      @param buffer Payload buffer object
  */
  void clientPackPayload(MessageBuffer::ptr_type buffer) final;

  /** Accept a loaded buffer for unpacking

      This accepts a buffer and passes it on to the unpackers.

      @param buffer Payload buffer object
   */
  void clientUnpackPayload(MessageBuffer::ptr_type buffer, unsigned id,
                           TimeTickType current_tick,
                           TimeTickType peertick, int usecoffset) final;

  /** Stop the activity, at end of program.

      Implements a function of the Accessor.
  */
  void prepareToStop() final;
};

DUECA_NS_END;

#endif
