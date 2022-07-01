/* ------------------------------------------------------------------   */
/*      item            : WebsockCommunicator.hxx
        made by         : Rene van Paassen
        date            : 200405
        category        : header file
        description     :
        changes         : 200405 first version
        language        : C++
        copyright       : (c) 2020 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef WebsockCommunicator_hxx
#define WebsockCommunicator_hxx

#include <map>
#include <chrono>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION < 106600
namespace boost { namespace asio {
    typedef io_service io_context;
  } }
#define BOOST1_65
#define expires_after expires_from_now
#endif

#include <simple-websocket-server/server_ws.hpp>
#include <simple-websocket-server/client_ws.hpp>

#include <dueca/MessageBuffer.hxx>
#include <dueca/SharedPtrTemplates.hxx>
#include <dueca/AsyncQueueMT.hxx>

#include "PacketCommunicator.hxx"

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
//using WssServer = SimpleWeb::SocketServer<SimpleWeb::WSS>;
//using WssClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;

DUECA_NS_START;

class AmorphStore;

/** Helper object to link connection pointer to peer

 */
struct WSConnectionData {
  /** Assigned peer ID */
  int peer_id;

  /** The connection object */
  std::shared_ptr<typename WsServer::Connection> connection;

  /** Assign connectiondata object */
  WSConnectionData& operator =
  (std::shared_ptr<typename WsServer::Connection> connection);
};


/** Packet communication over websocket connection, configuration
    connection.

    Configuration for network communication, uses websocket protocol.
 */
class WebsockCommunicatorConfig
{
  friend class WebsockCommunicatorMaster;

  /** Type for the callback function to assign peer id's */
  typedef CommonCallbackBase<int,const std::string>::smart_ptr_type
  callback_type;

  // typedef for derive class support
  typedef WsServer                          S;

  // URL
  std::string                               url;

  // client or server read response timeout
  std::chrono::duration<long,std::micro>    timeout;

  /** IO context to perform a ready run */
  std::shared_ptr<boost::asio::io_context>  runcontext;

  /** Boost steady timer */
  boost::asio::steady_timer                 timer;

  /** Server, uncoded */
  boost::scoped_ptr<S>                      server;

  /** Type for connected client mapping */
  typedef std::map<void*,WSConnectionData>  allpeers_type;

  /** Connected clients */
  allpeers_type                             peers;

  /** Async list for my messages */
  AsyncQueueMT<MessageBuffer::ptr_type>     incoming;

  /** Callback for getting Peer ID */
  callback_type                             assign_peer_id;

  /** Free storage for received messages */
  AsyncQueueMT<MessageBuffer::ptr_type>     messagebuffers;

  /** Buffers */
  size_t                                    buffer_size;

  /** Callback for timer expiry */
  void timerCallback(const boost::system::error_code&);

  /** Obtain a new or recycled buffer */
  MessageBuffer::ptr_type getBuffer();

public:

  /** Constructor */
  WebsockCommunicatorConfig(const std::string& url,
                            double timeout,
                            callback_type assignPeerId,
                            size_t buffer_size,
                            unsigned nbuffers);

  /** Destructor */
  ~WebsockCommunicatorConfig();

  /** Code and send new data, or re-send a previous message after
      failure */
  void sendConfig(const AmorphStore& s, unsigned peer_id);

  /** Code and send data, to all peers */
  void sendConfig(const AmorphStore& s);

  /** Blocking or non-Blocking receive data

      @param block     If true, do blocking with the configured timeout time
      @returns Buffer, or NULL pointer */
  MessageBuffer::ptr_type receiveConfig(bool block);

  /** Return one of the message buffers */
  void returnBuffer(MessageBuffer::ptr_type buffer);
};

/** Sending master for data communication over Websocket.

    This functions as a back-end to various communication devices,
    e.g. inter and net communication, and implements PacketCommunicator
    capabilities.
    WebSocket protocol.
 */

class WebsockCommunicatorMaster: public PacketCommunicator
{
  // typedef for derive class support
  typedef WsServer                           S;

  /** The connection infrastructure is borrowed from the websocket
      connection that does configuration messages */
  std::shared_ptr<WebsockCommunicatorConfig> config;

  // client or server read response timeout
  std::chrono::duration<long,std::micro>     timeout;

  /** Type for connected client mapping */
  typedef std::map<void*,WSConnectionData>   allpeers_type;

  /** Connected clients */
  allpeers_type                              peers;

  /** Async list for my messages */
  AsyncQueueMT<MessageBuffer::ptr_type>      incoming;

  // URL
  std::string                                url;

public:
  /** Constructor */
  WebsockCommunicatorMaster(const PacketCommunicatorSpecification& spec);

  /** Destructor */
  ~WebsockCommunicatorMaster();

  /** Attach to master */
  void attachToMaster(std::shared_ptr<WebsockCommunicatorConfig>
                      master);

  /** Code and send new data, or re-send a previous message after
      failure */
  void send(MessageBuffer::ptr_type buffer) final;

  /** Blocking receive data

      @returns Number of bytes received */
  std::pair<int,ssize_t> receive() final;
};

/** Packet communication over websocket connection, peer

    This functions as a back-end to various communication devices,
    e.g. inter and net communication. Different PacketCommunicator
    types are available. This one implements communication over the
    WebSocket protocol.
 */
class WebsockCommunicatorPeer: public PacketCommunicator
{
protected:
  /** IO context to perform a ready run */
  std::shared_ptr<boost::asio::io_context>  runcontext;

  /** Boost steady timer */
  boost::asio::steady_timer                 timer;

  // client or server read response timeout
  std::chrono::duration<long,std::micro>    timeout;

  // typedef for derive class support
  typedef WsClient               S;

  /** Client connection to server, uncoded */
  boost::scoped_ptr<S>           client;

  /** Connection */
  std::shared_ptr<S::Connection> connection;

  /** Async list for my messages */
  AsyncQueueMT<MessageBuffer::ptr_type>     incoming;

  /** Callback for timer expiry */
  void timerCallback(const boost::system::error_code&);

public:
  /** Constructor */
  WebsockCommunicatorPeer(const PacketCommunicatorSpecification& spec,
                          bool allinit=true);

  /** Destructor */
  ~WebsockCommunicatorPeer();

  /** Code and send new data */
  void send(MessageBuffer::ptr_type buffer) final;

  /** Blocking receive data

      @returns Number of bytes received */
  std::pair<int,ssize_t> receive() final;

  /** Check operational */
  bool isOperational() override;
};


/** Packet communication over websocket connection, peer

    This functions as a back-end to various communication devices,
    e.g. inter and net communication. Different PacketCommunicator
    types are available. This one implements communication over the
    WebSocket protocol.
 */
class WebsockCommunicatorPeerConfig: public WebsockCommunicatorPeer
{
public:
  /** Constructor */
  WebsockCommunicatorPeerConfig(const PacketCommunicatorSpecification& spec);

  /** Destructor */
  ~WebsockCommunicatorPeerConfig();

  /** Code and send new data */
  void sendConfig(const AmorphStore& s);

  /** Non-blocking receive.

      @returns Number of bytes received */
  ssize_t checkup();
};


DUECA_NS_END;

#endif
