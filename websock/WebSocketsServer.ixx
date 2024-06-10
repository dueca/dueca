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

#pragma once

// include the definition of the module class
#include "WebSocketsServer.hxx"
#include "jsonpacker.hxx"
#include "msgpackpacker.hxx"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <dueca/DataClassRegistry.hxx>
#include <sstream>
#include <fstream>

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

#ifdef BOOST1_65
#define BOOST_POSTCALL runcontext->post
#define BOOST_POSTARG1
#else
#define BOOST_POSTCALL boost::asio::post
#define BOOST_POSTARG1 *runcontext,
#endif

DUECA_NS_START;
WEBSOCK_NS_START;

// class/module names
template <>
const char *const WebSocketsServer<jsonpacker, jsonunpacker>::classname =
  "web-sockets-server";

template <>
const char *const WebSocketsServer<msgpackpacker, msgpackunpacker>::classname =
  "web-sockets-server-msgpack";

template <typename Encoder, typename Decoder>
WebSocketsServer<Encoder,Decoder>::WebSocketsServer(Entity *e, const char *part,
                                           const PrioritySpec &ps) :
WebSocketsServerBase(e, part, ps, classname)
{ }

template <typename Encoder, typename Decoder>
WebSocketsServer<Encoder,Decoder>::~WebSocketsServer()
{ }

/** Local function for sending the response data, using 64K chunks */
template <typename R>
static void read_and_send(const R &response,
                          const std::shared_ptr<ifstream> &ifs)
{
  // single-thread only
  static vector<char> buffer(0x10000);
  streamsize read_length;

  if ((read_length =
         ifs->read(&buffer[0], static_cast<streamsize>(buffer.size()))
           .gcount()) > 0) {
    response->write(&buffer[0], read_length);
    if (read_length == static_cast<streamsize>(buffer.size())) {
      response->send([response, ifs](const SimpleWeb::error_code &ec) {
        if (!ec) {
          read_and_send(response, ifs);
        }
        else {
          /* DUECA websockets.

             An error occured in attempting to send a requested
             file data in 64K chunks, as answer to an HTTP
             request. File sending is incomplete.
           */
          E_XTR("File connection interrupted");
        }
      });
    }
  }
}

template <typename S>
bool WebSocketsServerBase::_complete_http(S &server)
{
  server.config.port = http_port;

  // create a generic URL
  server.default_resource["GET"] =
    [this](shared_ptr<typename S::Response> response,
           shared_ptr<typename S::Request> request) {
      try {
        auto web_root_path = boost::filesystem::canonical(this->document_root);
        auto path = boost::filesystem::canonical(web_root_path / request->path);

        DEB("http request for " << request->path);

        // Check if path is within document_root
        if (distance(web_root_path.begin(), web_root_path.end()) >
              distance(path.begin(), path.end()) ||
            !equal(web_root_path.begin(), web_root_path.end(), path.begin())) {
          throw(invalid_argument("path outside root requested"));
        }

        // If this is a folder, get the index file
        if (boost::filesystem::is_directory(path)) {
          path /= "index.html";
        }

        SimpleWeb::CaseInsensitiveMultimap header;
        auto ifs = make_shared<ifstream>();
        ifs->open(path.string(), ifstream::in | ios::binary | ios::ate);

        if (*ifs) {
          auto length = ifs->tellg();
          ifs->seekg(0, ios::beg);
          header.emplace("Content-Length", to_string(length));
          string ext = boost::filesystem::extension(path);
          auto mime = mimemap.find(ext);
          if (mime == mimemap.end()) {
            /* DUECA websockets.

               The http server cannot determine this mime type
            */
            W_XTR("Cannot determine mime type for " << path);
          }
          else {
            header.emplace("Content-Type", mime->second);
          }
          response->write(header);
          read_and_send(response, ifs);
        }
      }
      catch (const exception &e) {
        response->write(SimpleWeb::StatusCode::client_error_bad_request,
                        "Could not open " + request->path + ": " + e.what());
        DEB("HTTP fails for " << request->path << ": " << e.what());
      }
    };

  server.on_error = [](shared_ptr<typename S::Request> request,
                       const SimpleWeb::error_code &ec) {
    // note, error 125 is returned when a client pauses too much
    if (ec.value() != 125) {
      /* DUECA websockets.

         Unexpected error in the HTTP static file server. */
      E_XTR("Http server error code " << ec << " (" << ec.message()
                                      << ") for request :" << request->path
                                      << ' ' << request->query_string);
    }
  };

  server.io_service = runcontext;

  return true;
}

