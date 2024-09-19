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

#include "PrioritySpec.hxx"
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

#define DEBPRINTLEVEL -1
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
                                 const WebSocketsServerBase *master,
                                 const PrioritySpec &ps) :
  ConnectionList(channelname + std::string("(entry :)") +
                 boost::lexical_cast<std::string>(eid) + std::string(")"), master),
  autostart_cb(this, &SingleEntryRead::tokenValid),
  do_valid(master->getId(), "token valid", &autostart_cb, ps),
  r_token(master->getId(), NameSet(channelname), datatype, eid, Channel::AnyTimeAspect,
          Channel::OneOrMoreEntries, Channel::JumpToMatchTime, 0.1, &do_valid),
  datatype(datatype),
  inactive(true)
{
  do_valid.switchOn();
}

SingleEntryRead::~SingleEntryRead() {}

const GlobalId &ConnectionList::getId() { return master->getId(); }

template<typename C>
void SingleEntryRead::addConnection(C &c)
{
  if (!inactive) {
    // send the entry configuration in the first message
    std::stringstream buf;

    master->codeEntryInfo(buf, "", 0, datatype, r_token.getEntryId());

    sendOne(buf.str(), "WriterReader info", c);
  }
  this->ConnectionList::addConnection(c);
}

void SingleEntryRead::close(const char* reason, int status)
{
  this->ConnectionList::close(reason, status);
  inactive = true;
}

template
void SingleEntryRead::addConnection<std::shared_ptr<WsServer::Connection> >
  (std::shared_ptr<WsServer::Connection>& connection);
template
void SingleEntryRead::addConnection<std::shared_ptr<WssServer::Connection> >
  (std::shared_ptr<WssServer::Connection>& connection);


template<typename C>
void SingleEntryRead::passData(const TimeSpec &ts, C& connection)
{
  std::stringstream buffer;
  DEB("SingleEntryRead::passData " << ts);

  // Fix for initial triggering when not enough data in the channel,
  // cause not exactly clear @TODO investigate
  if (!inactive && r_token.haveVisibleSets()) {
    DCOReader r(datatype.c_str(), r_token);
    master->codeData(buffer, r);
  }
  else {
    DEB("SingleEntryRead, no data for time step " << ts);
    master->codeEmpty(buffer);
  }
  sendOne(buffer.str(), "channel data", connection);
}

template
void SingleEntryRead::passData<std::shared_ptr<WsServer::Connection> >
  (const TimeSpec &ts,
  std::shared_ptr<WsServer::Connection>& connection);
template
void SingleEntryRead::passData<std::shared_ptr<WssServer::Connection> >
  (const TimeSpec &ts,
  std::shared_ptr<WssServer::Connection>& connection);

void SingleEntryRead::tokenValid(const TimeSpec &ts)
{
  if (inactive) {
    // send the entry configuration in the first message
    std::stringstream buf;

    master->codeEntryInfo(buf, "", 0, datatype, r_token.getEntryId());
    DEB("read entry, token valid " << buf.str());

    sendAll(buf.str(), "WriterReader info");
    inactive = false;
  }
}

bool SingleEntryRead::checkToken()
{
  bool res = r_token.isValid();
  if (!res) {
    /* DUECA websockets.

       The channel read token for a "current" URL entry is not (yet)
       valid. Check your configuration and opening mode. */
    W_XTR("Channel read token not (yet) valid for " << identification);
  }
  return res;
}


SingleEntryFollow::SingleEntryFollow(
  const std::string &channelname, const std::string &datatype, entryid_type eid,
  const WebSocketsServerBase *master, const PrioritySpec &ps,
  const DataTimeSpec &ts) :
  ConnectionList(channelname + std::string(" (entry ") +
                   boost::lexical_cast<std::string>(eid) + std::string(")"),
                 master),
  autostart_cb(this, &SingleEntryFollow::tokenValid),
  do_valid(master->getId(), "token valid", &autostart_cb, ps),
  r_token(master->getId(), NameSet(channelname), datatype, eid,
          Channel::AnyTimeAspect, Channel::OneOrMoreEntries,
          Channel::ReadAllData, 0.0, &do_valid),
  cb(this, &SingleEntryFollow::passData),
  do_calc(master->getId(), "read for server", &cb, ps),
  datatype(datatype),
  inactive(true),
  firstwrite(true)
{
  do_valid.switchOn();
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
  if (inactive) {
    // send the entry configuration in the first message

    DEB("Starting follow activity " << identification << " at " << ts);
    do_calc.switchOn(ts);
    std::stringstream buf;

    master->codeEntryInfo(buf, "", 0, datatype, r_token.getEntryId());
    DEB("follow entry, token valid " << buf.str());

    sendAll(buf.str(), "WriterReader info");
    inactive = false;
  }
}

