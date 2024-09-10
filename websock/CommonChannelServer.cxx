/* ------------------------------------------------------------------   */
/*      item            : CommonChannelServer.cxx
        made by         : Rene' van Paassen
        date            : 181127
        category        : body file
        description     :
        changes         : 181127 first version
        language        : C++
        copyright       : (c) 18 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include <sstream>
#define CommonChannelServer_cxx
#include "CommonChannelServer.hxx"
#include <boost/lexical_cast.hpp>
#include <debug.h>
#define NO_TYPE_CREATION
#define DO_INSTANTIATE
#include "WebSocketsServer.hxx"
#include <dueca.h>
#include <dueca/CommObjectReaderWriter.hxx>
#include <dueca/CommObjectWriter.hxx>
#include <dueca/DataClassRegistry.hxx>

#include "WebsockExceptions.hxx"

#define DEBPRINTLEVEL 1
#include <debprint.h>

DUECA_NS_START;
WEBSOCK_NS_START;

const char *presetmismatch::what() const throw()
{
  return "websocket preset does not match client data";
}

const char *connectionparseerror::what() const throw()
{
  return "Parse error at connection defining message";
}

const char *connectionconfigerror::what() const throw()
{
  return "Requested configuration is not possible";
}

const char *dataparseerror::what() const throw()
{
  return "Parse error at data message";
}

SingleEntryRead::SingleEntryRead(const std::string &channelname,
                                 const std::string &datatype, entryid_type eid,
                                 const GlobalId &master) :
  r_token(master, NameSet(channelname), datatype, eid, Channel::AnyTimeAspect,
          Channel::OneOrMoreEntries, Channel::JumpToMatchTime),
  datatype(datatype)
{
  r_token.isValid();
}

SingleEntryRead::~SingleEntryRead() {}

SingleEntryFollow::SingleEntryFollow(
  const std::string &channelname, const std::string &datatype, entryid_type eid,
  const WebSocketsServerBase *master, const PrioritySpec &ps,
  const DataTimeSpec &ts, bool extended, unsigned char marker, bool autostart) :
  ConnectionList(channelname + std::string(" (entry ") +
                 boost::lexical_cast<std::string>(eid) + std::string(")"), marker),
  master(master), autostart_cb(this, &SingleEntryFollow::tokenValid),
  r_token(master->getId(), NameSet(channelname), datatype, eid,
          Channel::AnyTimeAspect, Channel::OneOrMoreEntries,
          Channel::ReadAllData, 0.0, autostart ? &autostart_cb : NULL),
  cb(this, &SingleEntryFollow::passData),
  do_calc(master->getId(), "read for server", &cb, ps), datatype(datatype),
  inactive(true), extended(extended), firstwrite(true)
{
  if (ts.getValiditySpan() != 0) {
    regulator.reset(new TriggerRegulatorGreedy(r_token, ts));
    DEB("SingleEntryFollow " << identification << " regulator " << ts);
    do_calc.setTrigger(regulator);
  }
  else {
    do_calc.setTrigger(r_token);
  }
}

SingleEntryFollow::~SingleEntryFollow() { do_calc.clearTriggers(); }

void SingleEntryFollow::disconnect() { do_calc.clearTriggers(); }

void SingleEntryFollow::tokenValid(const TimeSpec &ts)
{
  /** The callback for this is only connected when autostart was active */
  if (inactive) {
    DEB("Starting follow activity " << identification << " at " << ts);
    do_calc.switchOn(ts);
    inactive = false;
  }
}

ConnectionList::ConnectionList(const std::string &ident, unsigned char marker) :
  marker(marker), flock(ident.c_str(), false), identification(ident)
{}

ConnectionList::~ConnectionList() {}

void ConnectionList::addConnection(std::shared_ptr<WsServer::Connection> &c)
{
  // ScopeLock l(flock);
  connections.push_back(c);
}

void ConnectionList::addConnection(std::shared_ptr<WssServer::Connection> &c)
{
  // ScopeLock l(flock);
  sconnections.push_back(c);
}

bool ConnectionList::removeConnection(
  const std::shared_ptr<WsServer::Connection> &c)
{
  // ScopeLock l(flock);
  auto toremove = std::find(connections.begin(), connections.end(), c);
  if (toremove != connections.end()) {
    connections.erase(toremove);
    return true;
  }
  return false;
}