template <typename Encoder, typename Decoder>
template <typename S>
bool WebSocketsServer<Encoder, Decoder>::_complete(S &server)
{
  server.config.port = port;

  // access configuration of the server
  auto &configinfo = server.endpoint["^/configuration"];
  configinfo.on_error = [](shared_ptr<typename S::Connection> connection,
                           const SimpleWeb::error_code &ec) {
      /* DUECA websockets.

 Unexpected error in the "configuration" URL connection. */
    W_XTR("Error in info connection " << connection.get() << ". "
                                      << "Error: " << ec
                                      << ", error message: " << ec.message());
  };
  configinfo.on_open = [this](shared_ptr<typename S::Connection> connection) {
    // Encoder class converts data to binary/ascii
    std::stringstream buf;
    Encoder writer(buf);
      // create a response
      // rapidjson::StringBuffer doc;
      // rapidjson::Writer<rapidjson::StringBuffer> writer(doc);

    writer.StartObject(6);

    writer.Key("current");
    writer.StartArray(readsingles.size());
    for (const auto &sr : readsingles) {
      writer.StartObject(4);
      writer.Key("endpoint");
      writer.String(sr.first.name.c_str());
      writer.Key("dataclass");
      writer.String(sr.second->datatype.c_str());
      writer.Key("typeinfo");
      codeTypeInfo(writer, sr.second->datatype);
      writer.Key("entry");
      writer.Int(sr.first.id);
      writer.EndObject();
    }
    writer.EndArray();

    writer.Key("read");
    writer.StartArray(followers.size());
    for (const auto &fr : followers) {
      writer.StartObject(4);
      writer.Key("endpoint");
      writer.String(fr.first.name.c_str());
      writer.Key("dataclass");
      writer.String(fr.second->datatype.c_str());
      writer.Key("typeinfo");
      codeTypeInfo(writer, fr.second->datatype);
      writer.Key("entry");
      writer.Int(fr.first.id);
      writer.EndObject();
    }
    writer.EndArray();

    writer.Key("info");
    writer.StartArray(monitors.size());
    for (const auto &mn : monitors) {
      writer.StartObject(1);
      writer.Key("endpoint");
      writer.String(mn.first.c_str());
      writer.EndObject();
    }
    writer.EndArray();

    writer.Key("write");
    writer.StartArray(writersetup.size());
    for (const auto &wr : writersetup) {
      writer.StartObject(2);
      writer.Key("endpoint");
      writer.String(wr.first.c_str());
      writer.Key("dataclass");
      writer.String(wr.second->dataclass.c_str());
      writer.Key("typeinfo");
      codeTypeInfo(writer, wr.second->dataclass);
      writer.EndObject();
    }
    writer.EndArray();

    writer.Key("write-and-read");
    writer.StartArray(writereadsetup.size());
    for (const auto &wr : writereadsetup) {
      writer.StartObject(1);
      writer.Key("endpoint");
      writer.String(wr.first.c_str());
      writer.EndObject();
    }
    writer.EndArray();

    writer.Key("granule");
    writer.Double(Ticker::single()->getTimeGranule());

    writer.EndObject();
    writer.EndLine();

    connection->send(buf.str(), [](const SimpleWeb::error_code &ec) {
      if (ec) {
             /* DUECA websockets.

Unexpected error in sending the configuration
information. */
        W_XTR("Error sending message " << ec);
      }
    });
    DEB("New connection on ^/configuration, sent data" << buf.str());

      // removed, closing at this point upsets some clients
      // const std::string reason("Configuration data sent");
      // connection->send_close(1000, reason);
  };
  configinfo.on_close = [this](shared_ptr<typename S::Connection> connection,
                               int status, const std::string &reason) {
      /* DUECA websockets.

 Information on the closing of the connection of a client with
 the configuration URL. */
    I_XTR("Closing configuration endpoint "
          << " code: " << status << " reason: \"" << reason << '"');
  };

  // access channel data on request; each message (no data needed)
  // is replied to with the current value read from the accessed channel
  // and entry
  auto &current = server.endpoint["^/current/([a-zA-Z0-9_-]+)$"];

  current.on_message = [this](shared_ptr<typename S::Connection> connection,
                              shared_ptr<typename S::InMessage> in_message) {
    DEB("Message on connection 0x"
        << std::hex << reinterpret_cast<void *>(connection.get()) << std::dec);

      // find the channel access corresponding to the connection
    auto em =
      this->singlereadsmapped.find(reinterpret_cast<void *>(connection.get()));

    if (em == this->singlereadsmapped.end()) {
        /* DUECA websockets.

   Cannot find the connection entry for a message to the
   "current" URL. */
      E_XTR("Cannot find connection");
      const std::string reason("Server failure, cannot find connection data");
      connection->send_close(1001, reason);
      return;
    }

      // room for response
    std::stringstream buf;
    Encoder writer(buf);
    writer.StartObject(2);
    try {
        // create the reader
      DCOReader r(em->second->datatype.c_str(), em->second->r_token);
      DataTimeSpec dtd = r.timeSpec();
      writer.Key("tick");
      writer.Uint(dtd.getValidityStart());
      writer.Key("data");
      writer.dco(r);
    }
    catch (const NoDataAvailable &e) {
        /* DUECA websockets.

   There is no current data on the requested stream.
*/
      W_XTR("No data on " << em->second->r_token.getName()
                          << " sending empty {}");
    }
    writer.EndObject();
    connection->send(buf.str(), [](const SimpleWeb::error_code &ec) {
      if (ec) {
             /* DUECA websockets.

Unexpected error in sending a message to a client for
the "current" URL */
        W_XTR("Error sending message " << ec);
      }
    });
  };

  current.on_error = [](shared_ptr<typename S::Connection> connection,
                        const SimpleWeb::error_code &ec) {
      /* DUECA websockets.

 Unexpected error in the "current" URL connection. */
    W_XTR("Error in connection " << connection.get() << ". "
                                 << "Error: " << ec
                                 << ", error message: " << ec.message());
  };

  current.on_close = [this](shared_ptr<typename S::Connection> connection,
                            int status, const std::string &reason) {
    DEB("Close on connection 0x"
        << std::hex << reinterpret_cast<void *>(connection.get()) << std::dec);

    std::string ename("unknown");
    auto qpars = SimpleWeb::QueryString::parse(connection->query_string);
    auto ekey = qpars.find("entry");
    if (ekey != qpars.end()) {
      ename = ekey->second;
    }

      /* DUECA websockets.

 Information on the closing of the connection of a client with
 a "current" URL. */
    I_XTR("Closing endpoint at /current/"
          << connection->path_match[1] << "?entry=" << ename
          << " code: " << status << " reason: \"" << reason << '"');

      // find the mapped connection, and remove
    if (this->singlereadsmapped.erase(
          reinterpret_cast<void *>(connection.get()))) {
        // OK
    }
    else {
        /* DUECA websockets.

   Programming error? Cannot find the connection corresponding
   to a close attempt on a "current" URL.
*/
      W_XTR("Cannot find mapping for endpoint at /current/"
            << connection->path_match[1] << "?entry=" << ename);
    }
  };

  // open should be last?
  current.on_open = [this](shared_ptr<typename S::Connection> connection) {
    DEB("Open on connection 0x"
        << std::hex << reinterpret_cast<void *>(connection.get()) << std::dec);
    DEB("New connection currentdata");

      // find the specific URL, and entry number
    auto qpars = SimpleWeb::QueryString::parse(connection->query_string);
    auto ekey = qpars.find("entry");
    unsigned entry = 0;
    if (ekey != qpars.end()) {
      entry = boost::lexical_cast<unsigned>(ekey->second);
    }

      // try to find the mapped reader
    NameEntryId key(connection->path_match[1], entry);
      // ScopeLock l(this->thelock);
    auto ee = this->readsingles.find(key);
    auto ea = this->autosingles.find(key);

      // if not found, try to look in the channelinfo bag
    if (ee == this->readsingles.end() && ea == this->autosingles.end()) {
      auto mon = this->monitors.find(connection->path_match[1]);

        // try to create an entry if possible
      if (mon != this->monitors.end()) {
        std::string datatype = mon->second->findEntry(entry);
        if (datatype.size()) {
          std::shared_ptr<SingleEntryRead> newcur(new SingleEntryRead(
            mon->second->channelname, datatype, entry, this->getId()));

            // insert the entry, and find it again
          this->autosingles[key] = newcur;
          ea = this->autosingles.find(key);
        }
      }
    }

    if (ee == this->readsingles.end() && ea == this->autosingles.end()) {

        // URL point not found, and not creatable
      const std::string reason("Resource not available");
      connection->send_close(1001, reason);
    }
    else {
      // connect the connection to the located or created channel reader
      this->singlereadsmapped[reinterpret_cast<void *>(connection.get())] =
        ee != this->readsingles.end() ? ee->second : ea->second;
    }
  };

  // access channel data to collect all data; messages are followed
  auto &follow = server.endpoint["^/read/([a-zA-Z0-9_-]+)$"];

  follow.on_error = [](shared_ptr<typename S::Connection> connection,
                       const SimpleWeb::error_code &ec) {
    /* DUECA websockets.

       Unexpected error in the "follow" URL connection. */
    W_XTR("Error in connection " << connection.get() << ". "
                                 << "Error: " << ec
                                 << ", error message: " << ec.message());
  };

  follow.on_close = [this](shared_ptr<typename S::Connection> connection,
                           int status, const std::string &reason) {
    // find the specific URL, and entry number
    auto qpars = SimpleWeb::QueryString::parse(connection->query_string);
    auto ekey = qpars.find("entry");
    unsigned entry = 0;
    if (ekey != qpars.end()) {
      entry = boost::lexical_cast<unsigned>(ekey->second);
    }

    // try to find the setup object
    NameEntryId key(connection->path_match[1], entry);
    // ScopeLock l(this->thelock);

    // obtain the follower, first look at manually configured ones
    auto ee = this->followers.find(key);

    // indicate this connection is leaving
    if (ee == this->followers.end()) {
      ee = this->autofollowers.find(key);
      if (ee == autofollowers.end()) {
        /* DUECA websockets.

           Programming error? Cannot find the connection corresponding
           to a close attempt on a "read" URL.
        */
        E_XTR("Trying to close connection, cannot find mapping at "
              << "/read/" << connection->path_match[1]);
        return;
      }
    }
    if (ee->second->removeConnection(connection)) {
      /* DUECA websockets.

         Programming error? Cannot remove the connection corresponding
         to a close attempt on a "read" URL.
      */
      E_XTR("Closing connection, cannot remove connection at "
            << "/read/" << connection->path_match[1]);
      return;
    }
  };

  follow.on_open = [this](shared_ptr<typename S::Connection> connection) {
    // find the specific URL, and entry number
    auto qpars = SimpleWeb::QueryString::parse(connection->query_string);
    auto ekey = qpars.find("entry");
    unsigned entry = 0;
    if (ekey != qpars.end()) {
      entry = boost::lexical_cast<unsigned>(ekey->second);
    }
    DEB("New read connection attempt " << connection->path_match[1] << " entry "
                                       << entry);

    // try to find the setup object
    NameEntryId key(connection->path_match[1], entry);
    // ScopeLock l(this->thelock);

    // figure out if this channel/entry is already being followed
    auto ee = this->followers.find(key);
    bool foundconnect = false;

    // if this is not the case, it might be configured
    if (ee != this->followers.end()) {
      foundconnect = true;
    }
    else {

      // run through the monitors now
      auto mm = this->monitors.find(connection->path_match[1]);
      if (mm != this->monitors.end()) {

        // and this entry should exist
        auto dataclass = mm->second->findEntry(entry);
        if (dataclass.size()) {

          // check whether we have one already
          ee = this->autofollowers.find(key);

          if (ee == this->autofollowers.end()) {
            DEB("Creating new follow on " << mm->second->channelname
                                          << " entry " << entry << "("
                                          << dataclass << ")");
            std::shared_ptr<SingleEntryFollow> newfollow(new SingleEntryFollow(
              mm->second->channelname, dataclass, entry, this,
              this->read_prio, mm->second->time_spec, extended, true));
            this->autofollowers[key] = newfollow;
            ee = this->autofollowers.find(key);
          }
          foundconnect = ee != this->autofollowers.end();
        }
      }
    }

    if (foundconnect) {
      ee->second->addConnection(connection);
    }
    else {
      const std::string reason("Resource not available");
      connection->send_close(1001, reason);
    }
  };

  auto &monitor = server.endpoint["^/info/([a-zA-Z0-9_-]+)$"];

  monitor.on_error = [](shared_ptr<typename S::Connection> connection,
                        const SimpleWeb::error_code &ec) {
    /* DUECA websockets.

       Unexpected error in an "info" URL connection. */
    W_XTR("Error in connection " << connection.get() << ". "
                                 << "Error: " << ec
                                 << ", error message: " << ec.message());
  };

  monitor.on_close = [this](shared_ptr<typename S::Connection> connection,
                            int status, const std::string &reason) {
    // try to find the monitoring object
    std::string key(connection->path_match[1]);
    // ScopeLock l(this->thelock);

    // obtain the monitor
    auto ee = this->monitors.find(key);

    if (ee == this->monitors.end()) {
      /* DUECA websockets.

         Programming error? Cannot find the connection corresponding
         to a close attempt on an "info" URL.
      */
      E_XTR("Closing connection, cannot find mapping at /info/"
            << connection->path_match[1]);
    }
    else {
      if (!ee->second->removeConnection(connection)) {
        /* DUECA websockets.

           Programming error? Cannot remove the connection corresponding
           to a close attempt on an "info" URL.
        */
        E_XTR("Closing connection, cannot find connection at /info/"
              << connection->path_match[1]);
      }
    }
  };

  monitor.on_open = [this](shared_ptr<typename S::Connection> connection) {
    // try to find the monitoring object
    std::string key(connection->path_match[1]);
    // ScopeLock l(this->thelock);

    // obtain the monitor
    auto ee = this->monitors.find(key);

    if (ee == this->monitors.end()) {
      const std::string reason("Resource not available");
      connection->send_close(1001, reason);
    }
    else {
      /* DUECA websockets.

         Information on an additional client on an "info" URL.
      */
      I_XTR("Adding channel monitoring connection at /info/" << key);
      ee->second->addConnection(connection);
    }
  };

  // writing to DUECA from a websocket connection
  auto &writer = server.endpoint["^/write/([a-zA-Z0-9_-]+)$"];

  // error response
  writer.on_error = [](shared_ptr<typename S::Connection> connection,
                       const SimpleWeb::error_code &ec) {
    /* DUECA websockets.

       Unexpected error in a "write" URL connection. */
    W_XTR("Error in connection " << connection->path_match[0] << ". "
                                 << "Error: " << ec
                                 << ", error message: " << ec.message());
  };

  writer.on_open = [this](shared_ptr<typename S::Connection> connection) {
    // try to find the setup
    // ScopeLock l(this->thelock);

    // just accept the connection.
    std::string key = connection->path_match[1];

    // check no entry is present on this connection
    auto ww = this->writers.find(reinterpret_cast<void *>(connection.get()));
    if (ww != this->writers.end()) {
      /* DUECA websockets.

         Entry is not free. */
      W_XTR("There is already a writer on " << connection->path_match[0]
                                            << ", closing.");
      const std::string reason("Server logic error");
      connection->send_close(1007, reason);
      return;
    }

    // this is either defined in the writer setup (dynamic) or
    // in the presetwriters, never in both
    auto ee = this->writersetup.find(key);
    auto pre = this->presetwriters.find(key);
    assert(
      !(ee != this->writersetup.end() && pre != this->presetwriters.end()));

    // check that this URL is available
    if (ee == this->writersetup.end() && pre == this->presetwriters.end()) {
      /* DUECA websockets.

         there is no endpoint here. */
      W_XTR("URL not available on " << connection->path_match[0]
                                    << ", closing.");
      const std::string reason("Resource not available");
      connection->send_close(1001, reason);
      return;
    }

    // actions if a specific entry has been pre-cooked
    if (pre != this->presetwriters.end()) {
      if (pre->second->isAvailable() || aggressive_reconnect) {
        if (!pre->second->isAvailable()) {
          /* DUECA websockets.

             Warning about a new connection taking over from an
             existing connection for a preset write entry. You might
             have started an external program twice, you may ignore
             the warning if this simply replaces a connection with a
             crashed, now defunct program.
          */
          W_XTR("New connection for " << pre->second->identification
                                      << " forcing old connection to close");
          auto it = this->writers.find(pre->second->disConnect());
          if (it != this->writers.end()) {
            this->writers.erase(it);
          }
          else {
            /* DUECA websockets.

               Trying to replace and remove an existing connection
               to a preset write entry, but the old connection
               cannot be found. */
            W_XTR("Could not find old connection to remove");
          }
        }
        pre->second->doConnect(connection);
        this->writers[reinterpret_cast<void *>(connection.get())] = pre->second;
        return;
      }
      else {
        /* DUECA websockets.

           There is already a connection here.
        */
        W_XTR("There is already a connection on " << connection->path_match[0]);
        const std::string reason("Resource already connected");
        connection->send_close(1001, reason);
        return;
      }
    }

    // create an empty write-entry
    this->writers[reinterpret_cast<void *>(connection.get())] =
      boost::intrusive_ptr<WriteEntry>(
        new WriteEntry(ee->second->channelname, ee->second->dataclass));
  };

  writer.on_message = [this](shared_ptr<typename S::Connection> connection,
                             shared_ptr<typename S::InMessage> in_message) {
    // find the entry, of type WriteEntry
    auto ww = this->writers.find(reinterpret_cast<void *>(connection.get()));

    // check it is there
    if (ww == this->writers.end()) {
      const std::string reason("Resource not available");
      connection->send_close(1001, reason);
      return;
    }

    // is it complete? Simply write
    if (ww->second->isComplete()) {
      if (ww->second->checkToken()) {
        try {
          Decoder dec(in_message->string());
          ww->second->writeFromCoded(dec);
        }
        catch (const dataparseerror &) {
          const std::string reason("data coding error");
          connection->send_close(1007, reason);
          return;
        }
      }
    }
    else {
      try {
        ww->second->complete(in_message->string(), this->getId());
      }
      catch (const connectionparseerror &) {
        const std::string reason("connection start incorrect");
        connection->send_close(1007, reason);
        return;
      }
      catch (const presetmismatch &) {
        const std::string reason("connection start does not match preset");
        connection->send_close(1007, reason);
        return;
      }
    }
  };

  // close occurrence
  writer.on_close = [this](shared_ptr<typename S::Connection> connection,
                           int status, const std::string &reason) {
    /* DUECA websockets.

       Information on the closing of the connection of a client with
       a "write" URL. */
    I_XTR("Closing endpoint at /write/" << connection->path_match[1]
                                        << " code: " << status << " reason: \""
                                        << reason << '"');

    // find the entry
    auto ww = this->writers.find(reinterpret_cast<void *>(connection.get()));

    if (ww != this->writers.end()) {
      ww->second->doDisconnect();
      this->writers.erase(ww);
      // OK
    }
    else {
      /* DUECA websockets.

         Programming error? Cannot find the connection corresponding
         to a close attempt on a "write" URL.
      */
      W_XTR("Cannot find mapping for endpoint at /write/"
            << connection->path_match[1]);
    }
  };

  auto &writerreader = server.endpoint["^/write-and-read/([a-zA-Z0-9_-]+)$"];

  writerreader.on_error = [](shared_ptr<typename S::Connection> connection,
                             const SimpleWeb::error_code &ec) {
    DEB("Error in write-and-read connection");
    /* DUECA websockets.

       Unexpected error in a "write-and-read" URL connection. */
    W_XTR("Error in connection " << connection->path_match[0] << ". "
                                 << "Error: " << ec
                                 << ", error message: " << ec.message());
  };

  writerreader.on_open = [this](shared_ptr<typename S::Connection> connection) {
    // try to find the setup
    // ScopeLock l(this->thelock);

    // just accept the connection.
    std::string key = connection->path_match[1];
    DEB("Opened /write-and-read/" << key);

    // this must be defined
    auto ee = this->writereadsetup.find(key);

    // check that this URL is available
    if (ee == this->writereadsetup.end()) {
      const std::string reason("Resource not available");
      connection->send_close(1001, reason);
      return;
    }

    // create an initial write-read-entry
    this->writersreaders[reinterpret_cast<void *>(connection.get())] =
      boost::intrusive_ptr<WriteReadEntry>(
        new WriteReadEntry(ee->second, this, read_prio, extended));
    this->writersreaders[reinterpret_cast<void *>(connection.get())]
      ->setConnection(connection);
  };

  writerreader.on_message =
    [this](shared_ptr<typename S::Connection> connection,
           shared_ptr<typename S::InMessage> in_message) {
      // find the entry
      auto ww =
        this->writersreaders.find(reinterpret_cast<void *>(connection.get()));

      // check it is there
      if (ww == this->writersreaders.end()) {
        const std::string reason("Resource not available");
        connection->send_close(1001, reason);
        return;
      }

      // is it complete? Simply write
      if (ww->second->isComplete()) {
        if (ww->second->checkToken()) {
          try {
            Decoder dec(in_message->string());
            ww->second->writeFromCoded(dec);
          }
          catch (const exception &e) {
            /* DUECA websockets.

               Error in attempting to read data from a "write-and-read" URL.
            */
            E_XTR("Exception trying to read from socket "
                  << e.what() << " msg:" << in_message->string());
            const std::string reason("data coding error");
            connection->send_close(1007, reason);
            return;
          }
        }
        else {
          /* DUECA websockets.

             There is data coming in on a "write-and-read" url, while
             the configuration is not yet complete. Check your
             external client; wait with sending further data until an
             information message with configuration has arrived.
          */
          W_XTR("/write-and-read/" << connection->path_match[1]
                                   << " not yet complete");
        }
      }
      else {
        try {
          // call on the connection to complete itself
          ww->second->complete(in_message->string());
        }
        catch (const connectionparseerror &) {
          const std::string reason("connection start incorrect");
          connection->send_close(1007, reason);
          return;
        }
      }
    };

  // close occurrence
  writerreader.on_close = [this](shared_ptr<typename S::Connection> connection,
                                 int status, const std::string &reason) {
    // find the writing object, using the connection as index
    // ScopeLock l(this->thelock);
    auto wr =
      this->writersreaders.find(reinterpret_cast<void *>(connection.get()));

    if (wr == this->writersreaders.end()) {
      /* DUECA websockets.

         Programming error? Cannot find the connection corresponding
         to a close attempt on a "write-and-read" URL.
      */
      E_XTR("Closing connection, cannot find mapping at "
            << "/write-and-read/" << connection->path_match[1]);
    }
    else {
      /* DUECA websockets.

         Information on the closing of the connection of a client with
         a "write-and-read" URL. */
      I_XTR("Closing connection and writer " << wr->second->identification);
      wr->second->doDisconnect();
      this->writersreaders.erase(wr);
    }
  };

  server.io_service = runcontext;

  return true;
}

