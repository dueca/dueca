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
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <dueca/DCOtoJSON.hxx>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>

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

template <typename Encoder>
template <typename S>
bool WebSocketsServer<Encoder>::_complete(S &server)
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
    Encoder writer;
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
      writeTypeInfo(writer, sr.second->datatype);
      writer.Key("entry");
      writer.Int(sr.first.id);
      writer.EndObject();
    }
    writer.array();

    writer.Key("read");
    writer.StartArray(followers.size());
    for (const auto &fr : followers) {
      writer.StartObject(4);
      writer.Key("endpoint");
      writer.String(fr.first.name.c_str());
      writer.Key("dataclass");
      writer.String(fr.second->datatype.c_str());
      writer.Key("typeinfo");
      writeTypeInfo(writer, fr.second->datatype);
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
      writeTypeInfo(writer, wr.second->dataclass);
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

    connection->send(writer.getstring(), [](const SimpleWeb::error_code &ec) {
      if (ec) {
             /* DUECA websockets.

 Unexpected error in sending the configuration
 information. */
        W_XTR("Error sending message " << ec);
      }
    });
    DEB("New connection on ^/configuration, sent data" << writer.getstring());

      // removed, closing at this point upsets some clients
      // const std::string reason("Configuration data sent");
      // connection->send_close(1000, reason);
  };
  configinfo.on_close = [this](shared_ptr<typename S::Connection> connection,
                               int status, const std::string &reason) {
      /* DUECA websockets.

   Information on the closing of the connection of a client with
   the configuration URL. */
    I_XTR("Closing configuration endpoint " << " code: " << status
                                            << " reason: \"" << reason << '"');
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
    Encoder writer;
    writer.StartObject(2);
    try {
        // create the reader
      DCOReader r(em->second->datatype.c_str(), em->second->r_token);
      DataTimeSpec dtd = r.timeSpec();
      writer.Key("tick");
      writer.uinteger(dtd.getValidityStart());
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
    connection->send(doc.getstring(), [](const SimpleWeb::error_code &ec) {
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
    W_XTR("Error in connection " << connection.get() << ". " << "Error: " << ec
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
    W_XTR("Error in connection " << connection.get() << ". " << "Error: " << ec
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
              mm->second->channelname, dataclass, entry, this->getId(),
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
    W_XTR("Error in connection " << connection.get() << ". " << "Error: " << ec
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
      E_XTR("Closing connection, cannot find mapping at "
            << "/info/" << connection->path_match[1]);
    }
    else {
      if (!ee->second->removeConnection(connection)) {
        /* DUECA websockets.

           Programming error? Cannot remove the connection corresponding
           to a close attempt on an "info" URL.
        */
        E_XTR("Closing connection, cannot find connection at "
              << "/info/" << connection->path_match[1]);
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
    // find the entry
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
          ww->second->writeFromJSON(in_message->string());
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
            ww->second->writeFromJSON(in_message->string());
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

template<typename Encoder>
bool WebSocketsServer<Encoder>::complete()
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

WEBSOCK_NS_END;
DUECA_NS_END;
