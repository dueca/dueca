/* ------------------------------------------------------------------   */
/*      item            : WebSocketsServer.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Tue Nov 27 13:58:45 2018
        category        : body file
        description     :
        changes         : Tue Nov 27 13:58:45 2018 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "WebsockExceptions.hxx"
#include "dueca_ns.h"
#define WebSocketsServer_cxx

// include the definition of the module class
#include "WebSocketsServer.hxx"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

// include the debug writing header, by default, write warning and
// error messages
#define I_XTR
#define W_XTR
#define E_XTR
#include <debug.h>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>

#define DEBPRINTLEVEL 0
#include <debprint.h>

DUECA_NS_START;
WEBSOCK_NS_START;


// Parameters to be inserted
const ParameterTable *WebSocketsServerBase::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, vector<int>>(&_ThisModule_::checkTiming),
      check_timing_description },

    { "current",
      new MemberCall<_ThisModule_, std::vector<std::string>>(
        &_ThisModule_::setCurrentData),
      "Set up a URL for simply reading the latest data from a single entry\n"
      "in a channel. Specify the URL name, channel name, dataclass, and\n"
      "optionally (in a string) the entry number. Data is returned whenever\n"
      "requested by (an empty) message\n"
      "This results in a URL /current/name?entry=...\n"
      "The returned objects objects have members \"tick\" for timing and \n"
      "\"data\" with the DCO object encoded in JSON or msgpack" },

    { "read-timing",
      new VarProbe<_ThisModule_, TimeSpec>(&_ThisModule_::time_spec),
      "Set the read timing for read data. TimeSpec(0,0) disables this\n"
      "again, and will read at the rate the data was sent to the channel." },

    { "extended", new VarProbe<_ThisModule_, bool>(&_ThisModule_::extended),
      "Use the extended (sloppy) JSON, with NaN, Infinite and -Infinite.\n"
      "Ensure any external clients understand this format." },

    { "read",
      new MemberCall<_ThisModule_, std::vector<std::string>>(
        &_ThisModule_::setFollowData),
      "Set up a URL for tracking all data from a single entry in a channel.\n"
      "Specify the URL name, channel name, dataclass, and optionally (in a\n"
      "string) the entry number. Data is pushed to the client, if specified\n"
      "before, the rate given by \"read-timing\" is used for reading"
      "This results in a URL /read/name?entry=...\n" },

    { "info",
      new MemberCall<_ThisModule_, std::vector<std::string>>(
        &_ThisModule_::setChannelInfo),
      "Set up a URL for obtaining information about channel entries.\n"
      "Specify the URL name and channel name. Any discovered entries will\n"
      "be available in the /current or /read URL's"
      "Configuring this results in a URL /info/name\n"
      "Information will be provided upon completion of the websocket." },

    { "write",
      new MemberCall<_ThisModule_, std::vector<std::string>>(
        &_ThisModule_::setWriterSetup),
      "Set up URL for writing to a channel. Specify URL name, channel name\n"
      "and optionally the dataclass to be written. Any connects will first\n"
      "need to send a JSON object with information for the written entry:\n"
      "  - \"dataclass\" <string>, needed if not specified in the arguments\n"
      "  - \"label\" <string>, mandatory\n"
      "  - \"ctiming\" <bool>, optional for event, mandatory for stream,\n"
      "    indicates that writer will supply timing\n"
      "  - \"event\", <bool>, optional, default true, event writing\n"
      "  - \"stream\", <bool>, optional, indicate stream channel\n"
      "  - \"bulk\", <bool>, optional, send data in bulk priority\n"
      "  - \"diffpack\", <bool>, optional, use differential pack for sending\n"
      "For stream channels, timing information *must* be supplied. Subsequent\n"
      "messages carry \"tick\" (if timing supplied, single int for event,\n"
      "two int's for stream), and \"data\" for the data struct.\n"
      "This results in a URL /write/name, each connection creates an entry" },

    { "write-preset",
      new MemberCall<_ThisModule_, std::vector<std::string>>(
        &_ThisModule_::setPresetWriterSetup),
      "Set up URL for writing to a channel. Specify URL name, channel name,\n"
      "data class and a label for the channel entry. Optional keywords\n"
      "  - \"ctiming\" <bool>, optional for event, mandatory for stream,\n"
      "    indicates that writer will supply timing\n"
      "  - \"event\", <bool>, optional, default true, event writing\n"
      "  - \"stream\", <bool>, optional, indicate stream channel\n"
      "  - \"bulk\", <bool>, optional, send data in bulk priority\n"
      "  - \"diffpack\", <bool>, optional, use differential pack for sending\n"
      "The first JSON message carries the \"timing\", \"event\" / \"stream\"\n"
      "information, this must match the information specified here. This\n"
      "creates a single persistent channel entry, only one client can\n"
      "connect to this entry and write to it. After a disconnect, a new\n"
      "connection can be made. This results in a URL /write/name." },

    { "aggressive-reconnect",
      new VarProbe<_ThisModule_, bool>(&_ThisModule_::aggressive_reconnect),
      "Normally a preset entry is only reconnected after the connection with\n"
      "the previous client has been closed. Setting this variable to true\n"
      "makes reconnection more aggressive; old connections are closed and\n"
      "a new connection is always made." },

    { "immediate-start",
      new VarProbe<_ThisModule_, bool>(&_ThisModule_::immediate_start),
      "Start as soon as possible, i.e., when watched channels are valid\n"
      "This is normally only needed for specific debugging purposes." },

    { "write-and-read",
      new MemberCall<_ThisModule_, std::vector<std::string>>(
        &_ThisModule_::setWriteReadSetup),
      "Set up a URL connecting a reading and writing channel set. Specify\n"
      "URL name, channel name for writing, channel name for reading,\n"
      "optionally, add \"bulk\" for bulk sending mode and \"diffpack\" for\n"
      "differential packing.\n"
      "The communication is event-based, the first message from the client\n"
      "should have a JSON struct with information on the written dataclass\n"
      "( { \"dataclass\": ... } ). The first received message will have\n"
      "information on both dataclasses and channel connections:\n"
      "( { \"write\": { \"dataclass\": ..., \"entry\": ##, \"typeinfo\": "
      "{...}},\n"
      "(   \"read\":  { \"dataclass\": ..., \"entry\": ##, \"typeinfo\": "
      "{...}}\n"
      "The DUECA module servicing this connection will need to create "
      "matching\n"
      "entries to any write created by a connection, using the same label as\n"
      "generated by the socket server.\n"
      "This results in a URL /write-and-read/name, each connection creates a\n"
      "pair of entries, for completion you need a DUECA module monitoring the\n"
      "written channel and submitting an entry in the reading channel with a\n"
      "label matching the written entry" },

    { "port", new VarProbe<_ThisModule_, unsigned>(&_ThisModule_::port),
      "Server port to be used, default is 8001." },

    { "http-port",
      new VarProbe<_ThisModule_, unsigned>(&_ThisModule_::http_port),
      "If selected by setting a path, http server port, default is 8000." },

    { "document-root",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::document_root),
      "Location of static files to serve over http; if not supplied, no\n"
      "http server is created.\n" },

    { "certfiles",
      new MemberCall<_ThisModule_, std::vector<std::string>>(
        &_ThisModule_::setCertFiles),
      "Certificate files for SSL, specify a .crt and a .key file. If these\n"
      "are supplied, wss sockets will be used instead of ws" },

    { "add-mimetype",
      new MemberCall<_ThisModule_, std::vector<std::string>>(
        &_ThisModule_::addMimeType),
      "Add a mime type for an extension, specify the extension (with '.')\n"
      "and the mime type string" },

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
      "JSON/msgpack server providing access to selected channels and channel\n"
      "entries with websockets." }
  };

  return parameter_table;
}

// constructor
WebSocketsServerBase::WebSocketsServerBase(Entity *e, const char *part,
                                           const PrioritySpec &ps,
                                           const char *classname,
                                           unsigned char marker) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  marker(marker),
  server(), sserver(), http_server(), https_server(), server_crt(),
  server_key(), runcontext(new boost::asio::io_context), port(8001),
  http_port(8000), document_root(), aggressive_reconnect(false),
  immediate_start(false), auto_started(false),
  thelock("JSON ws(s) server", false), read_prio(ps), time_spec(0, 0),
  extended(false), readsingles(), autosingles(), singlereadsmapped(),
  followers(), monitors(), myclock(),
  cb1(this, &WebSocketsServerBase::doTransfer),
  do_transfer(getId(), "run websocket IO", &cb1, ps)
{
  // connect the triggers for simulation
  do_transfer.setTrigger(myclock);
}

#ifdef BOOST1_65
#define BOOST_POSTCALL runcontext->post
#define BOOST_POSTARG1
#else
#define BOOST_POSTCALL boost::asio::post
#define BOOST_POSTARG1 *runcontext,
#endif

// destructor
WebSocketsServerBase::~WebSocketsServerBase()
{
  if (immediate_start) {
    auto_started = false;
    TimeSpec now(SimTime::now());
    stopModule(now);
  }
}

// as an example, the setTimeSpec function
bool WebSocketsServerBase::setTimeSpec(const TimeSpec &ts)
{
  myclock.changePeriodAndOffset(ts);

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool WebSocketsServerBase::checkTiming(const vector<int> &i)
{
  if (i.size() == 3) {
    new TimingCheck(do_transfer, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_transfer, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

bool WebSocketsServerBase::setCurrentData(const std::vector<std::string> &def)
{
  if (def.size() < 3 || def.size() > 4 || def[0].size() == 0 ||
      def[1].size() == 0 || def[2].size() == 0) {
    /* DUECA websockets.

       Configuration for a "current" URL is not complete. Check your
       configuration file.
    */
    E_CNF("Need 3 or 4 valid arguments");
    return false;
  }
  unsigned entryid = 0;
  if (def.size() == 4) {
    try {
      entryid = boost::lexical_cast<unsigned int>(def[3]);
    }
    catch (const std::exception &e) {
      /* DUECA websockets.

         Failed in an attempt to read the 4-th argument to the
         configuration for a "current" URL as an entry number. Check
         your configuration file.
      */
      E_CNF("Cannot interpret entry number from \"" << def[3] << '"');
      return false;
    }
  }

  NameEntryId key(def[0], entryid);

  if (readsingles.find(key) != readsingles.end()) {
    /* DUECA websockets.

       The requested URL location for a "current" url has already been
       configured.
    */
    E_XTR("location \"/current/" << def[0] << "?entry=" << entryid
                                 << "\" already defined");
    return false;
  }

  try {
    std::shared_ptr<SingleEntryRead> nentry(
      new SingleEntryRead(def[1], def[2], entryid, this, this->read_prio));
    readsingles[key] = nentry;
  }
  catch (const std::exception &e) {
    /* DUECA websockets.

       Cannot open the channel entry for the requested "current" URL,
       check the set-up of your DUECA process, and whether the writing
       module for this channel has been successfully created.
    */
    E_XTR("Cannot create entry reader, exception " << e.what());
    return false;
  }
  return true;
}