#ifdef BOOST1_65
#define BOOST_POSTCALL runcontext->post
#define BOOST_POSTARG1
#else
#define BOOST_POSTCALL boost::asio::post
#define BOOST_POSTARG1 *runcontext,
#endif

template <typename Encoder, typename Decoder>
bool WebSocketsServer<Encoder, Decoder>::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */

  if (server_crt.size() && server_key.size()) {

    // ssl version
    sserver.reset(new WssServer(server_crt, server_key));
    if (!_complete(*sserver))
      return false;

    // add the server run to the run context
    BOOST_POSTCALL(BOOST_POSTARG1[this]() { this->sserver->start(); });

    if (document_root.size()) {
      https_server.reset(new HttpsServer(server_crt, server_key));
      if (!_complete_http(*https_server))
        return false;

      // add the server run to the run context
      BOOST_POSTCALL(BOOST_POSTARG1[this]() { this->https_server->start(); });
    }
    return true;
  }

  server.reset(new WsServer());
  if (!_complete(*server))
    return false;
  BOOST_POSTCALL(BOOST_POSTARG1[this]() { this->server->start(); });

  if (document_root.size()) {
    http_server.reset(new HttpServer);
    if (!_complete_http(*http_server))
      return false;

    // add the server run to the run context
    BOOST_POSTCALL(BOOST_POSTARG1[this]() { this->http_server->start(); });
  }

  return true;
}

