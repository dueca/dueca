/* ------------------------------------------------------------------   */
/*      item            : ChannelReplicatorMaster.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Tue Feb 14 11:13:12 2017
        category        : header file
        description     :
        changes         : Tue Feb 14 11:13:12 2017 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelReplicatorMaster_hxx
#define ChannelReplicatorMaster_hxx

// include the dusime header
#include <dueca.h>
USING_DUECA_NS;


// include headers for functions/classes you need in the module
#include "ChannelReplicator.hxx"
#include <udpcom/NetCommunicatorMaster.hxx>

STARTNSREPLICATOR;

/** A simple module for watching and replicating channel data.

    You can use this module to "loosely" couple several DUECA
    processes together.  Note that this is a typical hack; nodes are
    not synchronized, data is sent more or less "best effort", and
    only a selected set of channels is replicated. No efficient coding
    of channel data is done. This is a "master" module, it will connect
    with one or more peer modules.

    You need to indicate which channels are to be replicated (by
    channel name) to this module. Once a peer joins, these channels
    are also opened on the peer DUECA, and entries are replicated.

    It is possible to implement a vetting/information process. The
    module can open a channel with ReplicatorPeerJoined events. When a
    peer joins, its IP address is communicated over this channel. The
    peer is accepted when a response on another channel is returned.
    Names for both channels can be specified, and the dataclass for the
    response must be given. The packed size of the response must be under
    1KByte.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude channel-replicator-master.scm
*/
class ChannelReplicatorMaster:
  public ChannelReplicator,
  public NetCommunicatorMaster
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ChannelReplicatorMaster _ThisClass_;

private: // simulation data

  /** Remember peer acknowledgements */
  typedef std::map<uint16_t,ReplicatorPeerAcknowledge> peer_ack_type;

  /** Peer acknowledgements that came in */
  peer_ack_type                       peer_acknowledgements;

  /** Type definition, for queue of channel readers */
  typedef std::list<std::pair<uint16_t,boost::shared_ptr<EntryReader> > >
  readerlist_type;

  /** Channel readers to clean */
  readerlist_type                    obsolete_readers;

  /** Type definition, for queue of channel writers */
  typedef std::list<std::pair<uint16_t,boost::shared_ptr<EntryWriter> > >
  writerlist_type;

  /** Candidate writers to configure and insert */
  writerlist_type                    candidate_writers;

  /** Obsolete writers to clean */
  writerlist_type                    obsolete_writers;

private: // channel access
  /** If not NULL, information on the joining peer is sent over this
      channel */
  ChannelWriteToken                 *w_peernotice;

  /** If not NULL, peers are approved after info came in */
  ChannelReadToken                  *r_peerinfo;

  /** Watched channel information, including origin of entries */
  ChannelWriteToken                 *w_replicatorinfo;

private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm                      masterclock;

  /** Callback object for simulation calculation. */
  Callback<_ThisClass_>              cb1;

  /** Activity for simulation calculation. */
  ActivityCallback                   do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  ChannelReplicatorMaster(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~ChannelReplicatorMaster();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int>& i);

  /** Send notices when peer joins */
  bool setJoinNoticeChannel(const std::string& channelname);

  /** Wait for information for peer */
  bool setPeerInformationChannel(const std::string& peerinfo);

  /** Additional information on the replicator actions */
  bool setReplicatorInformationChannel(const std::string& channelname);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec& ts);

private: // helper functions

  /** For the master node, decode and process connect requests */
  void checkNewConnections(const TimeSpec&);

  /** Send the configuration as is now */
  void sendCurrentConfigToPeer(const CommPeer& peer,
                               TimeTickType join_cycle,
                               const std::string& extradata = std::string(""));

  /** modify which sender to follow */
  void changeFollowId(const CommPeer& peer);

  /** send config changes from channels; new and deleted reading entries */
  void sendChannelConfigChanges(const TimeSpec& ts);

  /** check configuration requests from peers */
  void checkAndUpdatePeerStates(const TimeSpec& ts);

  /** clear all entries that correspond to a specific peer ID */
  void clearPeerMatchingEntries(unsigned peerno);

  /** Add channels to watched list */
  bool watchChannels(const std::vector<std::string> &ch);

  /** Extract data from vetted message */
  void clientUnpackPayload(MessageBuffer::ptr_type buffer,
                           unsigned peer_id, TimeTickType current_tick,
                           TimeTickType i_peertick, int usecoffset) final;

  /** Pack payload data */
  void clientPackPayload(MessageBuffer::ptr_type buffer) final
  { ChannelReplicator::_clientPackPayload(buffer); }

  /** Access this function here, payload packing implementation is
      common to master and peer */
  using ChannelReplicator::clientUnpackPayload;


  /** Receive info on new peer */
  void clientInfoPeerJoined(const std::string& address, unsigned id,
                            const TimeSpec& ts) final;

  /** Information on events for descendants */
  void clientInfoPeerLeft(unsigned id, const TimeSpec& ts) final;

  /** Additional peer data to add to welcome message

      Pack data in the store, when full, flush the store to the given tcp socket.

      @param s      Store
      @param id     Numeric send id for the peer
      @param tcp_socket  Socket for flushing full stores
 */
  void clientWelcomeConfig(AmorphStore& s, unsigned id) final;

   /** decode configuration payload.

      Decode one object or message; when decode fails, restore the config buffer.

      @param s      Buffer with config data
      @param id     Id for the sending client
  */
  void clientDecodeConfig(AmorphReStore& s, unsigned id) final;

  /** encode configuration payload.

      Encode multiple objects or messages. Encode each object preceded
      by a UDPPeerConfig::ClientPayload message. When the buffer
      overflows, reset to previous state, and send with the sendConfig
      call. Then continue with cleared buffer.

      @param s      Buffer with config data
      @param id     ID for the target, if 0, send to all connected
 */
  void clientSendConfig(const TimeSpec& ts, unsigned id) final;

  /** Approve or decline peer joining.

      @param peer   Peer data
      @param ts     Time spec for the action
  */
  NetCommunicatorMaster::VettingResult
  clientAuthorizePeer(CommPeer& peer, const TimeSpec& ts) final;

private:
  /** Return buffer, implemented/accessed by Master/Peer */
  void returnBuffer(MessageBuffer::ptr_type buffer) final
  { data_comm->returnBuffer(buffer); }
};

ENDNSREPLICATOR;

#endif
