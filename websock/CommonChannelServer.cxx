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

#define CommonChannelServer_cxx
#include "CommonChannelServer.hxx"
#include <boost/lexical_cast.hpp>
#include <debug.h>
#define NO_TYPE_CREATION
#define DO_INSTANTIATE
#include <dueca.h>
#include <dueca/DCOtoJSON.hxx>
#include <dueca/JSONtoDCO.hxx>
#include <dueca/CommObjectWriter.hxx>
#include <dueca/DataClassRegistry.hxx>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>
#include <dueca/CommObjectReaderWriter.hxx>
#include "WebSocketsServer.hxx"

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;
WEBSOCK_NS_START;


namespace json = rapidjson;
typedef json::GenericDocument<json::UTF8<> > JDocument;

const char* presetmismatch::what() const throw()
{ return "websocket preset does not match client data"; }

const char* connectionparseerror::what() const throw()
{ return "JSON parse error at connection defining message"; }

const char* dataparseerror::what() const throw()
{ return "JSON parse error at connection defining message"; }

SingleEntryRead::SingleEntryRead
(const std::string& channelname, const std::string& datatype,
 entryid_type eid, const GlobalId& master) :
  r_token(master, NameSet(channelname), datatype, eid,
          Channel::AnyTimeAspect, Channel::OneOrMoreEntries,
          Channel::JumpToMatchTime),
  datatype(datatype)
{
  r_token.isValid();
}

SingleEntryRead::~SingleEntryRead()
{ }

SingleEntryFollow::SingleEntryFollow(const std::string& channelname,
                                     const std::string& datatype,
                                     entryid_type eid,
                                     const GlobalId& master,
                                     const PrioritySpec& ps,
                                     const DataTimeSpec& ts,
                                     bool extended,
                                     bool autostart) :
  ConnectionList(channelname + std::string(" (entry ") +
                 boost::lexical_cast<std::string>(eid) + std::string(")")),
  autostart_cb(this, &SingleEntryFollow::tokenValid),
  r_token(master, NameSet(channelname), datatype, eid,
          Channel::AnyTimeAspect, Channel::OneOrMoreEntries,
          Channel::ReadAllData, 0.0, autostart ? &autostart_cb : NULL),
  cb(this, &SingleEntryFollow::passData),
  do_calc(master, "read for server", &cb, ps),
  datatype(datatype),
  inactive(true),
  host_id(master),
  extended(extended),
  firstwrite(true)
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

SingleEntryFollow::~SingleEntryFollow()
{
  do_calc.clearTriggers();
}

void SingleEntryFollow::disconnect()
{
  do_calc.clearTriggers();
}

void SingleEntryFollow::tokenValid(const TimeSpec &ts)
{
  /** The callback for this is only connected when autostart was active */
  if (inactive) {
    DEB("Starting follow activity " << identification << " at " << ts);
    do_calc.switchOn(ts);
    inactive = false;
  }
}

ConnectionList::ConnectionList(const std::string& ident) :
  flock(ident.c_str(), false),
  identification(ident)
{ }

ConnectionList::~ConnectionList()
{ }

void ConnectionList::addConnection
(std::shared_ptr<WsServer::Connection>& c) {
  // ScopeLock l(flock);
  connections.push_back(c);
}

void ConnectionList::addConnection
(std::shared_ptr<WssServer::Connection>& c) {
  // ScopeLock l(flock);
  sconnections.push_back(c);
}

bool ConnectionList::removeConnection
(const std::shared_ptr<WsServer::Connection>& c) {
  // ScopeLock l(flock);
  auto toremove = std::find
    (connections.begin(), connections.end(), c);
  if (toremove != connections.end()) {
    connections.erase(toremove);
    return true;
  }
  return false;
}

bool ConnectionList::removeConnection
(const std::shared_ptr<WssServer::Connection>& c) {
  // ScopeLock l(flock);
  auto toremove = std::find
    (sconnections.begin(), sconnections.end(), c);
  if (toremove != sconnections.end()) {
    sconnections.erase(toremove);
    return true;
  }
  return false;
}

