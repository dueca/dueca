/* ------------------------------------------------------------------   */
/*      item            : ChannelReplicatorPeer.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Tue Feb 14 11:13:26 2017
        category        : header file
        description     :
        changes         : Tue Feb 14 11:13:26 2017 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelReplicatorPeer_hxx
#define ChannelReplicatorPeer_hxx

// include the dusime header
#include <dueca.h>
USING_DUECA_NS;

// include headers for functions/classes you need in the module
#include "ChannelReplicator.hxx"
#include <udpcom/NetCommunicatorPeer.hxx>
#include <boost/scoped_ptr.hpp>

STARTNSREPLICATOR;

/** A simulation module.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude channel-replicator-peer.scm
 */
class ChannelReplicatorPeer:
  public ChannelReplicator,
  public NetCommunicatorPeer
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ChannelReplicatorPeer _ThisClass_;

private: // simulation data
  // declare the data you need in your simulation

  /** Stop command received */
  bool                                commanded_stop;

  /** Type definition, for queue of channel readers */
  typedef std::list<std::pair<uint16_t,std::shared_ptr<EntryReader> > >
  readerlist_type;

  /** Candidate readers to configure and insert */
  readerlist_type                     candidate_readers;

  /** Adjust timing to master pace */
  bool                                sync_to_master_timing;

private: // channel access
  /** channel for writing master information */
  boost::scoped_ptr<ChannelWriteToken> w_masterinfo;

private: // activity allocation
  /** Timing object */
  PeriodicTimeSpec                    time_spec;

  /** Clock for triggering initial activation on startModule */
  AperiodicAlarm                      slaveclock;

  /** Callback object for simulation calculation. */
  Callback<_ThisClass_>              cb1;

  /** Activity for simulation calculation. */
  ActivityCallback                    do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const            classname;

  /** Return the parameter table. */
  static const ParameterTable*        getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  ChannelReplicatorPeer(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~ChannelReplicatorPeer();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Channel with master's information */
  bool setMasterInformationChannel(const std::string& channelname);

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

private:

  /** Reset all links to channel system & config. */
  void resetClientConfiguration();

  /** decode configuration messages.

      Implements function for NetCommunicatorPeer. Passes application-
      dependent configuration data.

      @param s       Buffer with config data

      @throws AmorphReStoreEmpty Throwing of this exception flags
                     incomplete client data in the buffer (i.e., the
                     socket needs to be read further before the
                     information is complete).
  */
  void clientDecodeConfig(AmorphReStore& s) final;

  /** Encode and send configuration data.

      uses the sendConfig function to pass data in filled AmorphStore
      objects to the master.
  */
  void clientSendConfig() final;

  /** send initial welcome message */
  void clientSendWelcome() final;

  /** Connection is established */
  void clientIsConnected() final;

  /** Pack payload into send buffer

      Uses the packers to fill the buffer. Packing only starts after all
      clients complete.

      @param buffer Payload buffer object
  */
  void clientPackPayload(MessageBuffer::ptr_type buffer) final
  { ChannelReplicator::_clientPackPayload(buffer); }

  /** Accept a loaded buffer for unpacking

      This accepts a buffer and passes it on to the unpackers.

      @param buffer Payload buffer object
   */
  void clientUnpackPayload(MessageBuffer::ptr_type buffer, unsigned id,
                           TimeTickType current_tick,
                           TimeTickType peertick,
                           int usecoffset) final;

  /** Return buffer, implemented/accessed by Master/Peer */
  void returnBuffer(MessageBuffer::ptr_type buffer) final
  { data_comm->returnBuffer(buffer); }

};

ENDNSREPLICATOR;

#endif