template<typename C>
void SingleEntryFollow::addConnection(C &c)
{
  if (!inactive) {
    std::stringstream buf;
    master->codeEntryInfo(buf, "", 0, datatype, r_token.getEntryId());
    sendOne(buf.str(), "Read targeted info", c);
  }
  this->ConnectionList::addConnection(c);
}

template
void SingleEntryFollow::addConnection<std::shared_ptr<WsServer::Connection> >
  (std::shared_ptr<WsServer::Connection>& connection);
template
void SingleEntryFollow::addConnection<std::shared_ptr<WssServer::Connection> >
  (std::shared_ptr<WssServer::Connection>& connection);

bool SingleEntryFollow::checkToken()
{
  bool res = r_token.isValid();
  if (!res) {
    /* DUECA websockets.

       The channel read token for a "read" URL entry is not (yet)
       valid. Check your configuration and opening mode. */
    W_XTR("Channel read token not (yet) valid for " << identification);
  }
  return res;
}

ConnectionList::ConnectionList(const std::string &ident, const WebSocketsServerBase *master) :
  marker(master->getMarker()),
  master(master),
  identification(ident)
{}

ConnectionList::~ConnectionList() {}

void ConnectionList::close(const char* reason, int status)
{
  for (auto &c: connections) {
    c->send_close(status, reason);
  }
  for (auto &c: sconnections) {
    c->send_close(status, reason);
  }
}

void ConnectionList::addConnection(std::shared_ptr<WsServer::Connection> &c)
{
  connections.push_back(c);
}

void ConnectionList::addConnection(std::shared_ptr<WssServer::Connection> &c)
{
  sconnections.push_back(c);
}

bool ConnectionList::removeConnection(
  const std::shared_ptr<WsServer::Connection> &c)
{
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

template<typename C>
void ConnectionList::sendOne(const std::string &data, const char *desc,
                             const C &cn)
{
  DEB("ConnectionList::sendOne " << desc << data);
  cn->send(
    data,
    [cn, this, desc](const SimpleWeb::error_code &ec) {
      if (ec) {
        /* DUECA websockets.

       Error in a send action, will remove the connection from the
       list of clients. */
        W_XTR("Error sending " << desc << ", " << ec.message()
                               << " removing connenction form "
                               << this->identification);
        this->removeConnection(cn);
      }
    },
    marker);
}

template
void ConnectionList::sendOne<std::shared_ptr<WsServer::Connection> >(const std::string &data, const char *desc,
                             const std::shared_ptr<WsServer::Connection> &cn);
template
void ConnectionList::sendOne<std::shared_ptr<WssServer::Connection> >(const std::string &data, const char *desc,
                             const std::shared_ptr<WssServer::Connection> &cn);

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

  DCOReader r(datatype.c_str(), r_token, ts);

  std::stringstream buffer;
  master->codeData(buffer, r);
  sendAll(buffer.str(), "channel data");
}

void SingleEntryFollow::close(const char* reason, int status)
{
  this->ConnectionList::close(reason, status);
  do_calc.switchOff();
}


bool SingleEntryFollow::start(const TimeSpec &ts)
{
  if (inactive) return false;
  if (r_token.isValid()) {
    do_calc.switchOn(ts);
  }
  return true;
}

bool SingleEntryFollow::stop(const TimeSpec &ts)
{
  if (inactive)
    return false;
  do_calc.switchOff(ts);
  firstwrite = true;
  return true;
}

ChannelMonitor::ChannelMonitor(const WebSocketsServerBase *server,
                               const std::string &channelname,
                               const DataTimeSpec &ts) :
  ChannelWatcher(channelname),
  ConnectionList(channelname,  server),
  channelname(channelname),
  time_spec(ts)
{}

ChannelMonitor::~ChannelMonitor() {}