bool WebSocketsServerBase::setFollowData(const std::vector<std::string> &def)
{
  if (def.size() < 3 || def.size() > 4 || def[0].size() == 0 ||
      def[1].size() == 0 || def[2].size() == 0) {
    /* DUECA websockets.

       Configuration for a "read" URL is not complete. Check your
       configuration file.
    */
    E_CNF("Need 3 or 4 valid arguments");
    return false;
  }

  unsigned entryid = 0;
  if (def.size() == 4) {
    try {
      // entry number
      entryid = boost::lexical_cast<unsigned int>(def[3]);
    }
    catch (const std::exception &e) {
      /* DUECA websockets.

         Failed in an attempt to read the 4-th argument to the
         configuration for a "read" URL as an entry number. Check
         your configuration file.
      */
      E_CNF("Cannot interpret entry number from \"" << def[3] << '"');
      return false;
    }
  }

  NameEntryId key(def[0], entryid);

  if (followers.count(key)) {
    /* DUECA websockets.

       The requested URL location for a "read" url has already been
       configured.
    */
    E_CNF("location \"/read/\"" << def[0] << "?entry=" << entryid
                                << "\" already defined");
    return false;
  }

  try {
    followread_t::mapped_type nentry(
      new followread_t::mapped_type::element_type(
        def[1], def[2], entryid, this, read_prio, time_spec));
    followers[key] = nentry;
  }
  catch (const std::exception &e) {
    /* DUECA websockets.

       Cannot open the channel entry for the requested "read" URL,
       check the set-up of your DUECA process, and whether the writing
       module for this channel has been successfully created.
    */
    E_CNF("Cannot create entry following reader, exception " << e.what());
    return false;
  }
  return true;
}