bool ConnectionList::removeConnection(
  const std::shared_ptr<WssServer::Connection> &c)
{
  // ScopeLock l(flock);
  auto toremove = std::find(sconnections.begin(), sconnections.end(), c);
  if (toremove != sconnections.end()) {
    sconnections.erase(toremove);
    return true;
  }
  return false;
}

void ConnectionList::sendAll(const std::string &data, const char *desc)
{
  // send to all existing connections
  for (auto &cn : connections) {
    sendOne(data, desc, cn);
  }
  for (auto &cn : sconnections) {
    sendOne(data, desc, cn);
  }
}

void ConnectionList::sendOne(const std::string &data, const char *desc,
                             const std::shared_ptr<WssServer::Connection> &cn)
{
  cn->send(data, [cn, this, desc](const SimpleWeb::error_code &ec) {
    if (ec) {
        /* DUECA websockets.

       Error in a send action, will remove the connection from the
       list of clients. */
      W_XTR("Error sending " << desc << ", " << ec.message()
                             << " removing connenction form "
                             << this->identification);
      this->removeConnection(cn);
    }
  }, marker);
}

void ConnectionList::sendOne(const std::string &data, const char *desc,
                             const std::shared_ptr<WsServer::Connection> &cn)
{
  cn->send(data, [cn, this, desc](const SimpleWeb::error_code &ec) {
    if (ec) {
        /* DUECA websockets.

       Error in a send action, will remove the connection from the
       list of clients. */
      W_XTR("Error sending " << desc << ", " << ec.message()
                             << " removing connenction form "
                             << this->identification);
      this->removeConnection(cn);
    }
  }, marker);
}

void SingleEntryFollow::passData(const TimeSpec &ts)
{
  if (firstwrite || regulator) {
    r_token.flushOlderSets(ts.getValidityStart());
    firstwrite = false;
  }

  // Fix for initial triggering when not enough data in the channel,
  // cause not exactly clear @TODO investigate
  if (!r_token.haveVisibleSets(ts)) {
    DEB("SingleEntryFollow, no data for time step " << ts);
    return;
  }

  // ScopeLock l(flock);
  DCOReader r(datatype.c_str(), r_token, ts);

  std::stringstream buffer;
  master->codeData(buffer, r);
  sendAll(buffer.str(), "channel data");
}

bool SingleEntryFollow::checkToken()
{
  bool res = r_token.isValid();
  if (!res) {
    /* DUECA websockets.

       The channel reading token for a "read" URL is not valid. */
    W_XTR("Channel following token not (yet) valid for " << identification);
  }
  return res;
}

bool SingleEntryFollow::start(const TimeSpec &ts)
{
  if (inactive) {
    if (r_token.isValid()) {
      do_calc.switchOn(ts);
      inactive = false;
      return true;
    }
    return false;
  }
  return true;
}

bool SingleEntryFollow::stop(const TimeSpec &ts)
{
  if (inactive)
    return false;
  do_calc.switchOff(ts);
  inactive = true;
  firstwrite = true;
  return true;
}

ChannelMonitor::ChannelMonitor(const WebSocketsServerBase *server,
                               const std::string &channelname,
                               const DataTimeSpec &ts,
                               unsigned char marker) :
  ChannelWatcher(channelname), ConnectionList(channelname, marker),
  channelname(channelname), time_spec(ts), server(server)
{}

ChannelMonitor::~ChannelMonitor() {}

void ChannelMonitor::entryAdded(const ChannelEntryInfo &info)
{
  // ScopeLock l(flock);

  if (info.entry_id >= entrydataclass.size()) {
    entrydataclass.resize(info.entry_id + 1);
  }
  assert(entrydataclass[info.entry_id].size() == 0);
  entrydataclass[info.entry_id] = info.data_class;

  // let the server code entry type information
  std::stringstream buffer;
  server->codeEntryInfo(buffer, "", entry_end, info.data_class, info.entry_id);
  sendAll(buffer.str(), "entry addition");
}