void ConnectionList::sendAll(const std::string& data,
                             const char* desc)
{
  // send to all existing connections
  for (auto &cn : connections) {
    sendOne(data, desc, cn);
  }
  for (auto &cn : sconnections) {
    sendOne(data, desc, cn);
  }
}

void ConnectionList::sendOne
(const std::string& data, const char* desc,
 const std::shared_ptr<WssServer::Connection>& cn) {
  cn->send
    (data,
     [cn, this, desc](const SimpleWeb::error_code &ec) {
      if (ec) {
        /* DUECA websockets.

           Error in a send action, will remove the connection from the
           list of clients. */
        W_XTR("Error sending " << desc << ", " << ec.message() <<
              " removing connenction form " << this->identification);
        this->removeConnection(cn);
      }
    });
}

void ConnectionList::sendOne
(const std::string& data, const char* desc,
 const std::shared_ptr<WsServer::Connection>& cn) {
  cn->send
    (data,
     [cn, this, desc](const SimpleWeb::error_code &ec) {
      if (ec) {
        /* DUECA websockets.

           Error in a send action, will remove the connection from the
           list of clients. */
        W_XTR("Error sending " << desc << ", " << ec.message() <<
              " removing connenction form " << this->identification);
        this->removeConnection(cn);
      }
    });
}

void SingleEntryFollow::passData(const TimeSpec& ts)
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
  rapidjson::StringBuffer doc;
  rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
  DataTimeSpec dtd = r.timeSpec();
  writer.StartObject();
  writer.Key("tick");
  writer.Uint(dtd.getValidityStart());
  writer.Key("data");
  if (extended) DCOtoJSONcompact(writer, r);
  else DCOtoJSONstrict(writer, r);
  writer.EndObject();

  DEB3("SingleEntryFollow::passData " << doc.GetString());
  sendAll(doc.GetString(), "channel data");
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

bool SingleEntryFollow::start(const TimeSpec& ts)
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

bool SingleEntryFollow::stop(const TimeSpec& ts)
{
  if (inactive) return false;
  do_calc.switchOff(ts);
  inactive = true;
  firstwrite = true;
  return true;
}

void writeTypeInfo(json::Writer<json::StringBuffer>& writer,
                   const std::string& dataclass)
{
  CommObjectReaderWriter rw(dataclass.c_str());
  writer.StartArray();
  for (size_t ii = 0; ii < rw.getNumMembers(); ii++ ) {
    writer.StartObject();
    writer.Key("name");
    writer.String(rw.getMemberName(ii));
    writer.Key("type");
    writer.String(rw.getMemberClass(ii));
    if (DataClassRegistry::single().isRegistered(rw.getMemberClass(ii))) {
      writer.Key("typeinfo");
      writeTypeInfo(writer, rw.getMemberClass(ii));
    }
    switch(rw.getMemberArity(ii)) {
    case Single:
      break;
    case FixedIterable:
      writer.Key("size");
      writer.Int(rw.getMemberSize(ii));
    case Iterable:
      writer.Key("array");
      writer.Bool(true);
      break;
    case Mapped:
      writer.Key("map");
      writer.Bool(true);
      writer.Key("keytype");
      writer.String(rw.getMemberKeyClass(ii));
    }
    writer.EndObject();
  }
  writer.EndArray();
}

ChannelMonitor::ChannelMonitor
(const std::string& channelname, const DataTimeSpec &ts) :
  ChannelWatcher(channelname),
  ConnectionList(channelname),
  channelname(channelname),
  time_spec(ts)
{ }

ChannelMonitor::~ChannelMonitor()
{ }

static void writeAdded(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                       unsigned entryid, const std::string& data_class)
{
  writer.StartObject();
  writer.Key("dataclass");
  writer.String(data_class.c_str());
  writer.Key("entry");
  writer.Int(entryid);
  writer.Key("typeinfo");
  writeTypeInfo(writer, data_class);
  writer.EndObject();
}