bool WebSocketsServerBase::setChannelInfo(const std::vector<std::string> &def)
{
  // check size of input arguments
  if (def.size() != 2 || def[0].size() == 0 || def[1].size() == 0) {
    /* DUECA websockets.

       Configuration for an "info" URL is not complete. Check your
       configuration file.
    */
    E_CNF("Need URL name and channel name");
    return false;
  }

  // ensure this URL is not yet known
  if (monitors.find(def[0]) != monitors.end()) {
    /* DUECA websockets.

       The requested URL location for an "info" url has already been
       configured.
    */
    E_CNF("location \"/info/" << def[0] << "\" already defined");
    return false;
  }

  try {
    std::shared_ptr<ChannelMonitor> nentry(
      new ChannelMonitor(this, def[1], time_spec));
    monitors[def[0]] = nentry;
  }
  catch (const std::exception &e) {
    /* DUECA websockets.

       Cannot open the information object for the requested "info"
       URL, check the set-up of your DUECA process, and whether the
       writing module for this channel has been successfully created.
    */
    E_CNF("Cannot create info follower, exception " << e.what());
    return false;
  }
  return true;
}

bool WebSocketsServerBase::setWriterSetup(const std::vector<std::string> &def)
{
  // check size of input arguments
  if (def.size() < 2 || def[0].size() == 0 || def[1].size() == 0) {
    /* DUECA websockets.

       Configuration for a "write" URL is not complete. Check your
       configuration file.
    */
    E_CNF("Need URL name, channel name as arguments");
    return false;
  }

  // ensure this URL is not yet known
  if (writersetup.count(def[0]) || presetwriters.count(def[0])) {
    /* DUECA websockets.

       The requested URL location for a "write" url has already been
       configured.
    */
    E_CNF("location \"/write/" << def[0] << "\" already defined");
    return false;
  }

  // a setup where multiple connections may be made, and each
  // connection dynamically creates a channel entry to write in
  try {
    std::shared_ptr<WriteableSetup> nentry(
      new WriteableSetup(def[1], (def.size() > 2) ? def[2] : std::string("")));
    writersetup[def[0]] = nentry;
  }
  catch (const std::exception &e) {
    /* DUECA websockets.

       Cannot open the write entry for the requested "write" URL,
       check the set-up of your DUECA process.
    */
    E_CNF("Cannot create writer setup, exception " << e.what());
    return false;
  }
  return true;
}

