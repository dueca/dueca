/* ------------------------------------------------------------------   */
/*      item            : WebsockCommunicator.cxx
        made by         : Rene' van Paassen
        date            : 200405
        category        : body file
        description     :
        changes         : 200405 first version
        language        : C++
        copyright       : (c) 2020 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// boost::asio::ip::tcp::no_delay option(true);

#define WebsockCommunicator_cxx
#include "WebsockCommunicator.hxx"

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>

#include <dueca/debug.h>
#include <dassert.h>
#ifdef DEBDEF
#include <dueca/SimTime.hxx>
#endif
#include "NetCommunicator.hxx"
#include <dueca/AmorphStore.hxx>

#define DEBPRINTLEVEL 0
#include <debprint.h>


DUECA_NS_START;

// -----------------------------------------------------------------
WSConnectionData&
WSConnectionData::operator =
(std::shared_ptr<typename WsServer::Connection> connection)
{
  this->peer_id = 0;
  this->connection = connection;
  return *this;
}

// -----------------------------------------------------------------
WebsockCommunicatorConfig::
WebsockCommunicatorConfig(const std::string& url,
                          double timeout,
                          callback_type assign_peer_id,
                          size_t buffer_size,
                          unsigned nbuffers) :
  url(url),
  timeout(int(round(1000000*timeout))),
  runcontext(new boost::asio::io_context),
  timer(*runcontext),
  server(new WsServer),
  peers(),
  incoming(10, "websock config"),
  assign_peer_id(assign_peer_id),
  messagebuffers(nbuffers, "Config spare message buffers"),
  buffer_size(buffer_size)
{
  for (int ii = nbuffers; ii--; ) {
    returnBuffer(new MessageBuffer(buffer_size));
  }

  // decode the url, e.g. "udp://myhost.mynet:8432"
  if (url.substr(0, 5) != "ws://") {
    throw CFErrorConstruction("cannot create, URL incorrect");
  }
  std::string dataport("8000");
  std::string epstring("");
  std::string hostip;
  try {
    size_t colon = url.find(":", 5);
    size_t slash = url.find("/", 5);

    if (colon != string::npos) {
      // assuming that this can be interpreted as a port number
      dataport = url.substr(colon+1, slash-colon-1);
      hostip = url.substr(5, colon - 5);
    }
    else {
      hostip = url.substr(5, slash - 5);
    }

    // endpoint string
    epstring = url.substr(slash);

    // get the address
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(hostip, dataport);
    boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

    // set the server config
    server->config.address = boost::lexical_cast<std::string>
      (iter->endpoint().address());
    server->config.port = iter->endpoint().port();
  }
  catch (const std::exception &e) {
    /* DUECA network.

       When configuring a websocket connection, it was not possible to
       determine the configuration from the given URL. Correct your
       config files. */
    E_CNF("WebsockComm cannot decode & configure with " << url);
  }

  // create the endpoint
  auto &endpoint = server->endpoint[std::string("^") + epstring];

  endpoint.on_open =
    [this](shared_ptr<typename S::Connection> connection) {

      // store new connection and set temporary ID
      auto cconn = peers.find(reinterpret_cast<void*>(connection.get()));
      if (cconn != peers.end()) {
        const std::string reason("Server failure, connection already exists");
        /* DUECA network.

           Could not open the server endpoint, because the connection
           already exists. Check which other processes might be using
           this connection. */
        E_NET(reason)
          connection->send_close(1001, reason);
        return;
      }

      // remember peer by connection pointer. ID still zero, means not
      // active/initialised
      peers[reinterpret_cast<void*>(connection.get())] = connection;
      peers[reinterpret_cast<void*>(connection.get())].peer_id =
        (*(this->assign_peer_id))
        (connection->remote_endpoint().address().to_string());

      DEB("Websocket config new configuration connection " << peers.size());
    };

  endpoint.on_error =
    [this](shared_ptr<typename S::Connection> connection,
           const SimpleWeb::error_code &ec) {
      /* DUECA network.

         Unforeseen websocket server error on the configuration
         master. See the message.
      */
      W_NET("Websocket server error " << ec << ", message: " << ec.message());

      auto cconn = peers.find(reinterpret_cast<void*>(connection.get()));
      if (cconn == peers.end()) {
        /* DUECA network.

           When trying to find the cause of an unforeseen websocket
           server error, the connection corresponding to this message
           could not be found.
        */
        E_NET("Cannot find error connection");
      }
      else {
        // flag the connection as invalid
        cconn->second.connection.reset();

        // and make sure the user knows
        MessageBuffer::ptr_type buffer = getBuffer();
        buffer->fill = 0;
        buffer->origin = cconn->second.peer_id;

        // write the empty buffer on incoming
        AsyncQueueWriter<MessageBuffer::ptr_type> w(incoming);
        w.data() = buffer;

        // remove the entry
        peers.erase(cconn);
      }
    };

  endpoint.on_close =
    [this](shared_ptr<typename S::Connection> connection,
           int status, const string& reason) {
      /* DUECA network.

         A client is closing, the status message and reason are given. */
      W_NET("Websocket client closing status " << status <<
            ", reason: " << reason);

      auto cconn = peers.find(reinterpret_cast<void*>(connection.get()));
      if (cconn == peers.end()) {
        /* DUECA network.

           When trying to find the client causing a close event, the
           corresponding connection could not be found.
        */
        E_NET("Cannot find peer connection for closing");
      }
      else {
        MessageBuffer::ptr_type buffer = getBuffer();

        // and make sure the user knows
        buffer->fill = 0;
        buffer->origin = cconn->second.peer_id;

        // write the empty buffer on incoming
        AsyncQueueWriter<MessageBuffer::ptr_type> w(incoming);
        w.data() = buffer;

        // remove the entry
        peers.erase(cconn);
      }
    };

  endpoint.on_message =
    [this](std::shared_ptr<typename S::Connection> connection,
           std::shared_ptr<typename S::InMessage> in_message) {

      auto cconn = peers.find(reinterpret_cast<void*>(connection.get()));
      if (cconn == peers.end()) {
        /* DUECA network.

           Received a message, but cannot find the peer that
           corresponds to this message in the set of connections.
        */
        E_NET("Server cannot find peer among connections");
        return;
      }

      MessageBuffer::ptr_type buffer = getBuffer();

      // read the data from the in_message into the buffer
      in_message->read(buffer->buffer, buffer->capacity);
      buffer->fill = in_message->gcount();
      buffer->origin = cconn->second.peer_id;
      AsyncQueueWriter<MessageBuffer::ptr_type> w(incoming);
      w.data() = buffer;
    };

  // specify my own io_service
  server->io_service = runcontext;

  // add the server run to the run context