void ChannelMonitor::entryAdded(const ChannelEntryInfo &info)
{
  // ScopeLock l(flock);

  if (info.entry_id >= entrydataclass.size()) {
    entrydataclass.resize(info.entry_id+1);
  }
  assert(entrydataclass[info.entry_id].size() == 0);
  entrydataclass[info.entry_id] = info.data_class;

  // manually construct a small JSON message
  rapidjson::StringBuffer doc;
  rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
  writeAdded(writer, info.entry_id, info.data_class);
  DEB("entryAdded " << doc.GetString());
  sendAll(doc.GetString(), "entry addition");
}

void ChannelMonitor::entryRemoved(const ChannelEntryInfo &info)
{
  // ScopeLock l(flock);

  assert(info.entry_id < entrydataclass.size() &&
         entrydataclass[info.entry_id].size() > 0);
  entrydataclass[info.entry_id] = std::string();

  // manually construct a small JSON message
  std::stringstream msg;
  msg << "{\"dataclass\":\"\",\"entry\":" << info.entry_id << "}";
  DEB("entryRemoved " << msg.str());
  sendAll(msg.str(), "entry removal");
}

void ChannelMonitor::addConnection(std::shared_ptr<WsServer::Connection>& c)
{
  ConnectionList::addConnection(c);
  // ScopeLock l(flock);
  for (size_t ii = 0; ii < entrydataclass.size(); ii++) {
    if (entrydataclass[ii].size()) {
      rapidjson::StringBuffer doc;
      rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
      writeAdded(writer, ii, entrydataclass[ii]);
      sendOne(doc.GetString(), "entry catch up", c);
    }
  }
}

void ChannelMonitor::addConnection(std::shared_ptr<WssServer::Connection>& c)
{
  ConnectionList::addConnection(c);
  // ScopeLock l(flock);
  for (size_t ii = 0; ii < entrydataclass.size(); ii++) {
    if (entrydataclass[ii].size()) {
      rapidjson::StringBuffer doc;
      rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
      writeAdded(writer, ii, entrydataclass[ii]);
      sendOne(doc.GetString(), "entry catch up", c);
    }
  }
}

const std::string& ChannelMonitor::findEntry(unsigned entryid)
{
  // ScopeLock l(flock);
  static std::string empty;
  if (entryid >= entrydataclass.size()) {
    return empty;
  }
  return entrydataclass[entryid];
}

WriteableSetup::WriteableSetup(const std::string& channelname,
                               const std::string& dataclass) :
  channelname(channelname),
  dataclass(dataclass)
{ }

CODE_REFCOUNT(WriteEntry);
#if 0
void intrusive_ptr_add_ref(const WriteEntry*)
{ t->intrusive_refcount++; }
void intrusive_ptr_release(const WriteEntry*)
{ if (--(t->intrusive_refcount) == 0) { delete t; } }
#endif

WriteEntry::WriteEntry(const std::string& channelname,
                       const std::string& datatype,
                       bool bulk, bool diffpack,
                       WriteEntry::WEState initstate) :
  INIT_REFCOUNT_COMMA
  state(initstate),
  w_token(),
  identification("not initialized"),
  channelname(channelname),
  datatype(datatype),
  ctiming(false),
  active(true),
  stream(false),
  bulk(bulk),
  diffpack(diffpack)
{ }

WriteEntry::~WriteEntry()
{
  //
}