bool WebSocketsServerBase::setPresetWriterSetup(const std::vector<std::string> &def)
{
  // check size of input arguments
  if (def.size() < 4 || def[0].size() == 0 || def[1].size() == 0 ||
      def[2].size() == 0) {
    /* DUECA websockets.

       Configuration for a "write" URL with preset is not
       complete. Check your configuration file.
    */
    E_CNF("Need URL name, channel name, datatype and label as arguments");
    return false;
  }

  // ensure this URL is not yet known
  if (writersetup.count(def[0]) || presetwriters.count(def[0])) {
    /* DUECA websockets.

       The requested URL location for a "write" url has already been
       configured.
    */
    E_CNF("location \"/write/" << def[0] << "\" already defined");
    return false;
  }

  // detect flags
  bool ctiming = false;
  bool stream = false;
  bool bulk = false;
  bool diffpack = false;
  if (def.size() > 3) {

    for (unsigned idx = 4; idx < def.size(); idx++) {
      if (def[idx] == "ctiming") {
        ctiming = true;
      }
      else if (def[idx] == "stream") {
        stream = true;
      }
      else if (def[idx] == "event") {
        stream = false;
      }
      else if (def[idx] == "bulk") {
        bulk = true;
      }
      else if (def[idx] == "diffpack") {
        diffpack = true;
      }
      else {
        /* DUECA websockets.

           Wrong keywords found in the setup of a writer with
           preset. Check your configuration files. */
        E_CNF("Can only use keywords \"event\", \"stream\" or \"ctiming\"");
        return false;
      }
    }
    ctiming = ctiming || stream;
  }

  try {
    boost::intrusive_ptr<PresetWriteEntry> nentry(new PresetWriteEntry(
      def[1], def[2], def[3], this, this->read_prio, ctiming, stream, bulk, diffpack));
    presetwriters[def[0]] = nentry;
  }
  catch (const std::exception &e) {
    /* DUECA websockets.

       Cannot open the write entry for the requested "write" URL,
       check the set-up of your DUECA process.
    */
    E_CNF("Cannot create preset writer, exception " << e.what());
    return false;
  }
  return true;
}

