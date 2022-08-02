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


#define WebSocketsServer_cxx

// include the definition of the module class
#include <dueca/DCOtoJSON.hxx>
#include "WebSocketsServer.hxx"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
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

#define DEBPRINTLEVEL 2
#include <debprint.h>

DUECA_NS_START;
WEBSOCK_NS_START;

// class/module name
const char* const WebSocketsServer::classname = "web-sockets-server";

// Parameters to be inserted
const ParameterTable* WebSocketsServer::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_,vector<int> >
      (&_ThisModule_::checkTiming), check_timing_description },

    { "current",
      new MemberCall<_ThisModule_,std::vector<std::string> >
      (&_ThisModule_::setCurrentData),
      "Set up a URL for simply reading the latest data from a single entry\n"
      "in a channel. Specify the URL name, channel name, dataclass, and\n"
      "optionally (in a string) the entry number. Data is returned whenever\n"
      "requested by (an empty) message\n"
      "This results in a URL /current/name?entry=...\n"
      "The returned JSON objects have members \"tick\" for timing and \n"
      "\"data\" with the DCO object encoded in JSON" },

    { "read-timing",
      new VarProbe<_ThisModule_,TimeSpec>
      (&_ThisModule_::time_spec),
      "Set the read timing for read data. TimeSpec(0,0) disables this\n"
      "again, and will read at the rate the data was sent to the channel." },

    { "extended",
      new VarProbe<_ThisModule_,bool>
      (&_ThisModule_::extended),
      "Use the extended (sloppy) JSON, with NaN, Infinite and -Infinite.\n"
      "Ensure any external clients understand this format." },

    { "read",
      new MemberCall<_ThisModule_,std::vector<std::string> >
      (&_ThisModule_::setFollowData),
      "Set up a URL for tracking all data from a single entry in a channel.\n"
      "Specify the URL name, channel name, dataclass, and optionally (in a\n"
      "string) the entry number. Data is pushed to the client, if specified\n"
      "before, the rate given by \"read-timing\" is used for reading"
      "This results in a URL /read/name?entry=...\n" },

    { "info",
      new MemberCall<_ThisModule_,std::vector<std::string> >
      (&_ThisModule_::setChannelInfo),
      "Set up a URL for obtaining information about channel entries.\n"
      "Specify the URL name and channel name. Any discovered entries will\n"
      "be available in the /current or /read URL's"
      "Configuring this results in a URL /info/name\n"
      "Information will be provided upon completion of the websocket." },

    { "write",
      new MemberCall<_ThisModule_,std::vector<std::string> >
      (&_ThisModule_::setWriterSetup),
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
      new MemberCall<_ThisModule_,std::vector<std::string> >
      (&_ThisModule_::setPresetWriterSetup),
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
      new VarProbe<_ThisModule_,bool>
      (&_ThisModule_::aggressive_reconnect),
      "Normally a preset entry is only reconnected after the connection with\n"
      "the previous client has been closed. Setting this variable to true\n"
      "makes reconnection more aggressive; old connections are closed and\n"
      "a new connection is always made."  },

    { "immediate-start",
      new VarProbe<_ThisModule_,bool>
      (&_ThisModule_::immediate_start),
      "Start as soon as possible, i.e., when watched channels are valid\n"
      "This is normally only needed for specific debugging purposes." },

    { "write-and-read",
      new MemberCall<_ThisModule_,std::vector<std::string> >
      (&_ThisModule_::setWriteReadSetup),
      "Set up a URL connecting a reading and writing channel set. Specify\n"
      "URL name, channel name for writing, channel name for reading,\n"
      "optionally, add \"bulk\" for bulk sending mode and \"diffpack\" for\n"
      "differential packing.\n"
      "The communication is event-based, the first message from the client\n"
      "should have a JSON struct with information on the written dataclass\n"
      "( { \"dataclass\": ... } ). The first received message will have\n"
      "information on both dataclasses and channel connections:\n"
      "( { \"write\": { \"dataclass\": ..., \"entry\": ##, \"typeinfo\": {...}},\n"
      "(   \"read\":  { \"dataclass\": ..., \"entry\": ##, \"typeinfo\": {...}}\n"
      "The DUECA module servicing this connection will need to create matching\n"
      "entries to any write created by a connection, using the same label as\n"
      "generated by the socket server.\n"
      "This results in a URL /write-and-read/name, each connection creates a\n"
      "pair of entries, for completion you need a DUECA module monitoring the\n"
      "written channel and submitting an entry in the reading channel with a\n"
      "label matching the written entry" },

    { "port",
      new VarProbe<_ThisModule_,unsigned>(&_ThisModule_::port),
      "Server port to be used, default is 8001." },

    { "http-port",
       new VarProbe<_ThisModule_,unsigned>(&_ThisModule_::http_port),
      "If selected by setting a path, http server port, default is 8000." },

    { "document-root",
      new VarProbe<_ThisModule_,std::string>(&_ThisModule_::document_root),
      "Location of static files to serve over http; if not supplied, no\n"
      "http server is created.\n" },

    { "certfiles",
      new MemberCall<_ThisModule_,std::vector<std::string> >
      (&_ThisModule_::setCertFiles),
      "Certificate files for SSL, specify a .crt and a .key file. If these\n"
      "are supplied, wss sockets will be used instead of ws" },

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
      "JSON server providing access to selected channels and channel\n"
      "entries with websockets."} };

  return parameter_table;
}

