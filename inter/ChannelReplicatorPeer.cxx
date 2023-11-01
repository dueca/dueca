/* ------------------------------------------------------------------   */
/*      item            : ChannelReplicatorPeer.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Tue Feb 14 11:13:26 2017
        category        : body file
        description     :
        changes         : Tue Feb 14 11:13:26 2017 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include <tuple>
#include <utility>
#define ChannelReplicatorPeer_cxx

// include the definition of the module class
#include "ChannelReplicatorPeer.hxx"
#include "ReplicatorExceptions.hxx"

// include the debug writing header, by default, write warning and
// error messages
//#define D_INT
#define I_INT
#include <debug.h>

// include additional files needed for your calculation here
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "EntryWriter.hxx"
#include "EntryReader.hxx"
#include <errno.h>
#include <fcntl.h>
#include <boost/lexical_cast.hpp>
#include <boost/swap.hpp>
#include <ifaddrs.h>
#include <net/if.h>
#include <exception>
#include <dassert.h>

#include <dueca/Environment.hxx>
#include <udpcom/UDPPeerConfig.hxx>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>

#define DEBPRINTLEVEL 1
#include <debprint.h>

STARTNSREPLICATOR;

// class/module name
const char* const ChannelReplicatorPeer::classname = "channel-replicator-peer";

// Parameters to be inserted
const ParameterTable* ChannelReplicatorPeer::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "if-address",
      new VarProbe<_ThisClass_,std::string>(&_ThisClass_::interface_address),
      "Address of the interface over which communication takes place. This\n"
      "is usually determined automatically."},

    // specific for UDP connections
    { "port-re-use",
      new VarProbe<_ThisClass_,bool>(&_ThisClass_::port_re_use),
      "Enable port re-use, typically for testing." },
    { "lowdelay", new VarProbe<_ThisClass_,bool>
      (&_ThisClass_::lowdelay),
      "Set lowdelay TOS on the sent packets. Default true."},
    { "socket-priority", new VarProbe<_ThisClass_,int>
      (&_ThisClass_::socket_priority),
      "Set socket priority on send socket. Default 6. Suggestion\n"
      "6, or 7 with root access / CAP_NET_ADMIN capability, -1 to disable." },

    { "timeout", new VarProbe<_ThisClass_,double>
      (&_ThisClass_::timeout),
      "timeout value [s]" },

    // configuration URL
    { "config-url", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::master_url),
      "URL of the configuration connection. Must be Websocket (start with ws\n"
      "includes port, and path), e.g., \"ws://myhost:8888/config\"" },

    { "override-data-url", new VarProbe<_ThisClass_,vstring>
      (&_ThisClass_::override_data_url),
      "Option to override the data url sent by the master, in case network\n"
      "port translation is applied." },

    { "master-information-channel",
      new MemberCall<_ThisClass_,std::string>
      (&_ThisClass_::setMasterInformationChannel),
      "Create a write token on channel with supplemental start information\n"
      "for this peer. Supply the channel name. The channel will receive\n"
      "a ReplicatorPeerAcknowledge object when the connection is established."},

    { "sync-to-master-timing",
      new VarProbe<_ThisClass_,bool>
      (&_ThisClass_::sync_to_master_timing),
      "Synchronize to the master's timing, creeps up to the master within the\n"
      "communication data rate" },

    { "timing-gain",
      new VarProbe<_ThisClass_,double>
      (&_ThisClass_::timing_gain),
      "Gain factor for determining timing differences (default 0.002)" },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "This is the peer side of the dueca Interconnect facility.\n"
      "Simply specify how to connect to the master, additional configuration\n"
      "will be received from the master. Note that this module will occupy\n"
      "a thread; specify an exclusive priority." }
  };

  return parameter_table;
}

// constructor
ChannelReplicatorPeer::ChannelReplicatorPeer(Entity* e, const char* part, const
                                             PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  ChannelReplicator(e, classname, part, ps),

  // initialize the data you need in your simulation or process
  commanded_stop(false),
  candidate_readers(),
  sync_to_master_timing(false),
  w_masterinfo(),
  time_spec(),
  slaveclock(),
  cb1(this, &ChannelReplicatorPeer::doCalculation),
  // the module's main activity
  do_calc(getId(), "replicate channel - peer", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(slaveclock);
}

bool ChannelReplicatorPeer::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  do_calc.setTrigger(slaveclock);

  // tell the clock to expect sync messages
  if (sync_to_master_timing) {
    Ticker::single()->noImplicitSync();
  }

  return true;
}

// destructor
ChannelReplicatorPeer::~ChannelReplicatorPeer()
{
  //
}

bool ChannelReplicatorPeer::
setMasterInformationChannel(const std::string& channelname)
{
  w_masterinfo.reset
    (new ChannelWriteToken
     (getId(), NameSet(channelname), ReplicatorPeerAcknowledge::classname,
      getNameSet().name, Channel::Events));
  return true;
}

// tell DUECA you are prepared
bool ChannelReplicatorPeer::isPrepared()
{
  bool res = true;

  if (w_masterinfo) {
    CHECK_TOKEN(*w_masterinfo);
  }

  // return result of checks
  return res;
}

// start the module
void ChannelReplicatorPeer::startModule(const TimeSpec &time)
{
  setStopTime(MAX_TIMETICK);
  do_calc.switchOn(time);
  time_spec.forceAdvance(time);
  slaveclock.requestAlarm(time_spec.getValidityStart());
}

// stop the module
void ChannelReplicatorPeer::stopModule(const TimeSpec &time)
{
  setStopTime(time.getValidityStart());
  do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void ChannelReplicatorPeer::doCalculation(const TimeSpec& ts)
{
  if (Environment::getInstance()->runningMultiThread()) {
    /* DUECA interconnect.
       
       Starting cyclic processing of messages */
    I_INT("cyclic start " << ts);
    setStopTime(MAX_TIMETICK);

    try {
      startCyclic(do_calc);
    }
    catch(const connectionfails& e) {
      /* DUECA interconnect.
	 
	 Tried to run a communication cycle, but could not make the
	 websockets connection for start up. Will attempt to connect
	 later. */
      W_NET("Connection failure, will attempt new connection later");
      slaveclock.requestAlarm(ts.getValidityStart() +
			      int(round(1.0/Ticker::single()->getTimeGranule())));
    }
  }
  
  else {
    DEB1("one cycle " << ts);
    try {
      oneCycle(do_calc);
      
      // when not over to cyclic, but still stopping
      if (commanded_stop) {
	clearConnections();
	return;
      }
      time_spec.advance();
      slaveclock.requestAlarm(time_spec.getValidityStart());
    }
    catch(const connectionfails& e) {
      /* DUECA interconnect.
	 
	 Tried to run a communication cycle, but could not make the
	 websockets connection for start up. Will attempt to connect
	 later. */
      W_NET("Connection failure, will attempt new connection later");
      slaveclock.requestAlarm(ts.getValidityStart() + 
			      int(round(1.0/Ticker::single()->getTimeGranule())));
    }
  }
}