void ChannelMonitor::entryAdded(const ChannelEntryInfo &info)
{
  if (info.entry_id >= entrydataclass.size()) {
    entrydataclass.resize(info.entry_id + 1);
  }
  assert(entrydataclass[info.entry_id].size() == 0);
  entrydataclass[info.entry_id] = info.data_class;

  // let the server code entry type information
  std::stringstream buffer;
  master->codeEntryInfo(buffer, "", entry_end, info.data_class, info.entry_id);
  sendAll(buffer.str(), "entry addition");
}

void ChannelMonitor::entryRemoved(const ChannelEntryInfo &info)
{
  assert(info.entry_id < entrydataclass.size() &&
         entrydataclass[info.entry_id].size() > 0);
  entrydataclass[info.entry_id] = std::string();

  // let the server code empty entry type information
  std::stringstream buffer;
  master->codeEntryInfo(buffer, "", entry_end, "", info.entry_id);
  DEB("entryRemoved " << info.entry_id);
  sendAll(buffer.str(), "entry removal");
}

void ChannelMonitor::addConnection(std::shared_ptr<WsServer::Connection> &c)
{
  ConnectionList::addConnection(c);
  for (size_t ii = 0; ii < entrydataclass.size(); ii++) {
    if (entrydataclass[ii].size()) {
      std::stringstream buffer;
      master->codeEntryInfo(buffer, "", entry_end, entrydataclass[ii], ii);
      sendOne(buffer.str(), "entry catch up", c);
    }
  }
}

void ChannelMonitor::addConnection(std::shared_ptr<WssServer::Connection> &c)
{
  ConnectionList::addConnection(c);
  for (size_t ii = 0; ii < entrydataclass.size(); ii++) {
    if (entrydataclass[ii].size()) {
      std::stringstream buffer;
      master->codeEntryInfo(buffer, "", entry_end, entrydataclass[ii], ii);
      sendOne(buffer.str(), "entry catch up", c);
    }
  }
}

const std::string &ChannelMonitor::findEntry(unsigned entryid)
{
  static std::string empty;
  if (entryid >= entrydataclass.size()) {
    return empty;
  }
  return entrydataclass[entryid];
}

WriteableSetup::WriteableSetup(const std::string &channelname,
                               const std::string &dataclass) :
  channelname(channelname),
  dataclass(dataclass)
{}

CODE_REFCOUNT(WriteEntry);

WriteEntry::WriteEntry(const std::string &channelname,
                       const std::string &datatype, 
                       const WebSocketsServerBase *master, const PrioritySpec &ps,
                       bool bulk, bool diffpack,
                       WriteEntry::WEState initstate) :
  INIT_REFCOUNT_COMMA 
  state(initstate),
  master(master),
  autostart_cb(this, &WriteEntry::tokenValid),
  do_valid(master->getId(), "", &autostart_cb, ps),
  w_token(),
  identification("not initialized"),
  channelname(channelname),
  datatype(datatype),
  ctiming(false),
  active(true),
  stream(false),
  bulk(bulk),
  diffpack(diffpack)
{
  do_valid.switchOn();  
}

WriteEntry::~WriteEntry()
{
  //
}

const GlobalId& WriteEntry::getId()
{
  return master->getId();
}

void WriteEntry::complete(const std::string &datatype, const std::string &label,
                          bool stream, bool ctiming, bool bulk, bool diffpack)
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
    master->getId(), NameSet(channelname), datatype, label,
    stream ? Channel::Continuous : Channel::Events, Channel::OneOrMoreEntries,
    diffpack ? Channel::MixedPacking : Channel::OnlyFullPacking,
    bulk ? Channel::Bulk : Channel::Regular, &do_valid));
  //checkToken();
  state = Connected;
}

