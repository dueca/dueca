/* ------------------------------------------------------------------   */
/*      item            : ChannelReplicatorMaster.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Tue Feb 14 11:13:12 2017
        category        : body file
        description     :
        changes         : Tue Feb 14 11:13:12 2017 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ChannelReplicatorMaster_cxx

// include the definition of the module class
#include "ChannelReplicatorMaster.hxx"
#include "ReplicatorExceptions.hxx"
#include "ReplicatorInfo.hxx"
#include <udpcom/UDPPeerConfig.hxx>

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
#include <math.h>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>

#include <debprint.h>

STARTNSREPLICATOR;

// class/module name
const char* const ChannelReplicatorMaster::classname = "channel-replicator-master";

// Parameters to be inserted
const ParameterTable* ChannelReplicatorMaster::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "set-timing",
      new MemberCall<_ThisClass_,TimeSpec>
        (&_ThisClass_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisClass_,vector<int> >
      (&_ThisClass_::checkTiming), check_timing_description },

    { "watch-channels",
      new MemberCall<_ThisClass_,std::vector<std::string> >
      (&_ThisClass_::watchChannels),
      "Provide a list of the watched channels for this replicator" },

    // specific for UDP connections
    { "port-re-use",
      new VarProbe<_ThisClass_,bool>(&_ThisClass_::port_re_use),
      "Specify port re-use, typically for testing." },
    { "lowdelay", new VarProbe<_ThisClass_,bool>
      (&_ThisClass_::lowdelay),
      "Set lowdelay TOS on the sent packets. Default true."},
    { "socket-priority", new VarProbe<_ThisClass_,int>
      (&_ThisClass_::socket_priority),
      "Set socket priority on send socket. Default 6. Suggestion\n"
      "6, or 7 with root access / CAP_NET_ADMIN capability, -1 to disable." },

    { "message-size",
      new VarProbe<_ThisClass_,unsigned>(&_ThisClass_::buffer_size),
      "Size of UDP messages." },

    { "join-notice-channel",
      new MemberCall<_ThisClass_,std::string>
      (&_ThisClass_::setJoinNoticeChannel),
      "Create a write token to a channel for sending ReplicatorPeerJoined\n"
      "messages. Supply the channel name." },

    { "peer-information-channel",
      new MemberCall<_ThisClass_,std::string >
      (&_ThisClass_::setPeerInformationChannel),
      "Create a read token on channel with supplemental start information\n"
      "for a peer. Supply the channel name." },

    { "replicator-information-channel",
      new MemberCall<_ThisClass_,std::string >
      (&_ThisClass_::setReplicatorInformationChannel),
      "Create a write token on channel with overview information on\n"
      "replication." },

    // for WebSocket connections, can be used for UDP too
    { "data-url", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::url),
      "URL of the data connection, for both UDP and WebSocket connections\n"
      "UDP example: \"udp://hostname-or-ipaddress:data-port\"\n"
      "WS  example: \"ws://hostname-or-ipaddress:data-port/data\". If you are\n"
      "using websockets for data communication, these must be on the same\n"
      "port as the configuration URL, but at a different endpoint."},

    { "public-data-url", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::public_data_url),
      "Override the information on the data connection, in case clients\n"
      "connect through a firewall with port mapping. Provide a different\n"
      "client-side view of the connection." },

    // configuration URL
    { "config-url", new VarProbe<_ThisClass_,std::string>
      (&_ThisClass_::config_url),
      "URL of the configuration connection. Must be Websocket (start with ws\n"
      "includes port, and path, e.g., \"ws://myhost:8888/config\"" },

    { "timeout",
      new VarProbe<_ThisClass_,double>
      (&_ThisClass_::timeout),
      "Timeout, in s, before a message from the peers is considered missing" },

    { "timing-gain",
      new VarProbe<_ThisClass_,double>
      (&_ThisClass_::timing_gain),
      "Gain factor for determining timing differences (default 0.002)" },

    { "timing-interval",
      new VarProbe<_ThisClass_,unsigned>
      (&_ThisClass_::ts_interval),
      "Interval on which data time translation is rounded. Default ticker's\n"
      "time interval." },

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
      "This is the master side of the dueca Interconnect facility.\n"
      "For the master module, specify a URL for the configuration service\n"
      "and a URL for the data service. With the watch-channels argument,\n"
      "you can indicate which dueca channels are to be replicated; note\n"
      "that these will be watched in all connected nodes."}
  };

  return parameter_table;
}

// constructor
ChannelReplicatorMaster::ChannelReplicatorMaster(Entity* e,
                                                 const char* part,
                                                 const PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  ChannelReplicator(e, classname, part, ps),
  w_peernotice(NULL),
  r_peerinfo(NULL),
  w_replicatorinfo(NULL),
  masterclock(),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisClass_::doCalculation),
  // the module's main activity
  do_calc(getId(), "replicate channel - master", &cb1, ps)
{
  //
}

bool ChannelReplicatorMaster::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  // this creates the server socket
  bool res = NetCommunicatorMaster::complete();

  // initiate regular triggering
  do_calc.setTrigger(masterclock);

  return res;
}

// destructor
ChannelReplicatorMaster::~ChannelReplicatorMaster()
{
  delete w_peernotice;
  delete r_peerinfo;
  delete w_replicatorinfo;
}

// as an example, the setTimeSpec function
bool ChannelReplicatorMaster::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // set the master clock
  masterclock.changePeriodAndOffset(ts);

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool ChannelReplicatorMaster::checkTiming(const vector<int>& i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

bool ChannelReplicatorMaster::
setJoinNoticeChannel(const std::string& channelname)
{
  delete w_peernotice;
  try {
    w_peernotice = new ChannelWriteToken
      (getId(), NameSet(channelname), ReplicatorPeerJoined::classname,
       getNameSet().name, Channel::Events);
  }
  catch(const std::exception& e) {
    /* DUECA interconnect.

       Unforeseen failure in creating a write token for the notice
       channel for peer connections. Check how other modules
       interacting with this channel have specified channel
       properties. */
    E_INT("Could not create write token on channel " << channelname <<
          " cause: " << e.what());
    return false;
  }
  return true;
}