void WriteEntry::complete(const std::string& message1,
                          const GlobalId& master)
{
  JDocument doc;
  json::ParseResult res = doc.Parse(message1.c_str());
  if (!res) {
    /* DUECA websockets.

       Error in parsing the initial JSON data for a "write" URL. Check
       your channel definition and the external client program.
    */
    W_XTR("JSON parse error " << rapidjson::GetParseError_En(res.Code()) <<
          " at " << res.Offset());
    throw connectionparseerror();
  }

  std::string label;
  {
    auto iv = doc.FindMember("label");
    if (iv == doc.MemberEnd() || !iv->value.IsString()) {
      /* DUECA websockets.

         Error the initial JSON data for a "write" URL. The entry
         "label" should be specified in this initial message.
      */
      W_XTR("JSON parse error no label specified");
      throw connectionparseerror();
    }
    label = iv->value.GetString();
  }

  ctiming = false;
  {
    auto im = doc.FindMember("ctiming");
    if (im != doc.MemberEnd()) {
      if (!im->value.IsBool()) {
      /* DUECA websockets.

         Error the initial JSON data for a "write" URL. A "ctiming"
         member cannot be interpreted as a boolean.
      */
        W_XTR("JSON parse error \"ctiming\" needs to be bool");
        throw connectionparseerror();
      }
      ctiming = im->value.GetBool();
    }
  }

  stream = false;
  {
    auto im = doc.FindMember("event");
    if (im != doc.MemberEnd()) {
      if (!im->value.IsBool()) {
        /* DUECA websockets.

           Error the initial JSON data for a "write" URL. An "event"
           member cannot be interpreted as a boolean.
        */
        W_XTR("JSON parse error \"event\" needs to be bool");
        throw connectionparseerror();
      }
      stream = !im->value.GetBool();
    }
  }

  bulk = false;
  {
    auto im = doc.FindMember("bulk");
    if (im != doc.MemberEnd()) {
      if (!im->value.IsBool()) {
        /* DUECA websockets.

           Error the initial JSON data for a "write" URL. A "bulk"
           member cannot be interpreted as a boolean.
        */
        W_XTR("JSON parse error \"bulk\" needs to be bool");
        throw connectionparseerror();
      }
      bulk = !im->value.GetBool();
    }
  }

  diffpack = false;
  {
    auto im = doc.FindMember("diffpack");
    if (im != doc.MemberEnd()) {
      if (!im->value.IsBool()) {
        /* DUECA websockets.

           Error the initial JSON data for a "write" URL. A "diffpack"
           member cannot be interpreted as a boolean.
        */
        W_XTR("JSON parse error \"diffpack\" needs to be bool");
        throw connectionparseerror();
      }
      diffpack = !im->value.GetBool();
    }
  }

  auto dc = doc.FindMember("dataclass");
  if (dc != doc.MemberEnd() && datatype.size() != 0 &&
      datatype != dc->value.GetString()) {
    /* DUECA websockets.

       Error the initial JSON data for a "write" URL. The name of the
       data class in \"dataclass\" does not match the configured
       name. Check your configuration or the external client program.
    */
    W_XTR("JSON parse error mis-matched dataclass " << dc->value.GetString() <<
          " needed " << datatype);
    throw connectionparseerror();
  }
  if (datatype.size() == 0) {
    datatype = dc->value.GetString();
  }

  identification = channelname + std::string(" type:") + datatype +
    std::string(" label:\"") + label + std::string("\"");
  w_token.reset
    (new ChannelWriteToken(master, NameSet(channelname), datatype, label,
                           stream ? Channel::Continuous : Channel::Events,
                           Channel::OneOrMoreEntries,
                           diffpack ? Channel::MixedPacking :
                           Channel::OnlyFullPacking,
                           bulk ? Channel::Bulk : Channel:: Regular));
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

void WriteEntry::writeFromJSON(const std::string& json)
{
  JDocument doc;
  json::ParseResult res = doc.Parse(json.c_str());
  if (!res) {
    /* DUECA websockets.

       Error in parsing the recurring JSON data for a "write" URL.
    */
    W_XTR("JSON parse error " << rapidjson::GetParseError_En(res.Code()) <<
          " at " << res.Offset());
    throw dataparseerror();
  }

  DataTimeSpec ts;
  if (ctiming) {
    if (stream) {
      auto it = doc.FindMember("tick");
      if (it == doc.MemberEnd() || !it->value.IsArray() ||
          it->value.Size() != 2 || !it->value[0].IsInt()) {
        /* DUECA websockets.

           For writing data as stream (dueca::Channel::Continuous),
           the client needs to supply a "tick" member in the JSON with
           two integer values for the time tick. Check/correct the
           configuration or your external client program.
        */
        W_XTR("JSON data needs 2 elt tick");
        throw dataparseerror();
      }

      // need two elements in tick, tick needs to be array
      ts.validity_start = it->value[0].GetInt();
      ts.validity_end = it->value[1].GetInt();
    }
    else {
      auto it = doc.FindMember("tick");
      if (it == doc.MemberEnd() || !it->value.IsInt()) {
        /* DUECA websockets.

           For writing data as stream (dueca::Channel::Continuous),
           the client needs to supply a "tick" member in the JSON with
           one integer value for the time tick. Check/correct the
           configuration or your external client program.
        */
        W_XTR("JSON data needs 1 elt tick");
        throw dataparseerror();
      }
      // tick needs to be a single value
      ts.validity_start = ts.validity_end = it->value.GetInt();
    }
  }
  else {
    // follow current time
    ts.validity_start = ts.validity_end = SimTime::now();
  }

  DCOWriter wr(*w_token, ts);
  try {
    JSONtoDCO(doc["data"], wr);
  }
  catch (const dueca::ConversionNotDefined& e) {
    /* DUECA websockets.

       Failed to decode an object of the given dataclass from the JSON
       string received. Check the correspondence between your
       (external) program and the DUECA object definitions. */
    W_XTR("Websockets, cannot extract '" << w_token->getDataClassName() <<
          "' from 'data' in '" << json << "'")
    wr.failed();
  }
}

PresetWriteEntry::PresetWriteEntry(const std::string& channelname,
                                   const std::string& datatype,
                                   const std::string& label,
                                   const GlobalId& master,
                                   bool ctiming, bool stream,
                                   bool bulk, bool diffpack) :
  WriteEntry(channelname, datatype, bulk, diffpack, UnConnected)
{
  this->ctiming = ctiming;
  this->stream = stream;
  this->identification = channelname + std::string(" type:") + datatype +
    std::string(" label:\"") + label + std::string("\"");
  w_token.reset
    (new ChannelWriteToken(master, NameSet(channelname), datatype, label,
                           stream ? Channel::Continuous : Channel::Events,
                           Channel::OneOrMoreEntries,
                           diffpack ? Channel::MixedPacking :
                           Channel::OnlyFullPacking,
                           bulk ? Channel::Bulk : Channel:: Regular));
  checkToken();
}

PresetWriteEntry::~PresetWriteEntry()
{
  //
}

void PresetWriteEntry::complete(const std::string& message1,
                                const GlobalId& master)
{
  JDocument doc;
  json::ParseResult res = doc.Parse(message1.c_str());
  if (!res) {
    /* DUECA websockets.

       Error in parsing the initial JSON data for a "write" URL with
       preset. Check your channel definition and the external client
       program.
    */
    W_XTR("JSON parse error " << rapidjson::GetParseError_En(res.Code()) <<
          " at " << res.Offset());
    throw connectionparseerror();
  }

  bool _ctiming = false;
  {
    auto im = doc.FindMember("ctiming");
    if (im != doc.MemberEnd()) {
      if (!im->value.IsBool()) {
        /* DUECA websockets.

           Error the initial JSON data for a "write" URL with
           preset. A "ctiming" member cannot be interpreted as a
           boolean.
        */
        W_XTR("JSON parse error \"ctiming\" needs to be bool");
        throw connectionparseerror();
      }
      _ctiming = im->value.GetBool();
    }
  }

  bool _stream = false;
  {
    auto im = doc.FindMember("event");
    if (im != doc.MemberEnd()) {
      if (!im->value.IsBool()) {
        /* DUECA websockets.

           Error the initial JSON data for a "write" URL with
           preset. An "event" member cannot be interpreted as a
           boolean.
        */
        W_XTR("JSON parse error \"event\" needs to be bool");
        throw connectionparseerror();
      }
      _stream = !im->value.GetBool();
    }
  }

  if (_ctiming != this->ctiming || _stream != this->stream) {
    throw(presetmismatch ());
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

void* PresetWriteEntry::disConnect()
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


NameEntryId::NameEntryId(const std::string& name, unsigned id) :
name(name), id(id)
{ }

bool NameEntryId::operator < (const NameEntryId& other) const
{
  if (this->name < other.name) return true;
  if (this->name > other.name) return false;
  if (this->id < other.id) return true;
  return false;
}

NameEntryTokenId::NameEntryTokenId
(const std::string& name, unsigned id, const std::string token) :
  name(name), id(id), token(token)
{ }

bool NameEntryTokenId::operator <
(const NameEntryTokenId& other) const
{
  if (this->name < other.name) return true;
  if (this->name > other.name) return false;
  if (this->id < other.id) return true;
  if (this->id > other.id) return true;
  if (this->token < other.token) return true;
  return false;
}

NameTokenId::NameTokenId
(const std::string& name, const std::string token) :
  name(name), token(token)
{ }

bool NameTokenId::operator <
(const NameTokenId& other) const
{
  if (this->name < other.name) return true;
  if (this->name > other.name) return false;
  if (this->token < other.token) return true;
  return false;
}

WriteReadSetup::WriteReadSetup(const std::string& wchannelname,
                               const std::string& rchannelname) :
  cnt_clients(0U),
  w_channelname(wchannelname),
  r_channelname(rchannelname),
  bulk(false),
  diffpack(false)
{
  //
}

unsigned WriteReadSetup::getNextId()
{
  return cnt_clients++;
}

CODE_REFCOUNT(WriteReadEntry);

WriteReadEntry::WriteReadEntry(std::shared_ptr<WriteReadSetup> setup,
                               WebSocketsServer *master,
                               const PrioritySpec& ps,
                               bool extended,
                               WriteReadEntry::WRState initstate) :
  ChannelWatcher(setup->r_channelname),
  INIT_REFCOUNT_COMMA
  autostart_cb(this, &WriteReadEntry::tokenValid),
  state(initstate),
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
  //
}

WriteReadEntry::~WriteReadEntry()
{
  //
}

void WriteReadEntry::setConnection(connection_t connection)
{
  this->connection = connection;
}

void WriteReadEntry::setConnection(sconnection_t connection)
{
  this->sconnection = connection;
}

void WriteReadEntry::complete(const std::string& message1)
{
  JDocument doc;
  json::ParseResult res = doc.Parse(message1.c_str());
  if (!res) {
    /* DUECA websockets.

       Error in parsing the initial JSON data for a "write-and-read"
       URL. Check your channel definition and the external client
       program.
    */
    W_XTR("JSON parse error " << rapidjson::GetParseError_En(res.Code()) <<
          " at " << res.Offset());
    throw connectionparseerror();
  }

  auto dc = doc.FindMember("dataclass");
  if (dc == doc.MemberEnd()) {
    /* DUECA websockets.

       Error the initial JSON data for a "write" URL. The entry
       "dataclass" should be specified in this initial message.
      */
    W_XTR("Read-Write entry needs write dataclass");
    throw connectionparseerror();
  }
  w_dataclass = dc->value.GetString();

  identification = w_channelname + std::string(" type:") + w_dataclass +
    std::string(" label:\"") + label + std::string("\" <-> ") + r_channelname;

  w_token.reset
    (new ChannelWriteToken(master->getId(), NameSet(w_channelname),
                           w_dataclass, label,
                           Channel::Events, Channel::OneOrMoreEntries,
                           diffpack ? Channel::MixedPacking :
                           Channel::OnlyFullPacking,
                           bulk ? Channel::Bulk : Channel:: Regular));
  state = ValidatingWrite;
  checkToken();
}

bool WriteReadEntry::checkToken()
{
  return (w_token->isValid() && r_token && r_token->isValid());
}

void WriteReadEntry::entryAdded(const ChannelEntryInfo& i)
{
  DEB("WriteReadEntry::entryAdded " << i);
  if (i.entry_label == label) {
    r_dataclass = i.data_class;
    assert(!r_token);
    r_token.reset(new ChannelReadToken
                  (master->getId(), NameSet(r_channelname), r_dataclass,
                   i.entry_id, i.time_aspect, i.arity,
                   Channel::ReadAllData, 0.0, &autostart_cb));
    if (checkToken()) {
      state = Linked;
    }
    do_calc.setTrigger(*r_token);
    do_calc.switchOn();
  }
}

const GlobalId& WriteReadEntry::getId() const
{ return master->getId(); }

void WriteReadEntry::tokenValid(const TimeSpec& ts)
{
  rapidjson::StringBuffer doc;
  rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
  writer.StartObject();

  // write side information, dataclass and typeinfo
  writer.Key("write");
  writer.StartObject();
  writer.Key("dataclass");
  writer.String(w_dataclass.c_str());
  writer.Key("entry");
  writer.Int(w_token->getEntryId());
  writer.Key("typeinfo");
  writeTypeInfo(writer, w_dataclass);
  writer.EndObject();

  // read side information, dataclass and typeinfo
  writer.Key("read");
  writer.StartObject();
  writer.Key("dataclass");
  writer.String(r_dataclass.c_str());
  writer.Key("entry");
  writer.Int(r_token->getEntryId());
  writer.Key("typeinfo");
  writeTypeInfo(writer, r_dataclass);
  writer.EndObject();

  // close off the JSON
  writer.EndObject();

  // both tokens valid now, return information on entries
  sendOne(doc.GetString(), "WriterReader info");
}

void WriteReadEntry::passData(const TimeSpec& ts)
{
  DCOReader r(r_dataclass.c_str(), *r_token, ts);
  rapidjson::StringBuffer doc;
  rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
  DataTimeSpec dtd = r.timeSpec();
  writer.StartObject();
  writer.Key("tick");
  writer.Uint(dtd.getValidityStart());
  writer.Key("data");
  if (extended) DCOtoJSONcompact(writer, r);
  else DCOtoJSONstrict(writer, r);
  writer.EndObject();

  DEB2("WriteReadEntry::passData " << doc.GetString());
  sendOne(doc.GetString(), "channel data");
}

void WriteReadEntry::sendOne(const std::string& data,
                             const char* desc)
{
  if (connection) {
    connection->send
      (data,
       [this, desc](const SimpleWeb::error_code &ec) {
         if (ec) {
           /* DUECA websockets.

              Error in a send action for a "write-and-read" URL, will
              remove the connection from the list of clients. */
           W_XTR("Error sending " << desc <<", " << ec.message() <<
                 " removing connenction form " << this->identification);
         }
       });
  }
  else {
    sconnection->send
      (data,
       [this, desc](const SimpleWeb::error_code &ec) {
         if (ec) {
           /* DUECA websockets.

              Error in a send action for a "write-and-read" URL, will
              remove the connection from the list of clients. */
           W_XTR("Error sending " << desc << ", " << ec.message() <<
                 " removing connenction form " << this->identification);
         }
       });
  }
}

void WriteReadEntry::entryRemoved(const ChannelEntryInfo& i)
{
  if (i.entry_label == label) {
    state = Connected;
    r_token.reset();
  }
}

void WriteReadEntry::writeFromJSON(const std::string& json)
{
  JDocument doc;
  json::ParseResult res = doc.Parse(json.c_str());
  if (!res) {
    /* DUECA websockets.

       Error in parsing the recurring JSON data for a "write-and-read"
       URL.
    */
    W_XTR("JSON parse error " << rapidjson::GetParseError_En(res.Code()) <<
          " at " << res.Offset());
    throw dataparseerror();
  }
  auto data = doc.FindMember("data");
  if (data == doc.MemberEnd()) {
    /* DUECA websockets.

       Error in interpreting the recurring JSON data for a
       "write-and-read" URL, it needs a member "data" with the
       to-be-written data.
    */
    W_XTR("JSON message has no member data");
    throw dataparseerror();
  }

  DCOWriter wr(*w_token, DataTimeSpec::now());
  try {
    JSONtoDCO(data->value, wr);
  }
  catch (const dueca::ConversionNotDefined& e) {
    /* DUECA websockets.

       Failed to decode a DCO object from the received JSON
       string. Check the correspondence between the DCO object and the
       external program. */
    W_XTR("Websockets, cannot decode '" << w_token->getDataClassName() <<
          "' from 'data' in '" << json << "'");
    wr.failed();
  }
}


DUECA_NS_END;
WEBSOCK_NS_END;