void ChannelMonitor::entryRemoved(const ChannelEntryInfo &info)
{
  // ScopeLock l(flock);

  assert(info.entry_id < entrydataclass.size() &&
         entrydataclass[info.entry_id].size() > 0);
  entrydataclass[info.entry_id] = std::string();

  // let the server code empty entry type information
  std::stringstream buffer;
  server->codeEntryInfo(buffer, "", entry_end, "", info.entry_id);
  DEB("entryRemoved " << info.entry_id);
  sendAll(buffer.str(), "entry removal");
}

void ChannelMonitor::addConnection(std::shared_ptr<WsServer::Connection> &c)
{
  ConnectionList::addConnection(c);
  // ScopeLock l(flock);
  for (size_t ii = 0; ii < entrydataclass.size(); ii++) {
    if (entrydataclass[ii].size()) {
      std::stringstream buffer;
      server->codeEntryInfo(buffer, "", entry_end, entrydataclass[ii], ii);
      sendOne(buffer.str(), "entry catch up", c);
    }
  }
}

void ChannelMonitor::addConnection(std::shared_ptr<WssServer::Connection> &c)
{
  ConnectionList::addConnection(c);
  // ScopeLock l(flock);
  for (size_t ii = 0; ii < entrydataclass.size(); ii++) {
    if (entrydataclass[ii].size()) {
      std::stringstream buffer;
      server->codeEntryInfo(buffer, "", entry_end, entrydataclass[ii], ii);
      sendOne(buffer.str(), "entry catch up", c);
    }
  }
}

const std::string &ChannelMonitor::findEntry(unsigned entryid)
{
  // ScopeLock l(flock);
  static std::string empty;
  if (entryid >= entrydataclass.size()) {
    return empty;
  }
  return entrydataclass[entryid];
}

WriteableSetup::WriteableSetup(const std::string &channelname,
                               const std::string &dataclass) :
  channelname(channelname), dataclass(dataclass)
{}

CODE_REFCOUNT(WriteEntry);

WriteEntry::WriteEntry(const std::string &channelname,
                       const std::string &datatype, bool bulk, bool diffpack,
                       WriteEntry::WEState initstate) :
  INIT_REFCOUNT_COMMA state(initstate), w_token(),
  identification("not initialized"), channelname(channelname),
  datatype(datatype), ctiming(false), active(true), stream(false), bulk(bulk),
  diffpack(diffpack)
{}

WriteEntry::~WriteEntry()
{
  //
}

void WriteEntry::complete(const std::string& datatype, const std::string& label, 
  bool stream, bool ctiming, bool bulk, bool diffpack, const GlobalId &master)
{
  this->datatype = datatype;
  this->ctiming = ctiming;
  this->stream = stream;
  if (stream && !ctiming) {
    throw connectionconfigerror();
  }
  this->bulk = bulk;
  this->diffpack = diffpack;

  identification = channelname + std::string(" type:") + datatype +
                   std::string(" label:\"") + label + std::string("\"");
  w_token.reset(new ChannelWriteToken(
    master, NameSet(channelname), datatype, label,
    stream ? Channel::Continuous : Channel::Events, Channel::OneOrMoreEntries,
    diffpack ? Channel::MixedPacking : Channel::OnlyFullPacking,
    bulk ? Channel::Bulk : Channel::Regular));
  checkToken();
  state = Linked;
}

bool WriteEntry::checkToken()
{
  bool res = w_token->isValid();
  if (!res) {
    /* DUECA websockets.

       The channel write token for a "write" URL entry is not (yet)
       valid. Check your configuration and opening mode. */
    W_XTR("Channel writing token not (yet) valid for " << identification);
  }
  return res;
}


PresetWriteEntry::PresetWriteEntry(const std::string &channelname,
                                   const std::string &datatype,
                                   const std::string &label,
                                   const GlobalId &master, bool ctiming,
                                   bool stream, bool bulk, bool diffpack) :
  WriteEntry(channelname, datatype, bulk, diffpack, UnConnected)
{
  this->ctiming = ctiming;
  this->stream = stream;
  this->identification = channelname + std::string(" type:") + datatype +
                         std::string(" label:\"") + label + std::string("\"");
  w_token.reset(new ChannelWriteToken(
    master, NameSet(channelname), datatype, label,
    stream ? Channel::Continuous : Channel::Events, Channel::OneOrMoreEntries,
    diffpack ? Channel::MixedPacking : Channel::OnlyFullPacking,
    bulk ? Channel::Bulk : Channel::Regular));
  checkToken();
}