bool ChannelReplicatorMaster::
setPeerInformationChannel(const std::string& channelname)
{
  delete r_peerinfo;
  try {
    r_peerinfo = new ChannelReadToken
      (getId(), NameSet(channelname), ReplicatorPeerAcknowledge::classname,
       0, Channel::Events, Channel::OnlyOneEntry, Channel::ReadAllData);
  }
  catch(const std::exception& e) {
    /* DUECA interconnect.

       Unforeseen failure in creating a read token for the peer
       information channel. Check that this channel is being written to. */
    E_INT("Could not create read token on channel " << channelname <<
          " cause: " << e.what());
    return false;
  }
  return true;
}

bool ChannelReplicatorMaster::
setReplicatorInformationChannel(const std::string& channelname)
{
  delete w_replicatorinfo;
  try {
    w_replicatorinfo = new ChannelWriteToken
      (getId(), NameSet(channelname), ReplicatorInfo::classname,
       getNameSet().name, Channel::Events);
  }
  catch(const std::exception& e) {
    /* DUECA interconnect.

       Unforeseen failure in creating a write token for the replicator
       information channel. Check how other modules interacting with
       this channel have specified channel properties. */
    E_INT("Could not create write token on channel " << channelname <<
          " cause: " << e.what());
    return false;
  }
  return true;
}

// tell DUECA you are prepared
bool ChannelReplicatorMaster::isPrepared()
{
  bool res = true;

  if (w_peernotice) CHECK_TOKEN(*w_peernotice);
  if (r_peerinfo) CHECK_TOKEN(*r_peerinfo);
  if (w_replicatorinfo) CHECK_TOKEN(*w_replicatorinfo);

  // return result of checks
  return res;
}

// start the module
void ChannelReplicatorMaster::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void ChannelReplicatorMaster::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void ChannelReplicatorMaster::doCalculation(const TimeSpec& ts)
{
  // early return if delayed by timeout
  if (do_calc.numScheduledBehind()) return;
  
  doCycle(ts, do_calc);

  if (peers.size() == 0) {
    // clear all accumulated entries
    flushReaders();
  }
}