#ifdef BOOST1_65
  runcontext->post(
#else
  boost::asio::post
    (*runcontext,
#endif
     [this]() {
       this->server->start();
     });
}

MessageBuffer::ptr_type WebsockCommunicatorConfig::getBuffer()
{
  MessageBuffer::ptr_type buf;
  if (messagebuffers.notEmpty()) {
    AsyncQueueReader<MessageBuffer::ptr_type> r(messagebuffers);
    buf = r.data();
  }
  else {
    DEB("Creating extra message buffer for config");
    buf = new MessageBuffer(buffer_size);
  }
  buf->nusers = 1;
  return buf;
}

void WebsockCommunicatorConfig::returnBuffer(MessageBuffer::ptr_type buffer)
{
  assert(buffer->nusers);
  if (buffer->release()) {
    AsyncQueueWriter<MessageBuffer::ptr_type> w(messagebuffers);
    w.data() = buffer;
  }
}

WebsockCommunicatorConfig::~WebsockCommunicatorConfig()
{

}

void WebsockCommunicatorConfig::
timerCallback(const boost::system::error_code& e)
{
#ifdef DEBDEF
  if (e != boost::asio::error::operation_aborted) {
    DEB1("Websocket reached timeout");
  }
#endif
  runcontext->stop();
}