bool WebSocketsServerBase::setWriteReadSetup(const std::vector<std::string> &def)
{
  if (def.size() < 3 || !def[0].size() || !def[1].size() || !def[2].size()) {
    /* DUECA websockets.

       Configuration for a "write-and-read" URL is not complete. Check your
       configuration file.
    */
    E_CNF("Need endpoint name + 2 x channel name");
    return false;
  }

  // ensure this URL is not yet known
  if (writereadsetup.count(def[0])) {
    /* DUECA websockets.

       The requested URL location for a "write-and-read" url has
       already been configured.
    */
    E_CNF("location \"/write-and-read/" << def[0] << "\" already defined");
    return false;
  }

  try {
    std::shared_ptr<WriteReadSetup> nentry(new WriteReadSetup(def[1], def[2]));

    // run through any additional arguments
    for (unsigned ii = 3; ii < def.size(); ii++) {
      if (def[ii] == "bulk") {
        nentry->bulk = true;
      }
      else if (def[ii] == "diffpack") {
        nentry->diffpack = true;
      }
      else {
        /* DUECA websockets.

           Wrong keywords found in the setup of a writer and
           reader. Check your configuration files.
        */
        E_CNF("Can only use keywords \"bulk\" and \"diffpack\"");
        return false;
      }
    }

    // all OK, store
    writereadsetup[def[0]] = nentry;
  }
  catch (const std::exception &e) {
    /* DUECA websockets.

       Cannot create the requested set-up for a write-and-read
       connection.
    */
    E_CNF("Cannot create write+read setup, exception " << e.what());
    return false;
  }

  return true;
}

bool WebSocketsServerBase::setCertFiles(const std::vector<std::string> &files)
{
  if (files.size() != 2) {
    /* DUECA websockets.

       When configuring ssl connections, need a set of two files. */
    E_CNF("Need two filenames");
    return false;
  }
  server_crt = files[0];
  server_key = files[1];
  return true;
}

bool WebSocketsServerBase::addMimeType(const std::vector<std::string> &i)
{
  if (i.size() != 2) {
    /* DUECA websockets.

       Supply two strings to add a new mime type.
    */
    E_CNF("Need extension and mime type");
    return false;
  }
  mimemap[i[0]] = i[1];
  return true;
}

// tell DUECA you are prepared
bool WebSocketsServerBase::isPrepared()
{
  bool res = true;

  // check the fixed configured entries
  for (auto &rs: readsingles) {
    res = res && rs.second->checkToken();
  }

  for (auto &fl : followers) {
    res = res && fl.second->checkToken();
  }

  for (auto &pw: presetwriters) {
    res = res && pw.second->checkToken();
  }

  if (res && immediate_start && !auto_started) {
    TimeSpec now(SimTime::now());
    startModule(now);
    auto_started = true;
  }

  // return result of checks
  return res;
}

// start the module
void WebSocketsServerBase::startModule(const TimeSpec &time)
{
  if (!auto_started) {
    // start followers
    for (auto &fl : followers) {
      fl.second->start(time);
    }
    for (auto &fl : autofollowers) {
      fl.second->start(time);
    }

    // switch on the activity
    do_transfer.switchOn(time);
  }
}

// stop the module
void WebSocketsServerBase::stopModule(const TimeSpec &time)
{
  if (!auto_started) {
    for (auto &fl : followers) {
      fl.second->stop(time);
    }
    for (auto &fl : autofollowers) {
      fl.second->stop(time);
    }

    // switch off activity
    do_transfer.switchOff(time);
  }
}

void WebSocketsServerBase::doTransfer(const TimeSpec &ts)
{
  // check late activation
  if (do_transfer.numScheduledBehind()) {
    if (do_transfer.getCheck()) {
      do_transfer.getCheck()->userReportsAnomaly();
    }
    /* DUECA websockets.

       Running/polling the websocket IO took more than the specified
       interval.
    */
    I_XTR("WebSocketsServer, running behind at " << ts);
  }
  DEB3("WebSocketsServer::doTransfer " << ts);

  runcontext->poll();
#ifdef BOOST1_65
  runcontext->reset();
#else
  runcontext->restart();
#endif
}


WEBSOCK_NS_END;
DUECA_NS_END;
#include <dueca/undebug.h>
#include <undebprint.h>

#include "msgpackpacker.hxx"
#include "jsonpacker.hxx"
#include "WebSocketsServer.ixx"

DUECA_NS_START;
WEBSOCK_NS_START;
template <>
const char *const WebSocketsServer<jsonpacker, jsonunpacker>::classname =
  "web-sockets-server";
template <>
const char *const WebSocketsServer<msgpackpacker, msgpackunpacker>::classname =
  "web-sockets-server-msgpack";
template class WebSocketsServer<msgpackpacker,msgpackunpacker>;
template class WebSocketsServer<jsonpacker, jsonunpacker>;
WEBSOCK_NS_END;
DUECA_NS_END;