void ChannelReplicatorMaster::
clientInfoPeerJoined(const std::string& address, unsigned id,
                     const TimeSpec& ts)
{
  DEB("Peer joined " << address << " peer id " << id);
  // inform about the presence of a new peer
  if (w_peernotice) {
    DataWriter<ReplicatorPeerJoined> j(*w_peernotice);
    j.data().netaddress = address;
    j.data().peer_id = id;
  }

  // create room for timing information
  peer_timing.emplace(std::piecewise_construct,
                      std::forward_as_tuple(id), 
                      std::forward_as_tuple(ts_interval, timing_gain));
}

/*
   Send the current channel configuration to a newly joining peer

   * send basic information, UDP connection information, who to
     follow, slave id
   * Inform on all channels and current entries
*/
static const UDPPeerConfig clientmark(UDPPeerConfig::ClientPayload);

void ChannelReplicatorMaster::
clientWelcomeConfig(AmorphStore& s, unsigned peer_id)
{
  unsigned filllevel = s.getSize();
  DEB("Welcome data to peer " << peer_id);

  for (channelmap_type::const_iterator cc = watched.begin();
       cc != watched.end(); ) {
    try {
      // add all configured channels and their entries
      ReplicatorConfig cf(ReplicatorConfig::AddChannel, 0, cc->first,
                          0, 0, cc->second->channelname);
      DEB(cf);
      ::packData(s, clientmark);
      ::packData(s, cf);

      // it worked, over to next channel, remember fill level
      filllevel = s.getSize();
    }
    catch(const AmorphStoreBoundary& e) {
      // send over the buffer, and reset it to accept more data
      s.setSize(filllevel);
      flushStore(s, peer_id);
      filllevel = 0;
    }

    // all entries originating here,
    for (WatchedChannel::readerlist_type::const_iterator
           ee = cc->second->readers.begin();
         ee != cc->second->readers.end(); ) {
      try {
        // add all configured channels and their entries
        ReplicatorConfig cf(ReplicatorConfig::AddEntry, 0U,
                            cc->first, (*ee)->getReplicatorEntryId(),
                            dueca::entry_end,
                            (*ee)->getLabel(),
                            (*ee)->getEntryTimeAspect(),
                            (*ee)->getEntryArity(), (*ee)->getPackingMode(),
                            (*ee)->getTransportClass());
        addDataClass(cf, (*ee)->getDataClass());
        DEB(cf);
        ::packData(s, clientmark);
        ::packData(s, cf);

        // it worked, over to next reader, remember fill level
        filllevel = s.getSize(); ee++;
      }
      catch(const AmorphStoreBoundary& e) {

        // send over the buffer, and reset it to accept more data
        s.setSize(filllevel);
        flushStore(s, peer_id);
        filllevel = 0;
      }
    }

    // all entries from elsewhere, and written here
    for (WatchedChannel::writerlist_type::const_iterator
           ee = cc->second->writers.begin();
         ee != cc->second->writers.end(); ) {
      try {
        ReplicatorConfig cf(ReplicatorConfig::AddEntry, ee->second->getOrigin(),
                            cc->first, ee->first, dueca::entry_end,
                            ee->second->getLabel(),
                            ee->second->getEntryTimeAspect(),
                            ee->second->getEntryArity(), ee->second->getPackingMode(),
                            ee->second->getTransportClass());
        addDataClass(cf, ee->second->getDataClass());
        DEB(cf);
        ::packData(s, clientmark);
        ::packData(s, cf);

        // it worked, over to next reader, remember fill level
        filllevel = s.getSize(); ee++;
      }
      catch(const AmorphStoreBoundary& e) {

        // send over the buffer, and reset it to accept more data
        s.setSize(filllevel);
        flushStore(s, peer_id);
        filllevel = 0;
      }
    }

    // when here, over to next channel watcher
    cc++;
  }
  // send remaining data
  flushStore(s, peer_id);
}

#ifdef DEBDEF
struct DEBANNOUNCE
{
  bool p1;
  const std::string msg;
  DEBANNOUNCE(const std::string& msg) : p1(true), msg(msg) {}
  template<typename X>
  void operator () (const X& x) {
    if (p1) { DEB(msg); }
    DEB(x); }
};
#else
#define DEB_A(A)
#endif