PresetWriteEntry::~PresetWriteEntry()
{
  //
}

void PresetWriteEntry::complete(const std::string &datatype, const std::string &label,
                                bool _stream, bool _ctiming, bool _bulk, bool _diffpack,
                                const GlobalId &master)
{
  if (_ctiming != this->ctiming || _stream != this->stream) {
    throw(presetmismatch());
  }
  state = Linked;
  checkToken();
}

void PresetWriteEntry::doConnect(connection_t connection)
{
  this->connection = connection;
  WriteEntry::doConnect();
}

void PresetWriteEntry::doConnect(sconnection_t sconnection)
{
  this->sconnection = sconnection;
  WriteEntry::doConnect();
}

void *PresetWriteEntry::disConnect()
{
  const std::string reason("Resource re-allocation to new client");
  void *res = NULL;
  if (connection) {
    connection->send_close(1000, reason);
    res = connection.get();
    connection.reset();
  }
  else if (sconnection) {
    sconnection->send_close(1000, reason);
    res = sconnection.get();
    sconnection.reset();
  }
  else {
    /* DUECA websockets.

       Attempt to disconnect a connection to a preset
       (pre-configured and connected) write url failed.
     */
    W_XTR("Cannot find preset writer for closing");
  }
  WriteEntry::doDisconnect();
  return res;
}

NameEntryId::NameEntryId(const std::string &name, unsigned id) :
  name(name), id(id)
{}

bool NameEntryId::operator<(const NameEntryId &other) const
{
  if (this->name < other.name)
    return true;
  if (this->name > other.name)
    return false;
  if (this->id < other.id)
    return true;
  return false;
}

NameEntryTokenId::NameEntryTokenId(const std::string &name, unsigned id,
                                   const std::string token) :
  name(name), id(id), token(token)
{}

bool NameEntryTokenId::operator<(const NameEntryTokenId &other) const
{
  if (this->name < other.name)
    return true;
  if (this->name > other.name)
    return false;
  if (this->id < other.id)
    return true;
  if (this->id > other.id)
    return true;
  if (this->token < other.token)
    return true;
  return false;
}

NameTokenId::NameTokenId(const std::string &name, const std::string token) :
  name(name), token(token)
{}

bool NameTokenId::operator<(const NameTokenId &other) const
{
  if (this->name < other.name)
    return true;
  if (this->name > other.name)
    return false;
  if (this->token < other.token)
    return true;
  return false;
}

WriteReadSetup::WriteReadSetup(const std::string &wchannelname,
                               const std::string &rchannelname) :
  cnt_clients(0U), w_channelname(wchannelname), r_channelname(rchannelname),
  bulk(false), diffpack(false)
{
  //
}

unsigned WriteReadSetup::getNextId() { return cnt_clients++; }

CODE_REFCOUNT(WriteReadEntry);

WriteReadEntry::WriteReadEntry(std::shared_ptr<WriteReadSetup> setup,
                               WebSocketsServerBase *master,
                               const PrioritySpec &ps, bool extended,
                               unsigned char marker) :
  ChannelWatcher(setup->r_channelname),
  INIT_REFCOUNT_COMMA autostart_cb(this, &WriteReadEntry::tokenValid),
  marker(marker),
  state(UnConnected), w_token(), r_token(), identification("not initialized"),
  w_channelname(setup->w_channelname), r_channelname(setup->r_channelname),
  w_dataclass(), r_dataclass(),
  label(boost::lexical_cast<std::string>(setup->getNextId())), master(master),
  active(true), bulk(setup->bulk), diffpack(setup->diffpack),
  extended(extended), cb(this, &WriteReadEntry::passData),
  do_calc(master->getId(), "read for server", &cb, ps)
{
  //
}

WriteReadEntry::~WriteReadEntry()
{
  //
}

void WriteReadEntry::setConnection(connection_t connection)
{
  assert(!this->connection.get() && !this->sconnection.get());
  this->connection = connection;
  state = Connected;
}

void WriteReadEntry::setConnection(sconnection_t connection)
{
  assert(!this->connection.get() && !this->sconnection.get());
  this->sconnection = connection;
  state = Connected;
}