void ChannelReplicatorPeer::resetClientConfiguration()
{
  DEB("Resetting peer reader/watchers configuration");
  candidate_readers.clear();
  watched.clear();
  while (detected_entries.notEmpty()) {
    delete detected_entries.front();
    detected_entries.pop();
  }
  while (deleted_entries.notEmpty()) {
    delete deleted_entries.front();
    deleted_entries.pop();
  }
}

void ChannelReplicatorPeer::clientIsConnected()
{
  // ??
}

// receive from the conf_socket, new data from the master about
// channel entry addition or removal
void ChannelReplicatorPeer::
clientDecodeConfig(AmorphReStore& s)
{
  try {
    ReplicatorConfig cmd(s);   // decode the config command
    DEB("decode config " << cmd);
    switch(cmd.mtype) {

    case ReplicatorConfig::AddChannel: {

      // this should be absolutely true
#ifdef assert
      channelmap_type::const_iterator ii = watched.begin();
      while (ii != watched.end() && ii->second->channelname != cmd.name &&
             ii->first != cmd.channel_id) ii++;
      assert(ii == watched.end());
#endif

      // add the channel watcher
      watched[cmd.channel_id] = std::shared_ptr<WatchedChannel>
        (new WatchedChannel(cmd.name, cmd.channel_id, this));
    }
      break;

    case ReplicatorConfig::AddEntry: {

      if (cmd.slave_id == peer_id) {

        DEB("Entry confirmation chn=" << cmd.channel_id <<
            " tmpid=" << cmd.tmp_entry_id <<
            " front chn=" << candidate_readers.front().first <<
            " front tmpid=" << candidate_readers.front().second->
            getReplicatorEntryId());

        // if the entry is from here, it is validated now. Considering that
        // order in the tcp link is preserved
        assert (candidate_readers.front().second->
                getReplicatorEntryId() == cmd.tmp_entry_id &&
                candidate_readers.front().first == cmd.channel_id);

        /* DUECA interconnect.

           Information on read accessing an entry in a channel for
           replication. */
        I_INT("making in channel " << cmd.channel_id <<
              " reading entry " << cmd.entry_id <<
              " permanent, temp id " << cmd.tmp_entry_id);
        candidate_readers.front().second->
          setReplicatorEntryId(cmd.entry_id);
        watched[cmd.channel_id]->readers.push_back
          (candidate_readers.front().second);
        candidate_readers.pop_front();
      }
      else {

        // entry creating somewhere else, start replicating it here
        assert(watched[cmd.channel_id]->writers.find(cmd.entry_id) ==
               watched[cmd.channel_id]->writers.end());

        // check all data class stuff matches
        verifyDataClass(cmd, cmd.slave_id);

        /* DUECA interconnect.

           Information on creating a new replicating writer in a
           channel. */
        I_INT("new writer in channel " <<
              watched[cmd.channel_id]->channelname <<
              " rid " << cmd.entry_id << " origin " << cmd.slave_id);
        watched[cmd.channel_id]->writers[cmd.entry_id] =
          std::shared_ptr<EntryWriter>
          (new EntryWriter
           (getId(), cmd.slave_id, cmd.entry_id,
            watched[cmd.channel_id]->channelname,
            cmd.dataclass.front(), cmd.data_magic.front(), cmd.name,
            cmd.time_aspect, cmd.arity, cmd.packmode, cmd.tclass, getId()));
        watched[cmd.channel_id]->writers[cmd.entry_id]->
          setReplicatorEntryId(cmd.entry_id);
      }
    }
      break;

    case ReplicatorConfig::RemoveEntry:

      if (cmd.slave_id == peer_id) {

        // from here. Entry has already been removed from readers
      }
      else {

        /* DUECA interconnect.

           Information on deleting new replicating writer in a
           channel. */
        I_INT("deleting writer in channel " << cmd.channel_id <<
              " entry id " << cmd.entry_id << " origin " << cmd.slave_id);
        watched[cmd.channel_id]->writers.erase(cmd.entry_id);

      }

      break;

    default:
      /* DUECA interconnect.

         Received a request that cannot be processed. Indicates a
         DUECA programming error. */
      E_INT("Slave received impossible config command " << cmd.mtype);
    }
  }
  catch (const dueca::AmorphReStoreEmpty &e) {

    /* DUECA interconnect.

       A message with "client data" for the ChannelReplicator was
       received, but did not contain a complete set of data. The read
       buffer is reset, and decoding will be attempted again when more
       data becomes available.  */
    I_INT("failed to get complete replicator client config");

    // re-throw the exception, so the store can be properly reset.
    throw(e);
  }
}