void WebsockCommunicatorConfig::sendConfig(const AmorphStore& s,
					   unsigned peer_id)
{
  for (auto &p: peers) {
    if (p.second.connection && p.second.peer_id == int(peer_id)) {
      DEB2("Websock master config sending to " << peer_id << " size " << s.getSize());
      std::shared_ptr<S::OutMessage> outmessage
        (new S::OutMessage(s.getSize()));
      outmessage->write(s.getToData(), s.getSize());
      outmessage->flush();
      p.second.connection->send(outmessage);
    }
  }
}

void WebsockCommunicatorConfig::sendConfig(const AmorphStore& s)
{
  DEB2("Websock master config sending to all " << s.getSize());
  for (auto &p: peers) {
    if (p.second.connection) {
      std::shared_ptr<S::OutMessage> outmessage
        (new S::OutMessage(s.getSize()));
      outmessage->write(s.getToData(), s.getSize());
      outmessage->flush();
      p.second.connection->send(outmessage);
    }
  }
}



MessageBuffer::ptr_type WebsockCommunicatorConfig::receiveConfig(bool block)
{
  if (!incoming.notEmpty()) {
    try {
      if (block) {
        timer.expires_after(timeout);
        timer.async_wait(boost::bind(&WebsockCommunicatorConfig::timerCallback,
                                     this, _1));
        runcontext->run();
#ifdef BOOST1_65
        runcontext->reset();
#else
        runcontext->restart();
#endif
      }
      else {
        // do poll while work is available or until data has been found
        int nrun = runcontext->poll_one();
        while (nrun && !incoming.notEmpty()) {
          nrun = runcontext->poll_one();
        }
      }
    }
    catch (const std::exception& e) {
      /* DUECA network.

         When trying to receive and process configuration messages and
         connections, an unexpected error occurred.
      */
      E_NET("Config websocket receive run exception " << e.what());
    }
  }

  if (incoming.notEmpty()) {
    AsyncQueueReader<MessageBuffer::ptr_type> r(incoming);
    return r.data();
  }
  return NULL;
}

WebsockCommunicatorMaster::
WebsockCommunicatorMaster(const PacketCommunicatorSpecification& spec) :
  PacketCommunicator(spec),
  config(),
  timeout(int(round(1000000*spec.timeout))),
  peers(),
  incoming(10, "Websocket master IO incoming"),
  url(spec.url)
{

}

WebsockCommunicatorMaster::~WebsockCommunicatorMaster()
{
  //
}