// constructor
WebSocketsServer::WebSocketsServer(Entity* e, const char* part, const
                                   PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  server(),
  sserver(),
  http_server(),
  https_server(),
  server_crt(),
  server_key(),
#if 0
  server_thread(),
#else
  runcontext(new boost::asio::io_context),
#endif
  port(8001),
  http_port(8000),
  document_root(),
  aggressive_reconnect(false),
  immediate_start(false),
  auto_started(false),
  thelock("JSON ws(s) server", false),
  read_prio(ps),
  time_spec(0,0),
  extended(false),
  readsingles(),
  autosingles(),
  singlereadsmapped(),
  followers(),
  monitors(),
  myclock(),
  cb1(this, &WebSocketsServer::doTransfer),
  do_transfer(getId(), "run websocket IO", &cb1, ps)
{
  // connect the triggers for simulation
  do_transfer.setTrigger(myclock);
}


template<typename S>
bool WebSocketsServer::_complete(S& server)
{
  server.config.port = port;

  // access configuration of the server
  auto &configinfo = server.endpoint["^/configuration"];
  configinfo.on_error =
    [](shared_ptr<typename S::Connection> connection,
           const SimpleWeb::error_code &ec) {
      /* DUECA websockets.

         Unexpected error in the "configuration" URL connection. */
      W_XTR("Error in info connection " << connection.get() << ". " <<
            "Error: " << ec << ", error message: " << ec.message());
    };
  configinfo.on_open =
    [this](shared_ptr<typename S::Connection> connection) {

      // create a response
      rapidjson::StringBuffer doc;
      rapidjson::Writer<rapidjson::StringBuffer> writer(doc);

      writer.StartObject();

      writer.Key("current");
      writer.StartArray();
      for (const auto &sr: readsingles) {
        writer.StartObject();
        writer.Key("endpoint");
        writer.String(sr.first.name.c_str());
        writer.Key("datatype");
        writer.String(sr.second->datatype.c_str());
        writer.EndObject();
      }
      writer.EndArray();

      writer.Key("read");
      writer.StartArray();
      for (const auto &fr: followers) {
        writer.StartObject();
        writer.Key("endpoint");
        writer.String(fr.first.name.c_str());
        writer.Key("datatype");
        writer.String(fr.second->datatype.c_str());
        writer.EndObject();
      }
      writer.EndArray();

      writer.Key("info");
      writer.StartArray();
      for (const auto &mn: monitors) {
        writer.StartObject();
        writer.Key("endpoint");
        writer.String(mn.first.c_str());
        writer.EndObject();
      }
      writer.EndArray();

      writer.Key("write");
      writer.StartArray();
      for (const auto &wr: writersetup) {
        writer.StartObject();
        writer.Key("endpoint");
        writer.String((std::string("write/") + wr.first).c_str());
        writer.Key("datatype");
        writer.String(wr.second->dataclass.c_str());
        writer.EndObject();
      }
      writer.EndArray();

      writer.Key("write-and-read");
      writer.StartArray();
      for (const auto &wr: writereadsetup) {
        writer.StartObject();
        writer.Key("endpoint");
        writer.String((std::string("write-and-read/") + wr.first).c_str());
        writer.EndObject();
      }
      writer.EndArray();

      writer.Key("granule");
      writer.Double(Ticker::single()->getTimeGranule());

      writer.EndObject();

      connection->send
        (doc.GetString(),
         [](const SimpleWeb::error_code &ec) {
           if (ec) {
             /* DUECA websockets.

                Unexpected error in sending the configuration
                information. */
             W_XTR("Error sending message " << ec);
           }
         });

      DEB("New connection on ^/configuration, sent data");

      // removed, closing at this point upsets some clients
      // const std::string reason("Configuration data sent");
      // connection->send_close(1000, reason);
    };
  configinfo.on_close =
    [this](shared_ptr<typename S::Connection> connection,
           int status, const std::string &reason) {
      /* DUECA websockets.

         Information on the closing of the connection of a client with
         the configuration URL. */
      I_XTR("Closing configuration endpoint " <<
            " code: " << status << " reason: \"" << reason << '"');
    };

  // access channel data on request; each message (no data needed)
  // is replied to with the current value read from the accessed channel
  // and entry
  auto &current = server.endpoint["^/current/([a-zA-Z0-9_-]+)$"];

  current.on_message =
    [this](shared_ptr<typename S::Connection> connection,
           shared_ptr<typename S::InMessage> in_message) {

      // find the channel access corresponding to the connection
      auto em = this->singlereadsmapped.find
        (reinterpret_cast<void*>(connection.get()));

      if (em == this->singlereadsmapped.end()) {
        /* DUECA websockets.

           Cannot find the connection entry for a message to the
           "current" URL. */
        E_XTR("Cannot find connection");
        const std::string reason("Server failure, cannot find connection data");
        connection->send_close(1001, reason);
        return;
      }

      // create the reader
      DCOReader r(em->second->datatype.c_str(), em->second->r_token);
      DataTimeSpec dtd = r.timeSpec();
      rapidjson::StringBuffer doc;
      rapidjson::Writer<rapidjson::StringBuffer> writer(doc);
      writer.StartObject();
      writer.Key("tick");
      writer.Uint(dtd.getValidityStart());
      writer.Key("data");
      if (extended) DCOtoJSONcompact(writer, r);
      else DCOtoJSONstrict(writer, r);
      writer.EndObject();

      connection->send
        (doc.GetString(),
         [](const SimpleWeb::error_code &ec) {
           if (ec) {
             /* DUECA websockets.

                Unexpected error in sending a message to a client for
                the "current" URL */
             W_XTR("Error sending message " << ec);
           }
         });
    };

  current.on_error =
    [](shared_ptr<typename S::Connection> connection,
           const SimpleWeb::error_code &ec) {

      /* DUECA websockets.

         Unexpected error in the "current" URL connection. */
      W_XTR("Error in connection " << connection.get() << ". " <<
            "Error: " << ec << ", error message: " << ec.message());
    };


  current.on_close =
    [this](shared_ptr<typename S::Connection> connection,
           int status, const std::string &reason) {

      auto qpars = SimpleWeb::QueryString::parse(connection->query_string);
      auto ekey = qpars.find("entry");
      /* DUECA websockets.

         Information on the closing of the connection of a client with
         a "current" URL. */
      I_XTR("Closing endpoint at /current/" <<
            connection->path_match[1] << "?entry=" << ekey->second <<
            " code: " << status << " reason: \"" << reason << '"');

      // find the mapped connection, and remove
      if (this->singlereadsmapped.erase
          (reinterpret_cast<void*>(connection.get()))) {
        // OK
      }
      else {
        /* DUECA websockets.

           Programming error? Cannot find the connection corresponding
           to a close attempt on a "current" URL.
        */
        W_XTR("Cannot find mapping for endpoint at /current/" <<
              connection->path_match[1] << "?entry=" << ekey->second);
      }
    };

  // open should be last?
  current.on_open =
    [this](shared_ptr<typename S::Connection> connection) {

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
            boost::shared_ptr<SingleEntryRead> newcur
              (new SingleEntryRead
               (mon->second->channelname, datatype, entry,
                this->getId()));

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

      // connect the connection to the located or created channel reader
      this->singlereadsmapped[reinterpret_cast<void*>(connection.get())] =
        ee != this->readsingles.end() ? ee->second : ea->second;
    };

  // access channel data to collect all data; messages are followed
  auto &follow = server.endpoint["^/read/([a-zA-Z0-9_-]+)$"];

  follow.on_error =
    [](shared_ptr<typename S::Connection> connection,
           const SimpleWeb::error_code &ec) {

      /* DUECA websockets.

         Unexpected error in the "follow" URL connection. */
      W_XTR("Error in connection " << connection.get() << ". " <<
            "Error: " << ec << ", error message: " << ec.message());
    };

  follow.on_close =
    [this](shared_ptr<typename S::Connection> connection,
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
          E_XTR("Trying to close connection, cannot find mapping at " <<
                "/read/" << connection->path_match[1]);
          return;
        }
      }
      if (ee->second->removeConnection(connection)) {
        /* DUECA websockets.

           Programming error? Cannot remove the connection corresponding
           to a close attempt on a "read" URL.
        */
        E_XTR("Closing connection, cannot remove connection at " <<
              "/read/" << connection->path_match[1]);
        return;
      }
    };

  follow.on_open =
    [this](shared_ptr<typename S::Connection> connection) {

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

      // figure out if this channel/entry is already being followed
      auto ee = this->followers.find(key);

      // if this is not the case, it might be configured
      if (ee == this->followers.end()) {


        // run through the monitors now
        auto mm = this->monitors.find(connection->path_match[1]);
        if (mm != this->monitors.end()) {

          // and this entry should exist
          auto dataclass = mm->second->findEntry(entry);
          if (dataclass.size()) {

            boost::shared_ptr<SingleEntryFollow>
              newfollow(new SingleEntryFollow
                        (mm->second->channelname, dataclass, entry,
                         this->getId(), this->read_prio,
                         mm->second->time_spec, extended, true));
            this->followers[key] = newfollow;
            ee = this->followers.find(key);
          }
        }
      }

      if (ee != this->followers.end()) {
        ee->second->addConnection(connection);
      }
      else {
        const std::string reason("Resource not available");
        connection->send_close(1001, reason);
      }
    };

  auto &monitor = server.endpoint["^/info/([a-zA-Z0-9_-]+)$"];

  monitor.on_error =
    [](shared_ptr<typename S::Connection> connection,
           const SimpleWeb::error_code &ec) {

      /* DUECA websockets.

         Unexpected error in an "info" URL connection. */
      W_XTR("Error in connection " << connection.get() << ". " <<
            "Error: " << ec << ", error message: " << ec.message());
    };

  monitor.on_close =
     [this](shared_ptr<typename S::Connection> connection,
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
         E_XTR("Closing connection, cannot find mapping at " <<
               "/info/" << connection->path_match[1]);
       }
       else {
         if (!ee->second->removeConnection(connection)) {
           /* DUECA websockets.

              Programming error? Cannot remove the connection corresponding
              to a close attempt on an "info" URL.
           */
           E_XTR("Closing connection, cannot find connection at " <<
                 "/info/" << connection->path_match[1]);
         }
       }
     };

  monitor.on_open =
    [this](shared_ptr<typename S::Connection> connection) {

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
  writer.on_error =
    [](shared_ptr<typename S::Connection> connection,
           const SimpleWeb::error_code &ec) {

      /* DUECA websockets.

         Unexpected error in a "write" URL connection. */
      W_XTR("Error in connection " << connection->path_match[0] << ". " <<
            "Error: " << ec << ", error message: " << ec.message());
    };

  writer.on_open =
    [this](shared_ptr<typename S::Connection> connection) {

      // try to find the setup
      // ScopeLock l(this->thelock);

      // just accept the connection.
      std::string key = connection->path_match[1];

      // check no entry is present on this connection
      auto ww = this->writers.find(reinterpret_cast<void*>(connection.get()));
      if (ww != this->writers.end()) {
        const std::string reason("Server logic error");
         connection->send_close(1007, reason);
        return;
      }

      // this is either defined in the writer setup (dynamic) or
      // in the presetwriters, never in both
      auto ee = this->writersetup.find(key);
      auto pre = this->presetwriters.find(key);
      assert (! ( ee != this->writersetup.end() &&
                  pre != this->presetwriters.end()) );

      // check that this URL is available
      if (ee == this->writersetup.end() &&
          pre == this->presetwriters.end()) {
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
            W_MOD("New connection for " << pre->second->identification <<
                  " forcing old connection to close");
            auto it = this->writers.find(pre->second->disConnect());
            if (it != this->writers.end()) {
              this->writers.erase(it);
            }
            else {
              /* DUECA websockets.

                 Trying to replace and remove an existing connection
                 to a preset write entry, but the old connection
                 cannot be found. */
              W_MOD("Could not find old connection to remove");
            }
          }
          pre->second->doConnect(connection);
          this->writers[reinterpret_cast<void*>(connection.get())] =
            pre->second;
          return;
        }
        else {
          const std::string reason("Resource already connected");
          connection->send_close(1001, reason);
          return;
        }
      }

      // create an empty write-entry
      this->writers[reinterpret_cast<void*>(connection.get())] =
        boost::intrusive_ptr<WriteEntry>
        (new WriteEntry(ee->second->channelname, ee->second->dataclass));
    };

  writer.on_message =
    [this](shared_ptr<typename S::Connection> connection,
           shared_ptr<typename S::InMessage> in_message) {

      // find the entry
      auto ww = this->writers.find(reinterpret_cast<void*>(connection.get()));

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
          } catch (const dataparseerror&) {
            const std::string reason("data coding error");
            connection->send_close(1007, reason);
            return;
          }
        }
      }
      else {
        try {
          ww->second->complete(in_message->string(), this->getId());
        } catch (const connectionparseerror&) {
          const std::string reason("connection start incorrect");
          connection->send_close(1007, reason);
          return;
        } catch (const presetmismatch&) {
          const std::string reason("connection start does not match preset");
          connection->send_close(1007, reason);
          return;
        }
      }
    };

  // close occurrence
  writer.on_close =
    [this](shared_ptr<typename S::Connection> connection,
           int status, const std::string &reason) {

      /* DUECA websockets.

         Information on the closing of the connection of a client with
         a "write" URL. */
      I_XTR("Closing endpoint at /write/" <<
            connection->path_match[1] <<
            " code: " << status << " reason: \"" << reason << '"');

      // find the entry
      auto ww = this->writers.find(reinterpret_cast<void*>(connection.get()));

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
        W_XTR("Cannot find mapping for endpoint at /write/" <<
              connection->path_match[1]);
      }
    };

  auto &writerreader = server.endpoint["^/write-and-read/([a-zA-Z0-9_-]+)$"];

  writerreader.on_error =
    [](shared_ptr<typename S::Connection> connection,
           const SimpleWeb::error_code &ec) {

      DEB("Error in write-and-read connection");
      /* DUECA websockets.

         Unexpected error in a "write-and-read" URL connection. */
      W_XTR("Error in connection " << connection->path_match[0] << ". " <<
            "Error: " << ec << ", error message: " << ec.message());
    };

  writerreader.on_open =
    [this](shared_ptr<typename S::Connection> connection) {

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
      this->writersreaders[reinterpret_cast<void*>(connection.get())] =
        boost::intrusive_ptr<WriteReadEntry>
        (new WriteReadEntry(ee->second, this, read_prio, extended));
      this->writersreaders[reinterpret_cast<void*>(connection.get())]->
        setConnection(connection);
    };

  writerreader.on_message =
    [this](shared_ptr<typename S::Connection> connection,
           shared_ptr<typename S::InMessage> in_message) {

      // find the entry
      auto ww = this->writersreaders.find(reinterpret_cast<void*>(connection.get()));

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
          } catch (const exception &e) {
            /* DUECA websockets.

               Error in attempting to read data from a "write-and-read" URL.
            */
            E_XTR("Exception trying to read from socket " << e.what() <<
                  " msg:" << in_message->string());
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
           W_XTR("/write-and-read/" << connection->path_match[1] <<
                " not yet complete");
        }
      }
      else {
        try {
          // call on the connection to complete itself
          ww->second->complete(in_message->string());
        } catch (const connectionparseerror&) {
          const std::string reason("connection start incorrect");
          connection->send_close(1007, reason);
          return;
        }
      }
    };

  // close occurrence
  writerreader.on_close =
    [this](shared_ptr<typename S::Connection> connection,
           int status, const std::string &reason) {

      // find the writing object, using the connection as index
      // ScopeLock l(this->thelock);
      auto wr = this->writersreaders.find
        (reinterpret_cast<void*>(connection.get()));

      if (wr == this->writersreaders.end()) {
        /* DUECA websockets.

           Programming error? Cannot find the connection corresponding
           to a close attempt on a "write-and-read" URL.
        */
        E_XTR("Closing connection, cannot find mapping at " <<
              "/write-and-read/" << connection->path_match[1]);
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

/** Local function for sending the response data, using 64K chunks */
template<typename R>
static void read_and_send(const R &response,
                          const std::shared_ptr<ifstream> &ifs) {
  // single-thread only
  static vector<char> buffer(0x10000);
  streamsize read_length;

  if ( (read_length = ifs->read
          (&buffer[0], static_cast<streamsize>(buffer.size())).gcount()) > 0) {
    response->write(&buffer[0], read_length);
    if(read_length == static_cast<streamsize>(buffer.size())) {
      response->send
        ([response, ifs](const SimpleWeb::error_code &ec) {
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

template<typename S>
bool WebSocketsServer::_complete_http(S& server)
{
  server.config.port = http_port;

  // create a generic URL
  server.default_resource["GET"] =
    [this](shared_ptr<typename S::Response> response,
           shared_ptr<typename S::Request> request) {

      try{
        auto web_root_path = boost::filesystem::canonical(this->document_root);
        auto path = boost::filesystem::canonical(web_root_path / request->path);

        DEB("http request for " << request->path);

        // Check if path is within document_root
        if(distance(web_root_path.begin(), web_root_path.end()) >
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
          response->write(header);
          read_and_send(response, ifs);
        }
      }
      catch(const exception &e) {
        response->write(SimpleWeb::StatusCode::client_error_bad_request,
                        "Could not open " + request->path + ": " + e.what());
        DEB("HTTP fails for " << request->path << ": " << e.what());
      }
    };

  server.on_error =
    [](shared_ptr<typename S::Request> request,
           const SimpleWeb::error_code &ec) {
      // note, error 125 is returned when a client pauses too much
      /* DUECA websockets.

         Unexpected error in the HTTP static file server. */
      E_MOD("Http server error code " << ec << " (" << ec.message() <<
            ") for request :" << request->path << ' ' << request->query_string);
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


bool WebSocketsServer::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */

  if (server_crt.size() && server_key.size()) {

    // ssl version
    sserver.reset(new WssServer(server_crt, server_key));
    if (!_complete(*sserver)) return false;

    // add the server run to the run context
    BOOST_POSTCALL
      (BOOST_POSTARG1
       [this]() {
         this->sserver->start();
       });

    if (document_root.size()) {
      https_server.reset(new HttpsServer(server_crt, server_key));
      if (!_complete_http(*https_server)) return false;

      // add the server run to the run context
      BOOST_POSTCALL
        (BOOST_POSTARG1
         [this]() {
           this->https_server->start();
         });
    }
    return true;
  }

  server.reset(new WsServer());
  if (!_complete(*server)) return false;
  BOOST_POSTCALL
    (BOOST_POSTARG1
     [this]() {
       this->server->start();
     });

  if (document_root.size()) {
    http_server.reset(new HttpServer);
    if (!_complete_http(*http_server)) return false;

    // add the server run to the run context
    BOOST_POSTCALL
      (BOOST_POSTARG1
       [this]() {
         this->http_server->start();
       });
  }

  return true;
}

// destructor
WebSocketsServer::~WebSocketsServer()
{
  if (immediate_start) {
    auto_started = false;
    TimeSpec now(SimTime::now());
    stopModule(now);
  }
}

// as an example, the setTimeSpec function
bool WebSocketsServer::setTimeSpec(const TimeSpec& ts)
{
  myclock.changePeriodAndOffset(ts);

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool WebSocketsServer::checkTiming(const vector<int>& i)
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

bool WebSocketsServer::setCurrentData(const std::vector<std::string>& def)
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
    catch (const std::exception& e) {
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
    boost::shared_ptr<SingleEntryRead> nentry
      (new SingleEntryRead(def[1], def[2], entryid, getId()));
    readsingles[key] = nentry;
  }
  catch (const std::exception& e) {
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


bool WebSocketsServer::setFollowData(const std::vector<std::string>& def)
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
    catch (const std::exception& e) {
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
    followread_t::mapped_type nentry
      (new followread_t::mapped_type::element_type
       (def[1], def[2], entryid, getId(), read_prio, time_spec, extended));
    followers[key] = nentry;
  }
  catch (const std::exception& e) {
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

bool WebSocketsServer::setChannelInfo(const std::vector<std::string>& def)
{
  // check size of input arguments
  if (def.size() != 2 ||
      def[0].size() == 0 || def[1].size() == 0) {
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
    boost::shared_ptr<ChannelMonitor> nentry
      (new ChannelMonitor(def[1], time_spec));
    monitors[def[0]] = nentry;
  }
  catch (const std::exception& e) {
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

bool WebSocketsServer::setWriterSetup(const std::vector<std::string>& def)
{
  // check size of input arguments
  if (def.size() < 2 ||
      def[0].size() == 0 || def[1].size() == 0) {
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
    boost::shared_ptr<WriteableSetup> nentry
      (new WriteableSetup(def[1], (def.size() > 2) ? def[2] : std::string("")));
    writersetup[def[0]] = nentry;
  }
  catch (const std::exception& e) {
    /* DUECA websockets.

       Cannot open the write entry for the requested "write" URL,
       check the set-up of your DUECA process.
    */
    E_CNF("Cannot create writer setup, exception " << e.what());
    return false;
  }
  return true;
}

bool WebSocketsServer::setPresetWriterSetup(const std::vector<std::string>& def)
{
  // check size of input arguments
  if (def.size() < 4 ||
      def[0].size() == 0 || def[1].size() == 0 || def[2].size() == 0) {
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
      } else if (def[idx] == "stream") {
        stream = true;
      } else if (def[idx] == "event") {
        stream = false;
      } else if (def[idx] == "bulk") {
        bulk = true;
      } else if (def[idx] == "diffpack") {
        diffpack = true;
      } else {
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
    boost::intrusive_ptr<PresetWriteEntry> nentry
      (new PresetWriteEntry(def[1], def[2], def[3], this->getId(),
                            ctiming, stream, bulk, diffpack));
    presetwriters[def[0]] = nentry;
  }
  catch (const std::exception& e) {
    /* DUECA websockets.

       Cannot open the write entry for the requested "write" URL,
       check the set-up of your DUECA process.
    */
    E_CNF("Cannot create preset writer, exception " << e.what());
    return false;
  }
  return true;
}

bool WebSocketsServer::setWriteReadSetup(const std::vector<std::string>& def)
{
  if (def.size() < 3 ||
      !def[0].size() || ! def[1].size() || !def[2].size()) {
    /* DUECA websockets.

       Configuration for a "write-and-read" URL is not complete. Check your
       configuration file.
    */
    E_CNF("Need URL name + 2 x channel name");
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
    boost::shared_ptr<WriteReadSetup> nentry
      (new WriteReadSetup(def[1], def[2]));

    // run through any additional arguments
    for (unsigned ii = 3; ii < def.size(); ii++) {
      if (def[ii] == "bulk") {
        nentry->bulk = true;
      } else if (def[ii] == "diffpack") {
        nentry->diffpack = true;
      } else {
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
  catch (const std::exception& e) {
    /* DUECA websockets.

       Cannot create the requested set-up for a write-and-read
       connection.
    */
    E_CNF("Cannot create write+read setup, exception " << e.what());
    return false;
  }

  return true;
}

bool WebSocketsServer::setCertFiles(const std::vector<std::string>& files)
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

// tell DUECA you are prepared
bool WebSocketsServer::isPrepared()
{
  bool res = true;

  for (auto &fl : followers) {
    res = res && fl.second->checkToken();
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
void WebSocketsServer::startModule(const TimeSpec &time)
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
void WebSocketsServer::stopModule(const TimeSpec &time)
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

void WebSocketsServer::doTransfer(const TimeSpec &ts)
{
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