void WriteEntry::close(const char* reason, int status)
{
  w_token.reset();
  if (connection) {
    connection->send_close(status, reason);
    connection.reset();
  }
  if (sconnection) {
    sconnection->send_close(status, reason);
    sconnection.reset();
  }
  state = UnConnected;
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

void WriteEntry::tokenValid(const TimeSpec &ts)
{
  if (state == Connected) {
    // send the entry configuration in the first message
    std::stringstream buf;

    master->codeEntryInfo(buf, datatype, w_token->getEntryId(),  "", entry_end);
    DEB("write entry, token valid " << buf.str());

    sendOne(buf.str(), "WriterReader info");
    state = Linked;
  }

}

template<>
void WriteEntry::setConnection<std::shared_ptr<WsServer::Connection> >(std::shared_ptr<WsServer::Connection>& c)
{
  connection = c;
}

template<>
void WriteEntry::setConnection<std::shared_ptr<WssServer::Connection> >(std::shared_ptr<WssServer::Connection>& c)
{
  sconnection = c;
}

void WriteEntry::sendOne(const std::string &data, const char *desc)
{
  DEB("WriteEntry::sendOne " << data);
  if (connection) {
    connection->send(
      data,
      [this, desc](const SimpleWeb::error_code &ec) {
        if (ec) {
        /* DUECA websockets.

              Error in a send action for a "write" URL, will
              remove the connection from the list of clients. */
          W_XTR("Error sending " << desc << ", " << ec.message()
                                 << " removing connenction form "
                                 << this->identification);
        }
      },
      master->getMarker());
  }
  else {
    sconnection->send(
      data,
      [this, desc](const SimpleWeb::error_code &ec) {
        if (ec) {
        /* DUECA websockets.

              Error in a send action for a "write" URL, will
              remove the connection from the list of clients. */
          W_XTR("Error sending " << desc << ", " << ec.message()
                                 << " removing connenction form "
                                 << this->identification);
        }
      },
      master->getMarker());
  }
}

PresetWriteEntry::PresetWriteEntry(const std::string &channelname,
                                   const std::string &datatype,
                                   const std::string &label,
                                   const WebSocketsServerBase *master, const PrioritySpec& ps,
                                   bool ctiming, bool stream, bool bulk, bool diffpack) :
  WriteEntry(channelname, datatype, master, ps, bulk, diffpack, UnConnected)
{
  this->ctiming = ctiming;
  this->stream = stream;
  this->identification = channelname + std::string(" type:") + datatype +
                         std::string(" label:\"") + label + std::string("\"");
  w_token.reset(new ChannelWriteToken(
    master->getId(), NameSet(channelname), datatype, label,
    stream ? Channel::Continuous : Channel::Events, Channel::OneOrMoreEntries,
    diffpack ? Channel::MixedPacking : Channel::OnlyFullPacking,
    bulk ? Channel::Bulk : Channel::Regular, &do_valid));
}

PresetWriteEntry::~PresetWriteEntry()
{
  //
}

void PresetWriteEntry::complete(const std::string &datatype,
                                const std::string &label, bool _stream,
                                bool _ctiming, bool _bulk, bool _diffpack)
{
  if (_ctiming != this->ctiming || _stream != this->stream) {
    throw(presetmismatch());
  }
  state = Linked;
  checkToken();
}

void PresetWriteEntry::close(const char* reason, int status)
{
  // does not reset the token
  if (connection) {
    connection->send_close(status, reason);
    connection.reset();
  }
  if (sconnection) {
    sconnection->send_close(status, reason);
    sconnection.reset();
  }
  state = UnConnected;
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
  name(name),
  id(id)
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
  name(name),
  id(id),
  token(token)
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
  name(name),
  token(token)
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
  w_channelname(wchannelname),
  r_channelname(rchannelname),
  bulk(false),
  diffpack(false)
{
  //
}

unsigned WriteReadSetup::getNextId() { 
  static unsigned nid = 0;
  return nid++; 
}

CODE_REFCOUNT(WriteReadEntry);

WriteReadEntry::WriteReadEntry(std::shared_ptr<WriteReadSetup> setup,
                               WebSocketsServerBase *master,
                               const PrioritySpec &ps, bool extended) :
  ChannelWatcher(setup->r_channelname),
  INIT_REFCOUNT_COMMA autostart_cb(this, &WriteReadEntry::tokenValid),
  do_valid(master->getId(), "channel valid", &autostart_cb, ps),
  marker(master->getMarker()),
  state(UnConnected),
  w_token(),
  r_token(),
  identification("not initialized"),
  w_channelname(setup->w_channelname),
  r_channelname(setup->r_channelname),
  w_dataclass(),
  r_dataclass(),
  label(boost::lexical_cast<std::string>(setup->getNextId())),
  master(master),
  active(true),
  bulk(setup->bulk),
  diffpack(setup->diffpack),
  extended(extended),
  cb(this, &WriteReadEntry::passData),
  do_calc(master->getId(), "read for server", &cb, ps)
{
  do_valid.switchOn();
  DEB("New writereadEntry " << label);
}

WriteReadEntry::~WriteReadEntry() { DEB("Deleting WriteReadEntry " << label); }

void WriteReadEntry::setConnection(connection_t connection)
{
  DEB("WriteReadEntry set connection");
  assert(!this->connection.get() && !this->sconnection.get());
  this->connection = connection;
  state = Connected;
}

void WriteReadEntry::setConnection(sconnection_t connection)
{
  DEB("WriteReadEntry set sconnection");
  assert(!this->connection.get() && !this->sconnection.get());
  this->sconnection = connection;
  state = Connected;
}

void WriteReadEntry::complete(const std::string &w_dataclass, const std::string& addtolabel)
{
  this->w_dataclass = w_dataclass;
  DEB("WriteReadEntry completing with write dataclass" << w_dataclass);
  assert(state == Connected);
  identification = w_channelname + std::string(" type:") + w_dataclass +
                   std::string(" label:\"") + label + std::string("\" <-> ") +
                   r_channelname;

  if (addtolabel.size()) {
    label = label + std::string(":") + addtolabel;
  }

  w_token.reset(new ChannelWriteToken(
    master->getId(), NameSet(w_channelname), w_dataclass, label,
    Channel::Events, Channel::OneOrMoreEntries,
    diffpack ? Channel::MixedPacking : Channel::OnlyFullPacking,
    bulk ? Channel::Bulk : Channel::Regular, &do_valid));
  state = ValidatingTokens;
}

void WriteReadEntry::close(const char* reason, int status)
{
  if (connection) {
    connection->send_close(status, reason);
    connection.reset();
  }
  if (sconnection) {
    sconnection->send_close(status, reason);
    sconnection.reset();
  }
  w_token.reset();
  r_token.reset();
}

bool WriteReadEntry::checkToken() { return state == Linked; }

void WriteReadEntry::entryAdded(const ChannelEntryInfo &i)
{
  DEB("WriteReadEntry::entryAdded " << i);
  if (state == ValidatingTokens && i.entry_label == label) {
    if (r_token) {
      /* DUECA websock.
      
         Attempt to connect a write-read entry to a second channel entry with the given 
         label.
        */
      W_XTR("WriteReadEntry already connected on label " << label);
    }
    else {
    r_dataclass = i.data_class;
    r_token.reset(new ChannelReadToken(
      master->getId(), NameSet(r_channelname), r_dataclass, i.entry_id,
      i.time_aspect, i.arity, Channel::ReadAllData, 0.0, &autostart_cb));
    }
  }
}

const GlobalId &WriteReadEntry::getId() const { return master->getId(); }

void WriteReadEntry::tokenValid(const TimeSpec &ts)
{
  DEB("WriteReadEntry::tokenValid " << w_token->isValid()
                                    << (r_token != NULL && r_token->isValid()));
  if (w_token->isValid() && r_token && r_token->isValid() &&
      state == ValidatingTokens) {

    std::stringstream buf;
    master->codeEntryInfo(buf, w_dataclass, w_token->getEntryId(), r_dataclass,
                          r_token->getEntryId());
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
  DEB("WriteReadEntry::passData " << ts);
  DCOReader r(r_dataclass.c_str(), *r_token, ts);
  std::stringstream buf;
  master->codeData(buf, r);
  sendOne(buf.str(), "channel data");
}

void WriteReadEntry::sendOne(const std::string &data, const char *desc)
{
  DEB("WriteReadEntry::sendOne " << data);
  if (connection) {
    connection->send(
      data,
      [this, desc](const SimpleWeb::error_code &ec) {
        if (ec) {
        /* DUECA websockets.

              Error in a send action for a "write-and-read" URL, will
              remove the connection from the list of clients. */
          W_XTR("Error sending " << desc << ", " << ec.message()
                                 << " removing connenction form "
                                 << this->identification);
        }
      },
      marker);
  }
  else {
    sconnection->send(
      data,
      [this, desc](const SimpleWeb::error_code &ec) {
        if (ec) {
        /* DUECA websockets.

              Error in a send action for a "write-and-read" URL, will
              remove the connection from the list of clients. */
          W_XTR("Error sending " << desc << ", " << ec.message()
                                 << " removing connenction form "
                                 << this->identification);
        }
      },
      marker);
  }
}

void WriteReadEntry::entryRemoved(const ChannelEntryInfo &i)
{
  if (i.entry_label == label) {
    DEB("WriteReadEntry::entryRemoved " << i);
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