void WriteReadEntry::complete(const std::string &w_dataclass)
{
  assert(state == Connected);
  identification = w_channelname + std::string(" type:") + w_dataclass +
                   std::string(" label:\"") + label + std::string("\" <-> ") +
                   r_channelname;

  w_token.reset(new ChannelWriteToken(
    master->getId(), NameSet(w_channelname), w_dataclass, label,
    Channel::Events, Channel::OneOrMoreEntries,
    diffpack ? Channel::MixedPacking : Channel::OnlyFullPacking,
    bulk ? Channel::Bulk : Channel::Regular, &autostart_cb));
  state = ValidatingTokens;
}

bool WriteReadEntry::checkToken()
{
  return state == Linked;
}

void WriteReadEntry::entryAdded(const ChannelEntryInfo &i)
{
  DEB("WriteReadEntry::entryAdded " << i);
  if (state == ValidatingTokens && i.entry_label == label) {
    r_dataclass = i.data_class;
    assert(!r_token);
    r_token.reset(new ChannelReadToken(
      master->getId(), NameSet(r_channelname), r_dataclass, i.entry_id,
      i.time_aspect, i.arity, Channel::ReadAllData, 0.0, &autostart_cb));
    /* if (checkToken()) {
      state = Linked;
    } */
  }
}

const GlobalId &WriteReadEntry::getId() const { return master->getId(); }

void WriteReadEntry::tokenValid(const TimeSpec &ts)
{
   if (w_token->isValid() && r_token && r_token->isValid() && state == ValidatingTokens) {

    std::stringstream buf;
    master->codeEntryInfo(buf, w_dataclass, w_token->getEntryId(),
      r_dataclass, r_token->getEntryId());
    DEB("Write read entry, tokens valid " << buf.str());

    // both tokens valid now, return information on entries
    sendOne(buf.str(), "WriterReader info");
      state = Linked;
      do_calc.setTrigger(*r_token);
      do_calc.switchOn();
  }
}

void WriteReadEntry::passData(const TimeSpec &ts)
{
#if 0
  if (state == ExpectingRead) {
    std::stringstream buf;
    master->codeEntryInfo(buf, w_dataclass, w_token->getEntryId(),
      r_dataclass, r_token->getEntryId());
    // both tokens valid now, return information on entries
    sendOne(buf.str(), "WriterReader info");
    state = Linked;
  }
#endif
  DCOReader r(r_dataclass.c_str(), *r_token, ts);
  /*
  DataTimeSpec dtd = r.timeSpec();
  writer.StartObject(2);
  writer.Key("tick");
  writer.Uint(dtd.getValidityStart());
  writer.Key("data");
  writer.dco(r);
  writer.EndObject();

  DEB2("WriteReadEntry::passData " << doc.GetString());
  sendOne(doc.GetString(), "channel data");
  */
  std::stringstream buf;
  master->codeData(buf, r);
  sendOne(buf.str(), "channel data");
}

void WriteReadEntry::sendOne(const std::string &data, const char *desc)
{
  if (connection) {
    connection->send(data, [this, desc](const SimpleWeb::error_code &ec) {
      if (ec) {
           /* DUECA websockets.

        Error in a send action for a "write-and-read" URL, will
        remove the connection from the list of clients. */
        W_XTR("Error sending " << desc << ", " << ec.message()
                               << " removing connenction form "
                               << this->identification);
      }
    }, marker);
  }
  else {
    sconnection->send(data, [this, desc](const SimpleWeb::error_code &ec) {
      if (ec) {
           /* DUECA websockets.

        Error in a send action for a "write-and-read" URL, will
        remove the connection from the list of clients. */
        W_XTR("Error sending " << desc << ", " << ec.message()
                               << " removing connenction form "
                               << this->identification);
      }
    }, marker);
  }
}

void WriteReadEntry::entryRemoved(const ChannelEntryInfo &i)
{
  if (i.entry_label == label) {

    const std::string reason("Termination of service.");
    if (connection) {
      connection->send_close(1000, reason);
      connection.reset();
    }
    if (sconnection) {
      sconnection->send_close(1000, reason);
      sconnection.reset();
    }
    state = DisConnected;
    r_token.reset();
    w_token.reset();
  }
}


DUECA_NS_END;
WEBSOCK_NS_END;