void WebsockCommunicatorMaster::
attachToMaster(std::shared_ptr<WebsockCommunicatorConfig> config)
{
  this->config = config;

  // decode the url, e.g. "udp://myhost.mynet:8432"
  if (url.substr(0, 5) != "ws://") {
    throw CFErrorConstruction("cannot create, URL incorrect");
  }
  std::string epstring("");
  size_t slash = 0;
  try {
    slash = url.find("/", 5);
    epstring = url.substr(slash);
  }
  catch (const std::exception &e) {
    /* DUECA network.

       When configuring a websocket connection, it was not possible to
       determine the configuration from the given URL. Correct your
       config files. */
    E_CNF("WebsockComm cannot decode & configure with " << url);
    return;
  }
  if ((config->url.substr(0, slash) != url.substr(0, slash)) ||
      (config->url == url)) {
    /* DUECA network.

       You are attempting to use websockets for configuration
       communication and for data communication. In that case the host
       name and port should be identical, and the only difference
       should be the URL endpoint. Correct your config files. 
    */
    E_NET("Configuration URL and data URL should only differ in endpoint!"
          << config->url << " " << url);
    return;
  }

  // add the endpoint to the config server
  auto &endpoint = config->server->endpoint[std::string("^") + epstring];
  DEB("Attaching to master, endpoint ^" << epstring);

  endpoint.on_open =
    [this](shared_ptr<typename S::Connection> connection) {

      // store new connection and set temporary ID
      auto cconn = peers.find(reinterpret_cast<void*>(connection.get()));
      if (cconn != peers.end()) {
        const std::string reason("Server failure, connection already exists");
        /* DUECA network.

           Could not open the server endpoint, because the connection
           already exists. Check which other processes might be using
           this connection. */
        E_NET(reason)
        connection->send_close(1001, reason);
        return;
      }

      // remember peer by connection pointer. ID still zero, means not
      // active/initialised
      peers[reinterpret_cast<void*>(connection.get())] = connection;

      DEB("Websocket master new connection " << peers.size());
    };

  endpoint.on_error =
    [this](shared_ptr<typename S::Connection> connection,
           const SimpleWeb::error_code &ec) {
      /* DUECA network.

         Unforeseen websocket server error on the data master. See the
         message.
      */
      W_NET("Websocket master error " << ec << ", message: " << ec.message());

      auto cconn = peers.find(reinterpret_cast<void*>(connection.get()));
      if (cconn == peers.end()) {
        /* DUECA network.

           When trying to find the cause of an unforeseen websocket
           server error, the connection corresponding to this message
           could not be found.
        */
        E_NET("Cannot find error connection");
      }
      else {
        // flag the connection as invalid
        //cconn->second.connection.reset();
        peers.erase(cconn);
      }
    };

  endpoint.on_close =
    [this](shared_ptr<typename S::Connection> connection,
           int status, const string& reason) {
      /* DUECA network.

         A client is closing, the status message and reason are given. */
      W_NET("Websocket client closing status " << status <<
            ", reason: " << reason);

      auto cconn = peers.find(reinterpret_cast<void*>(connection.get()));
      if (cconn == peers.end()) {
        /* DUECA network.

           When trying to find the client causing a close event, the
           corresponding connection could not be found.
        */
        E_NET("Cannot peer connection for closing");
      }
      else {
        peers.erase(cconn);
      }
    };

  endpoint.on_message =
    [this](std::shared_ptr<typename S::Connection> connection,
           std::shared_ptr<typename S::InMessage> in_message) {

      auto cconn = peers.find(reinterpret_cast<void*>(connection.get()));
      if (cconn == peers.end()) {
        /* DUECA network.

           Received a message, but cannot find the peer that
           corresponds to this message in the set of connections.
        */
        E_NET("Server cannot find peer among connections");
        return;
      }

      MessageBuffer::ptr_type buffer = getBuffer();

      // read the data from the in_message into the buffer
      in_message->read(buffer->buffer, buffer->capacity);
      buffer->fill = latest_received = in_message->gcount();

      if (latest_received >= ssize_t(NetCommunicator::control_size)) {

        if (cconn->second.peer_id == 0) {

          // take peer ID from first message
          int i_peer_id = NetCommunicator::ControlBlockReader::
            decodePeerId(buffer);
          cconn->second.peer_id = i_peer_id;
          DEB("Websock found id " << i_peer_id << " for new peer");
        }
        buffer->origin = cconn->second.peer_id;

#ifdef DEBDEF
        NetCommunicator::ControlBlockReader i_(buffer);
        DEB2("Received cycle=" << i_.cycle << " peer="
            << i_.peer_id << " err="
            << i_.errorflag << " npeers="
            << i_.npeers << " tick=" << i_.peertick
            << " size=" << latest_received << " crc=" << i_.crcvalue
            << " ok=" << i_.crcgood);
#endif

        // also replicate to all other peers, simulating broadcast
        for (auto p: peers) {
          if (reinterpret_cast<void*>(connection.get()) != p.first) {
            std::shared_ptr<S::OutMessage> outmessage
              (new S::OutMessage(buffer->fill));
            // I think this is needed TOCHECK
            outmessage->write(buffer->buffer, buffer->fill);
            outmessage->flush();
            p.second.connection->send(outmessage);
          }
        }

        // callback to return data to the client
        if (pass_data) {

          // cancels the timer
          this->config->timer.cancel();

          DEB2("Node " << peer_id << " receiving size " << buffer->fill <<
              " from " << cconn->second.peer_id);
          AsyncQueueWriter<MessageBuffer::ptr_type> w(incoming);
          w.data() = buffer;
          return;
        }
      }
      else {
        DEB("Not enough data received " << buffer->fill);
      }

      // clear up
      returnBuffer(buffer);
      pass_data = true;
    };
}