void ChannelReplicatorPeer::clientSendConfig()
{
  DEB2("Sending config");
  // send the channel changes to master
  char cbuf[config_buffer_size];
  AmorphStore s(cbuf, config_buffer_size);
  unsigned filllevel = s.getSize();
  static const UDPPeerConfig clientmark(UDPPeerConfig::ClientPayload);

  while (detected_entries.notEmpty()) {

    try {
      uint16_t cid = detected_entries.front()->first;
      uint16_t tmpid = watched[cid]->next_id;
      const ChannelEntryInfo *ei = &(detected_entries.front()->second);

      // check that this does not correspond to a replicator entry
      WatchedChannel::writerlist_type::iterator ww =
        watched[cid]->writers.begin();
      while (ww != watched[cid]->writers.end() &&
             ww->second->getEntryId() != ei->entry_id) ww++;

      if (ww != watched[cid]->writers.end()) {
        DEB("channel " << cid <<
            " ignoring callback on replicator-written entry# " <<
            ei->entry_id << " rid " << ww->second->getReplicatorEntryId());
      }
      else {
        ReplicatorConfig cf
          (ReplicatorConfig::AddEntry, peer_id,
           cid, 0U, tmpid,
           ei->entry_label,
           ei->time_aspect, ei->arity, ei->packingmode,
           ei->transportclass);
        addDataClass(cf, ei->data_class);
        ::packData(s, clientmark);
        ::packData(s, cf);
        DEB("Sending payld  " << clientmark);
        DEB("Sending config " << cf);
        filllevel = s.getSize();

        /* DUECA interconnect.

           Information on a local detected entry */
        I_INT("reporting new entry channel " << cid <<
              " temp entry id " << tmpid);

        // packing worked, proceed with completing the configuration
        // entryAdded places the entry in a candidate_readers list, where it
        // awaits confirmation by the master.
        candidate_readers.push_back
          (make_pair
           (cid, std::shared_ptr<EntryReader>
            (new EntryReader(getId(), *ei, watched[cid]->channelname))));
        candidate_readers.back().second->setReplicatorEntryId(tmpid);
        watched[cid]->next_id++;
      }

      // clean up the FIFO data
      delete detected_entries.front();
      detected_entries.pop();
    }
    catch (const AmorphStoreBoundary& e) {
      s.setSize(filllevel);
      sendConfig(s);
      filllevel = 0;
    }
  }

  while (deleted_entries.notEmpty()) {
    try {
      uint16_t cid = deleted_entries.front()->first;
      uint16_t eid = deleted_entries.front()->second;
      WatchedChannel::readerlist_type::iterator rr =
        watched[cid]->readers.begin();
      while (rr != watched[cid]->readers.end() &&
             (*rr)->getEntryId() != eid) rr++;
      if (rr != watched[cid]->readers.end()) {
        ReplicatorConfig cf
          (ReplicatorConfig::RemoveEntry, peer_id, cid,
           (*rr)->getReplicatorEntryId());
        DEB("Sending payld  " << clientmark);
        DEB("Sending config " << cf);
        ::packData(s, clientmark);
        ::packData(s, cf);

        /* DUECA interconnect.

           Information on a locally disappeared entry. */
        I_INT("reporting deletion channel " << cid << " entry# " << eid <<
              " rid " << (*rr)->getReplicatorEntryId());

        watched[cid]->readers.erase(rr);
        filllevel = s.getSize();
      }
      else {
        readerlist_type::iterator rc = candidate_readers.begin();
        while (rc != candidate_readers.end() &&
               rc->second->getEntryId() != eid &&
               rc->first != cid) rc++;
        if (rc != candidate_readers.end()) {

          /* DUECA interconnect.

             A locally disappeared entry was scheduled to be read and
             replicated, but not yet configured. The reporting on its
             removal will be postponed. */
          W_INT("reporting deletion channel " << cid <<
                " tmp entry " << rc->second->getReplicatorEntryId() <<
                " postponing");
          break;
        }
        else {
          /* DUECA interconnect.

             A locally disappeared entry was not watched. */
          I_INT("channel " << cid << " deletion non-watched entry# " << eid);
        }
      }
      delete deleted_entries.front();
      deleted_entries.pop();
    }
    catch (const AmorphStoreBoundary& e) {
      s.setSize(filllevel);
      sendConfig(s);
      filllevel = 0;
    }
  }
  if (filllevel) {
    sendConfig(s);
  }
}


void ChannelReplicatorPeer::clientSendWelcome()
{
  // nothing?
}

void ChannelReplicatorPeer::clientUnpackPayload
(MessageBuffer::ptr_type buffer, unsigned id,
 TimeTickType current_tick, TimeTickType peertick, int usecoffset)
{
  auto timing = peer_timing.find(id);
  if (timing == peer_timing.end()) {
    peer_timing.emplace(std::piecewise_construct,
                        std::forward_as_tuple(id), 
                        std::forward_as_tuple(ts_interval, timing_gain));
    timing = peer_timing.find(id);
  }
  timing->second.adjustDelta
    (current_tick, peertick,
     sync_to_master_timing && (id == 0), usecoffset);
  ChannelReplicator::_clientUnpackPayload(buffer, id, timing->second);
}


ENDNSREPLICATOR;


// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
// static TypeCreator<replicator::ChannelReplicatorPeer>
// a(replicator::ChannelReplicatorPeer::getMyParameterTable());