/*
   Configuration setup step 2

   * process any new detected channnel entries in the current node,
     ignore the ones that are a writer for a remote node, and
     replicate the truely local ones
   * run through channels to process all candidate writers (from other
     nodes) send configuration information and move them to the writer
     map and all obsolete writers (moved to list in previous step) are
     sent in configuration information and removed.
   * process any deleted channel entries in the current nodes. Again,
     ignore the ones from our own writers, and send deletion
     information on all others
*/
void ChannelReplicatorMaster::clientSendConfig(const TimeSpec& ts, unsigned peer_id)
{
#ifdef DEBDEF
  DEBANNOUNCE DEBA("Config data to all peers");
#else
#ifndef DEBA
#define DEBA(A)
#endif
#endif

  // send the configuration updates to the peers
  char cbuf[config_buffer_size];
  AmorphStore s(cbuf, config_buffer_size);
  unsigned filllevel = s.getSize();
  static const UDPPeerConfig clientmark(UDPPeerConfig::ClientPayload);

  // insert new entries found in the channels
  while (detected_entries.notEmpty()) {
    try {

      // find the indices first, channel + entry (replicator idx)
      uint16_t cid = detected_entries.front()->first;
      uint16_t rid = watched[cid]->next_id;
      const ChannelEntryInfo *ei = &(detected_entries.front()->second);

      // check that this does not correspond to a replicator entry
      WatchedChannel::writerlist_type::iterator ww =
        watched[cid]->writers.begin();
      while (ww != watched[cid]->writers.end() &&
             ww->second->getEntryId() != ei->entry_id) ww++;
      writerlist_type::iterator wc = candidate_writers.begin();
      while (wc != candidate_writers.end() &&
             wc->second->getEntryId() != ei->entry_id &&
             wc->first != cid) wc++;

      if (ww != watched[cid]->writers.end() ||
          wc != candidate_writers.end()) {
        DEB("channel " << cid <<
            " ignoring callback on replicator-written entry# " <<
            ei->entry_id <<
            (ww != watched[cid]->writers.end() ?
             " found in writers" : " found in candidates"));
        uint16_t peerid = (ww != watched[cid]->writers.end() ?
                           ww->second->getOrigin() : wc->second->getOrigin());
        if (w_replicatorinfo) {
          DataWriter<ReplicatorInfo> p(*w_replicatorinfo, ts);
          p.data().mtype = ReplicatorInfo::AddEntry;
          p.data().peer_id = peerid;
          p.data().name = ei->entry_label;
          p.data().entry_id = ei->entry_id;
          p.data().dataclass = ei->data_class;
          p.data().channelname = watched[cid]->channelname;
        }
      }
      else {

        // pack, to send to all peers
        ReplicatorConfig cf(ReplicatorConfig::AddEntry, 0U,
                            cid, rid, dueca::entry_end,
                            ei->entry_label,
                            ei->time_aspect, ei->arity, ei->packingmode,
                            ei->transportclass);
        addDataClass(cf, ei->data_class);
        ::packData(s, clientmark);
        ::packData(s, cf);
        DEBA(clientmark);
        DEBA(cf);
        filllevel = s.getSize();

        /* DUECA interconnect.

           Information on a local reading entry. */
        I_INT("Enabled local reading entry in channel " <<
              watched[cid]->channelname << " Rid " << rid);

        // packing worked, proceed with creating entry in readers
        watched[cid]->readers.push_back
          (std::shared_ptr<EntryReader>
           (new EntryReader(getId(), *ei, watched[cid]->channelname)));

        if (w_replicatorinfo) {
          DataWriter<ReplicatorInfo> p(*w_replicatorinfo, ts);
          p.data().mtype = ReplicatorInfo::AddEntry;
          p.data().peer_id = 0;
          p.data().name = ei->entry_label;
          p.data().entry_id = ei->entry_id;
          p.data().dataclass = ei->data_class;
          p.data().channelname = watched[cid]->channelname;
        }

        // set the correct replicator id, and increase count for next
        watched[cid]->readers.back()->setReplicatorEntryId(rid);
        watched[cid]->next_id++;
      }

      delete detected_entries.front();
      detected_entries.pop();
    }
    catch (const AmorphStoreBoundary& e) {
      // send over the buffer, and reset it to accept more data
      s.setSize(filllevel);
      distributeConfig(s);
      filllevel = 0;
    }
  }

  // check for new writers (from other nodes) or obsolete writers
  for (writerlist_type::iterator ww = candidate_writers.begin();
       ww != candidate_writers.end(); ) {
    try {

      // insert the candidate in the accepted writers, assign an
      // entry id
      uint16_t cid = ww->first;
      uint16_t rid = watched[cid]->next_id;

      // transmit configuration on this new writer
      ReplicatorConfig cf(ReplicatorConfig::AddEntry, ww->second->getOrigin(),
                          cid, rid,
                          ww->second->getReplicatorEntryId(),
                          ww->second->getLabel(),
                          ww->second->getEntryTimeAspect(),
                          ww->second->getEntryArity(), ww->second->getPackingMode(),
                          ww->second->getTransportClass());
      addDataClass(cf, ww->second->getDataClass());
      DEBA(clientmark);
      DEBA(cf);
      ::packData(s, clientmark);
      ::packData(s, cf);
      filllevel = s.getSize();

      // worked, can now set the final replicator ID and add to writers
      ww->second->setReplicatorEntryId(rid);
      watched[cid]->writers[rid] = ww->second;
      watched[cid]->next_id++;

      /* DUECA interconnect.

         Information on a local writing entry. */
      I_INT("Enabled local writing entry in channel " <<
            watched[cid]->channelname <<
            " entry# " << ww->second->getEntryId() <<
            " rid " << ww->second->getReplicatorEntryId());

      // over to next writer, remember fill level
      candidate_writers.pop_front();
      ww = candidate_writers.begin();
    }
    catch(const AmorphStoreBoundary& e) {

      // send over the buffer, and reset it to accept more data
      s.setSize(filllevel);
      distributeConfig(s);
      filllevel = 0;
    }
  }

  // obsolete writers are remote entries that have been removed from
  // a channel. Send information and delete; this will also delete
  // the write token
  for (writerlist_type::iterator ww = obsolete_writers.begin();
       ww != obsolete_writers.end(); ) {

    try {

      uint16_t cid = ww->first;
      // pack, to send to all peers
      ReplicatorConfig cf(ReplicatorConfig::RemoveEntry,
                          ww->second->getOrigin(),
                          cid, ww->second->getReplicatorEntryId());
      DEBA(clientmark);
      DEBA(cf);
      ::packData(s, clientmark);
      ::packData(s, cf);
      filllevel = s.getSize();

      /* DUECA interconnect.

         Information on the removal of a local writing entry in a
         channel. */
      I_INT("Removing local writing entry in channel " <<
            watched[cid]->channelname <<
            " entry# " << ww->second->getEntryId() <<
            " rid " << ww->second->getReplicatorEntryId());

      if (w_replicatorinfo) {
        DataWriter<ReplicatorInfo> p(*w_replicatorinfo, ts);
        p.data().mtype = ReplicatorInfo::RemoveEntry;
        p.data().peer_id = ww->second->getOrigin();
        p.data().entry_id = ww->second->getEntryId();
        p.data().channelname = watched[cid]->channelname;
      }

      // it worked, over to next obsolete entry
      obsolete_writers.pop_front();
      ww = obsolete_writers.begin();
    }
    catch(const AmorphStoreBoundary& e) {

      // send over the buffer, and reset it to accept more data
      s.setSize(filllevel);
      distributeConfig(s);
      filllevel = 0;
    }
  }

  // obsolete readers are channel entries that have been removed from
  // a channel. These may be monitored in the readers list, or written
  // by the replicator by an EntryWriter that has been removed
  while (deleted_entries.notEmpty()) {

    try {
      // find the reader currently in use
      uint16_t eid = deleted_entries.front()->second;
      uint16_t cid = deleted_entries.front()->first;
      WatchedChannel::readerlist_type::iterator rr =
        watched[cid]->readers.begin();

      while (rr != watched[cid]->readers.end() &&
             (*rr)->getEntryId() != eid) rr++;

      if (rr == watched[cid]->readers.end()) {

        /* DUECA interconnect.

           Information on a change in a locally watched channel. */
        I_INT("Channel " << watched[cid]->channelname <<
              " removed entry# " << eid <<
              " not in watched reader list");
      }
      else {
        ReplicatorConfig cf(ReplicatorConfig::RemoveEntry, 0U, cid,
                            (*rr)->getReplicatorEntryId());
        DEBA(clientmark);
        DEBA(cf);
        ::packData(s, clientmark);
        ::packData(s, cf);
        filllevel = s.getSize();

        /* DUECA interconnect.

           Information on removing a local entered entry. */
        I_INT("Channel " <<
              watched[cid]->channelname <<
              " removing entry# " << eid <<
              " rid " << (*rr)->getReplicatorEntryId());

        if (w_replicatorinfo) {
          DataWriter<ReplicatorInfo> p(*w_replicatorinfo, ts);
          p.data().mtype = ReplicatorInfo::RemoveEntry;
          p.data().peer_id = 0;
          p.data().entry_id = eid;
          p.data().channelname = watched[cid]->channelname;
        }

        watched[cid]->readers.erase(rr);
      }

      delete deleted_entries.front();
      deleted_entries.pop();
    }
    catch(const AmorphStoreBoundary& e) {

      // send over the buffer, and reset it to accept more data
      s.setSize(filllevel);
      distributeConfig(s);
      filllevel = 0;
    }
  }

  // send whatever is in the config buffer
  if (filllevel) {
    distributeConfig(s);
  }
}