template <typename Encoder, typename Decoder>
void WebSocketsServer<Encoder, Decoder>::codeData(std::ostream &s,
                                                  const DCOReader &r) const
{
  Encoder writer(s);
  DataTimeSpec dtd = r.timeSpec();
  writer.StartObject(2);
  writer.Key("tick");
  writer.Uint(dtd.getValidityStart());
  writer.Key("data");
  writer.dco(r);
  writer.EndObject();
}

template <typename Encoder>
void codeTypeInfo(Encoder &writer, const std::string &dataclass)
{
  CommObjectReaderWriter rw(dataclass.c_str());
  writer.StartArray(rw.getNumMembers());
  for (size_t ii = 0; ii < rw.getNumMembers(); ii++) {
    unsigned nelts =
      (DataClassRegistry::single().isRegistered(rw.getMemberClass(ii)) ? 3
                                                                       : 2) +
      ((rw.getMemberArity(ii) == FixedIterable ||
        rw.getMemberArity(ii) == Iterable)
         ? 1
         : 0) +
      (rw.getMemberArity(ii) == Mapped ? 2 : 0);
    writer.StartObject(nelts);
    writer.Key("name");
    writer.String(rw.getMemberName(ii));
    writer.Key("type");
    writer.String(rw.getMemberClass(ii));
    if (DataClassRegistry::single().isRegistered(rw.getMemberClass(ii))) {
      writer.Key("typeinfo");
      codeTypeInfo(writer, rw.getMemberClass(ii));
    }
    switch (rw.getMemberArity(ii)) {
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

template <typename Encoder, typename Decoder>
void WebSocketsServer<Encoder, Decoder>::codeEntryInfo(
  std::ostream &s, const std::string &w_dataname, unsigned w_entryid,
  const std::string &r_dataname, unsigned r_entryid) const
{
  Encoder writer(s);
  if (w_dataname.size() && r_dataname.size()) {
    writer.StartObject(2);
    writer.Key("read");
    writer.StartObject(3);
    writer.Key("dataclass");
    writer.String(r_dataname);
    writer.Key("entry");
    writer.Uint(r_entryid);
    writer.Key("typeinfo");
    codeTypeInfo(writer, r_dataname);
    writer.Key("write");
    writer.StartObject(3);
    writer.Key("dataclass");
    writer.String(w_dataname);
    writer.Key("entry");
    writer.Uint(w_entryid);
    writer.Key("typeinfo");
    codeTypeInfo(writer, w_dataname);
    writer.EndObject();
  }
  else {
    const std::string &dataname =
      r_dataname.size() == 0 ? w_dataname : r_dataname;
    const unsigned entryid = r_dataname.size() == 0 ? w_entryid : r_entryid;
    writer.StartObject(3);
    writer.Key("dataclass");
    writer.String(dataname);
    writer.Key("entry");
    writer.Uint(entryid);
    writer.Key("typeinfo");
    codeTypeInfo(writer, dataname);
  }
}

template <typename Decoder>
void WriteReadEntry::writeFromCoded(const Decoder &doc)
{
  DCOWriter wr(*w_token, DataTimeSpec::now());
  try {
    doc.codedToDCO(wr);
  }
  catch (const dueca::ConversionNotDefined &e) {
    /* DUECA websockets.

       Failed to decode a DCO object from the received JSON
       string. Check the correspondence between the DCO object and the
       external program. */
    W_XTR("Websockets, cannot decode '" << w_token->getDataClassName()
                                        << "' from 'data'");
    wr.failed();
  }
}


template <typename Decoder> void WriteEntry::writeFromCoded(const Decoder &doc)
{
  DataTimeSpec ts;
  if (ctiming) {
    if (stream) {
      ts = doc.getStreamTime();
    }
    else {
      ts = doc.getTime();
    }
  }
  else {
    // follow current time
    ts.validity_start = ts.validity_end = SimTime::now();
  }

  DCOWriter wr(*w_token, ts);
  try {
    doc.codedToDCO(wr);
  }
  catch (const dueca::ConversionNotDefined &e) {
    /* DUECA websockets.

       Failed to decode an object of the given dataclass from the JSON
       string received. Check the correspondence between your
       (external) program and the DUECA object definitions. */
    W_XTR("Websockets, cannot extract '" << w_token->getDataClassName()
                                         << "' from 'data'");
    wr.failed();
  }
}


WEBSOCK_NS_END;
DUECA_NS_END;