void WebsockCommunicatorMaster::send(MessageBuffer::ptr_type buffer)
{
#ifdef DEBDEF
  assert (buffer->fill >= NetCommunicator::control_size);
  NetCommunicator::ControlBlockReader i_(buffer);
  DEB2("Send cycle=" << i_.cycle << " peer="
      << i_.peer_id << " err="
      << i_.errorflag << " npeers="
      << i_.npeers << " tick=" << i_.peertick
      << " size=" << buffer->fill << " crc=" << i_.crcvalue);
#endif
  for (auto &p: peers) {
    if (p.second.connection) {
      std::shared_ptr<S::OutMessage> outmessage
        (new S::OutMessage(buffer->fill));
      outmessage->write(buffer->buffer, buffer->fill);
      outmessage->flush();
      p.second.connection->send(outmessage);
    }
  }
}

std::pair<int,ssize_t> WebsockCommunicatorMaster::receive()
{
  // intrusive pointer to self, ensure that a delete of the master does not
  // interfere with this call
  boost::intrusive_ptr<WebsockCommunicatorMaster> keep(this);

  if (!incoming.notEmpty()) {
    try {

      config->timer.expires_after(timeout);
      config->timer.async_wait
        (boost::bind(&WebsockCommunicatorConfig::timerCallback,
                     config.get(), _1));
      config->runcontext->run();

      // restart for continuing
#ifdef BOOST1_65
      config->runcontext->reset();
#else
      config->runcontext->restart();
#endif
    }
    catch (const std::exception& e) {
      /* DUECA network.

         When trying to receive and process data messages and
         connections, an unexpected error occurred.
      */
      E_NET("Websocket receive run exception " << e.what());
      return std::make_pair(-1, ssize_t(0));
    }
  }
  if (incoming.notEmpty()) {
    AsyncQueueReader<MessageBuffer::ptr_type> r(incoming);
    MessageBuffer::ptr_type buffer = r.data();
    auto result = std::make_pair(buffer->origin, ssize_t(buffer->fill));
    (*callback)(buffer);
    return result;
  }
  return std::make_pair(-1, ssize_t(0));
}

// ----------------------------------------------------------------