void ChannelReplicatorMaster::
clientDecodeConfig(AmorphReStore& s, unsigned peer_id)
{
#ifdef DEBDEF
  DEBANNOUNCE DEBA("Decoding data from peer " +
                   boost::lexical_cast<std::string>(peer_id));
#else
#ifndef DEBA
#define DEBA(A)
#endif
#endif

  try {
    // read a single command, and remember where the buffer is at
    ReplicatorConfig cmd(s);
    DEB("decode config " << cmd);

    switch(cmd.mtype) {
    case ReplicatorConfig::AddEntry: {

      // throws and stops all if the dataclass tree is wrong
      verifyDataClass(cmd, peer_id);

      // add the entry to the candidate writers, this addition
      // will be processed in sendChannelConfigChanges
      candidate_writers.push_back
        (make_pair
         (cmd.channel_id, std::shared_ptr<EntryWriter>
          (new EntryWriter
           (getId(), peer_id, cmd.tmp_entry_id,
            watched[cmd.channel_id]->channelname,
            cmd.dataclass.front(), cmd.data_magic.front(), cmd.name,
            cmd.time_aspect, cmd.arity, cmd.packmode, cmd.tclass, getId()))));

      /* DUECA interconnect.

         Information on adding a local entered entry. */
      I_INT("Adding writer entry to candidates, from " <<
            cmd.slave_id << " RidT " << cmd.tmp_entry_id);

      break;
    }
    case  ReplicatorConfig::RemoveEntry: {

      // find the entry, and move it to the obsoletes, so it can be
      // notified and deleted
      WatchedChannel::writerlist_type::iterator ww =
        watched[cmd.channel_id]->writers.begin();
      while (ww != watched[cmd.channel_id]->writers.end() &&
             ww->first != cmd.entry_id) { ww++; }

      // check we found it
      if (ww == watched[cmd.channel_id]->writers.end()) {
        /* DUECA interconnect.

           There is an issue with removing a writer entry from a
           channel. Indicates a DUECA programming error. */
        W_INT("Cannot remove writer entry id " << cmd.entry_id <<
              " from channel " << watched[cmd.channel_id]->channelname);
        break;
      }

      // add to the obsoletes, and remove from the map
      obsolete_writers.push_back(make_pair(cmd.channel_id, ww->second));
      watched[cmd.channel_id]->writers.erase(ww);

      break;
    }
    default:
      /* DUECA interconnect.

         Received a request that cannot be processed. Indicates a
         DUECA programming error. */
      E_INT("Incorrect request from peer " << cmd.mtype);
    }
  }
  catch (const dueca::AmorphReStoreEmpty& e) {

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

NetCommunicatorMaster::VettingResult ChannelReplicatorMaster::
clientAuthorizePeer(CommPeer& peer, const TimeSpec& ts)
{
  DEB("Authorizing peer " << peer.send_id);
  // first check up the peerinfo channel
  while (r_peerinfo && r_peerinfo->getNumVisibleSets()) {
    DataReader<ReplicatorPeerAcknowledge> pa(*r_peerinfo);
    peer_acknowledgements[pa.data().peer_id] = pa.data();
  }

  // initially assume no data on this peer
  peer_ack_type::iterator ack = peer_acknowledgements.end();

  // if peerinfo channel used, look for guidance from the channel
  if (r_peerinfo != NULL) {
    ack = peer_acknowledgements.find(peer.send_id);
    if (ack != peer_acknowledgements.end()) {
      if (ack->second.reject) {
        return Reject;
      }
    }
  }

  if (r_peerinfo == NULL || ack != peer_acknowledgements.end()) {
    if (w_replicatorinfo) {
      DataWriter<ReplicatorInfo> p(*w_replicatorinfo, ts);
      p.data().mtype = ReplicatorInfo::AddPeer;
      p.data().peer_id = peer.send_id;
      p.data().name = peer.address;
      p.data().entry_id = 0;
    }
    return Accept;
  }
  return Delay;
}


/* walk through all watched & written entries, and make these obsolete */
void ChannelReplicatorMaster::
clientInfoPeerLeft(unsigned peer_id, const TimeSpec& ts)
{
  DEB("Goodbye to peer " << peer_id);
  if (w_replicatorinfo) {
    DataWriter<ReplicatorInfo> p(*w_replicatorinfo, ts);
    p.data().mtype = ReplicatorInfo::RemovePeer;
    p.data().peer_id = peer_id;
  }

  // clear matching entries
  for (channelmap_type::iterator cc = watched.begin();
       cc != watched.end(); cc++) {

    // move from the writers to a list with obsoletes; will be
    // cleaned in the config update
    for (WatchedChannel::writerlist_type::iterator ww =
           cc->second->writers.begin();
         ww != cc->second->writers.end(); ) {
      if (ww->second->getOrigin() == peer_id) {
        obsolete_writers.push_back(make_pair(cc->first, ww->second));
        WatchedChannel::writerlist_type::iterator toerase = ww;
        ww++;
        cc->second->writers.erase(toerase);
      }
      else {
        ww++;
      }
    }

    // directly remove from candidate writers
    for (writerlist_type::iterator
           ww = candidate_writers.begin();
         ww != candidate_writers.end(); ) {
      if (ww->second->getOrigin() == peer_id && ww->first == cc->first) {
        ww = candidate_writers.erase(ww);
      }
      else {
        ww++;
      }
    }
  }
}


bool ChannelReplicatorMaster::watchChannels
(const std::vector<std::string> &chlist)
{
  // continue numbering with size of watched map
  channel_id_t channel_id = watched.size();

  // add all named channels to the watched list
  for (std::vector<std::string>::const_iterator ii = chlist.begin();
       ii != chlist.end(); ii++) {
    watched[channel_id] = std::shared_ptr<WatchedChannel>
      (new WatchedChannel(*ii, channel_id, this));
    channel_id++;
  }

  // done
  return true;
}

void ChannelReplicatorMaster::clientUnpackPayload
(MessageBuffer::ptr_type buffer, unsigned peer_id,
 TimeTickType current_tick, TimeTickType i_peertick, int usecoffset)
{
  peer_timing[peer_id].adjustDelta(current_tick, i_peertick, false);
  ChannelReplicator::_clientUnpackPayload
    (buffer, peer_id, peer_timing[peer_id]);
}



ENDNSREPLICATOR


// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
// static TypeCreator<replicator::ChannelReplicatorMaster>
// a(replicator::ChannelReplicatorMaster::getMyParameterTable());