WebsockCommunicatorPeer::
WebsockCommunicatorPeer(const PacketCommunicatorSpecification& spec,
                        bool allinit) :
  PacketCommunicator(spec),
  runcontext(new boost::asio::io_context),
  timer(*runcontext),
  timeout(int(round(1000000*spec.timeout))),
  client(new S(spec.url.substr(5))),
  connection()
{
  client->on_open =
    [this,allinit,spec](shared_ptr<typename S::Connection> connection) {
      this->is_operational = true;
      this->connection = connection;
#ifdef DEBDEF
      if (allinit) {
        DEB("Peer socket opened to " << spec.url);
      }
      else {
        DEB("Peer config socket opened " << spec.url);
      }
#endif
    };

  client->on_error =
    [this](shared_ptr<S::Connection> connection,
       const SimpleWeb::error_code &ec) {
      /* DUECA network.

         Unexpected error in peer client communication.
      */
      W_NET("Websocket client error " << ec << ", message: " << ec.message());
      this->is_operational = false;
    };

  client->on_close =
    [this](shared_ptr<S::Connection> connection,
       int status, const string& reason) {
      /* DUECA network.

         A client is closing, the status message and reason are given. */
      W_NET("Websocket closing status " << status << ", reason: " << reason);
      this->is_operational = false;
    };

  if (allinit) {
    client->on_message =
      [this](shared_ptr<S::Connection> connection,
             shared_ptr<S::InMessage> in_message) {

        // access a new buffer for receiving the data
        MessageBuffer::ptr_type buffer = getBuffer();

        // copy the data, saves message size
        in_message->read(buffer->buffer, buffer->capacity);
        buffer->fill = in_message->gcount();

        // take the origin from the data; master might re-send other
        // participants messages, or send its own
        if (buffer->fill >= NetCommunicator::control_size) {

          buffer->origin =
            NetCommunicator::ControlBlockReader::decodePeerId(buffer);

#ifdef DEBDEF
          NetCommunicator::ControlBlockReader i_(buffer);
          DEB2("Received cycle=" << i_.cycle << " peer="
              << i_.peer_id << " err="
              << i_.errorflag << " npeers="
              << i_.npeers << " tick=" << i_.peertick
              << " size=" << latest_received << " crc=" << i_.crcvalue
              << " ok=" << i_.crcgood);
#endif
          if (pass_data) {

            // cancels the timer & stops the context
            timer.cancel();

            DEB2("Node " << peer_id << " receiving size " << buffer->fill <<
                 " from " << buffer->origin);

            AsyncQueueWriter<MessageBuffer::ptr_type> w(incoming);
            w.data() = buffer;
            return;
          }
          else {
          }
        }
        else {
          /* DUECA network.

             A data message was received from a peer/client, but the
             message size is too small to be valid. Caused by spurious
             communication, incompatible DUECA versions, or a DUECA
             programming error.
          */
          W_NET("Received message too small " << buffer->fill << "/"
                << NetCommunicator::control_size);
        }

        // clear up in case of little or no data
        returnBuffer(buffer);
        pass_data = true;
      };
  }

  // specify my own io_service
  client->io_service = runcontext;

  if (allinit) {
    // add the client run to the run context
#ifdef BOOST1_65
    runcontext->post(
#else
    boost::asio::post
      (*runcontext,
#endif
       [this]() {
         this->client->start();
       });
  }
}

WebsockCommunicatorPeer::~WebsockCommunicatorPeer()
{ }

std::pair<int,ssize_t> WebsockCommunicatorPeer::receive()
{
  if (!incoming.notEmpty()) {
    try {

      timer.expires_after(timeout);
      timer.async_wait
        (boost::bind(&WebsockCommunicatorPeer::timerCallback, this, _1));
      runcontext->run();

      // restart for continuing
#ifdef BOOST1_65
      runcontext->reset();
#else
      runcontext->restart();
#endif
    }
    catch (const std::exception& e) {
      /* DUECA network.

         Unexpected error in peer receiving messages. */
      E_NET("Websocket receive run exception " << e.what());
      is_operational = false;
      return std::make_pair(-1, ssize_t(0));
    }
  }
  if (incoming.notEmpty()) {
    AsyncQueueReader<MessageBuffer::ptr_type> r(incoming);
    MessageBuffer::ptr_type buffer = r.data();
    auto result = std::make_pair(buffer->origin, ssize_t(buffer->fill));
    (*callback)(buffer);
    return result;
  }
  return std::make_pair(-1, ssize_t(0));
}

void WebsockCommunicatorPeer::
timerCallback(const boost::system::error_code& e)
{
#ifdef DEBDEF
  if (e != boost::asio::error::operation_aborted) {
    DEB1("Websocket reached timeout");
  }
#endif
  runcontext->stop();
}

void WebsockCommunicatorPeer::send(MessageBuffer::ptr_type buffer)
{
  if (is_operational) {
    try {
      std::shared_ptr<S::OutMessage> outmessage
        (new S::OutMessage(buffer->fill));
#ifdef DEBDEF
      assert(buffer->fill >= NetCommunicator::control_size);
      NetCommunicator::ControlBlockReader i_(buffer);
      DEB2("Send cycle=" << i_.cycle << " peer="
           << i_.peer_id << " err="
           << i_.errorflag << " npeers="
           << i_.npeers << " tick=" << i_.peertick
           << " size=" << latest_received << " crc=" << i_.crcvalue);
#endif
      outmessage->write(buffer->buffer, buffer->fill);
      outmessage->flush();
      DEB2("Websock client sending, size " << buffer->fill);
      connection->send(outmessage);
    }
    catch (const std::exception& e) {
      /* DUECA network.

         Unexpected error in trying to send a data message to the
         master. */
      E_NET("Websocket send " << e.what());
      is_operational = false;
    }
  }
  else {
    DEB("Websock client not yet operational, ignoring size " << buffer->fill);
  }
}

WebsockCommunicatorPeerConfig::
WebsockCommunicatorPeerConfig(const PacketCommunicatorSpecification& spec) :
  WebsockCommunicatorPeer(spec, false)
{
  // only specify the on_message part, different for config
  client->on_message =
    [this](shared_ptr<S::Connection> connection,
           shared_ptr<S::InMessage> in_message) {

      // access a new buffer for receiving the data
      MessageBuffer::ptr_type buffer = getBuffer();

      // copy the data, saves message size
      in_message->read(buffer->buffer, buffer->capacity);
      buffer->fill = latest_received = in_message->gcount();

      // take the origin from the data; master might re-send other
      // participants messages, or send its own
      if (latest_received) {

        // cancels the timer & stops the context
        timer.cancel();

        buffer->origin = 0;
        DEB2("Node " << peer_id << " receiving config size " << buffer->fill);
        AsyncQueueWriter<MessageBuffer::ptr_type> w(incoming);
        w.data() = buffer;
        return;
      }

      // clear up in case of little or no data
      latest_received = 0;
      returnBuffer(buffer);
    };

  // add the client run to the run context
#ifdef BOOST1_65
  runcontext->post(
#else
  boost::asio::post
    (*runcontext,
#endif
     [this]() {
       this->client->start();
     });
}

WebsockCommunicatorPeerConfig::~WebsockCommunicatorPeerConfig()
{ }

void WebsockCommunicatorPeerConfig::sendConfig(const AmorphStore& s)
{
  if (is_operational) {
    try {
      std::shared_ptr<S::OutMessage> outmessage(new S::OutMessage(s.getSize()));
      outmessage->write(s.getToData(), s.getSize());
      outmessage->flush();
      DEB2("Websock client config sending, size " << s.getSize());
      connection->send(outmessage);
    }
    catch (const std::exception& e) {
      /* DUECA network.

         Unexpected error in sending a configuration message to the
         master.
      */
      E_NET("Websocket peer config send run exception " << e.what());
      is_operational = false;
    }
  }
  else {
    DEB("Websock client not yet operational, ignoring size " << s.getSize());
  }
}


ssize_t WebsockCommunicatorPeerConfig::checkup()
{
  /* Receive is entered non-blocking.

     Approach:

     - poll non-blocking, but returning when data has been received

     - if no data was received, run with a timeout, then return with
       either no data (real timeout, or no peers yet), or with the data
  */

  DEB2("Websock trying receive");
  latest_received = 0;

  try {
    int nrun = 1;
    while (!incoming.notEmpty() && nrun) {
      nrun = runcontext->poll_one();
    }
  }
  catch (const std::exception& e) {
    /* DUECA network.

       Unexpected error in processing connections and data by the
       peer in a check-up cycle.
    */
    E_NET("Websocket receive run exception " << e.what());

  }
  if (incoming.notEmpty()) {
    AsyncQueueReader<MessageBuffer::ptr_type> r(incoming);
    MessageBuffer::ptr_type buffer = r.data();
    auto result = ssize_t(buffer->fill);
    (*callback)(buffer);
    return result;
  }

  return ssize_t(0);
}

bool WebsockCommunicatorPeer::isOperational()
{
  try {

    timer.expires_after(timeout);
    timer.async_wait
      (boost::bind(&WebsockCommunicatorPeer::timerCallback, this, _1));
    runcontext->run();

  }
  catch (const std::exception& e) {
    /* DUECA network.

       Unexpected error in processing connections and data by the
       peer for the initial set-up.
    */
    E_NET("Websocket receive run exception " << e.what());
    is_operational = false;
  }

  // restart run context for later
#ifdef BOOST1_65
  runcontext->reset();
#else
  runcontext->restart();
#endif

  return is_operational;
}


DUECA_NS_END;
