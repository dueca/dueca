/* ------------------------------------------------------------------   */
/*      item            : CommonChannelServer.hxx
        made by         : Rene van Paassen
        date            : 181127
        category        : header file
        description     :
        changes         : 181127 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommonChannelServer_hxx
#define CommonChannelServer_hxx

#include "Activity.hxx"
#include "PrioritySpec.hxx"
#include "WebsockExceptions.hxx"
#include <boost/intrusive_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <dueca/ChannelWatcher.hxx>
#include <dueca/CommObjectReader.hxx>
#include <dueca/DCOtoJSON.hxx>
#include <dueca/SharedPtrTemplates.hxx>
#include <dueca/StateGuard.hxx>
#include <dueca/TriggerRegulatorGreedy.hxx>
#include <dueca/dueca.h>
#include <map>
#include <memory>
#include <simple-websocket-server/server_ws.hpp>
#include <simple-websocket-server/server_wss.hpp>
#include <string>

DUECA_NS_START;
WEBSOCK_NS_START;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WssServer = SimpleWeb::SocketServer<SimpleWeb::WSS>;

class WebSocketsServerBase;
template <typename Encoder, typename Decoder> class WebSocketsServer;

/** Base class for maintaining a set of websocket connections to send
    data to.

    Approach for making websocket connections that receive data:

    - When message comes in on the configured socket, see if there is
      a connection object. If not, create it.

    - Create a map entry linking the connection to the connection object.

    - Add the connection to the connection object/list.

    - Once the link to the channel is valid, all connections receive an
      initial message with information on the channel configuration
      (datatype, entry, datatype info).

    - If a connection is created when the channel is already valid, the
      initial message is sent immediately.
 */
struct ConnectionList
{
  /** Close off marker for the messages, used to distinguish between binary
      (msgpack) and utf-8 text (JSON) */
  unsigned char marker;

  /** Server and provider of id */
  const WebSocketsServerBase *master;

  /** What this is for, for error messages */
  std::string identification;

  /** List of connections through non-secure server */
  typedef std::list<std::shared_ptr<WsServer::Connection>> connectionlist_t;

  /** List of connections through secure server */
  typedef std::list<std::shared_ptr<WssServer::Connection>> sconnectionlist_t;

  /** List of connections through non-secure server */
  connectionlist_t connections;

  /** List of connections through secure server */
  sconnectionlist_t sconnections;

  /** For the callback function */
  const GlobalId &getId();

  /** Add an additional data reading connection */
  void addConnection(std::shared_ptr<WsServer::Connection> &c);

  /** Add an additional data reading connection, secure socket */
  void addConnection(std::shared_ptr<WssServer::Connection> &c);

  /** Remove the data connection */
  bool removeConnection(const std::shared_ptr<WsServer::Connection> &c);

  /** Remove the data connection, secure server, secure connection */
  bool removeConnection(const std::shared_ptr<WssServer::Connection> &c);

  /** Send some string or data to all connections on the endpoint */
  void sendAll(const std::string &data, const char *desc);

  /** Send some string or data to specified connection */
  template <typename C>
  void sendOne(const std::string &data, const char *desc, const C &c);

  /** Close the connections */
  void close(const char *reason, int status = 1000);

  /** Constructor */
  ConnectionList(const std::string &ident, const WebSocketsServerBase *master);

  /** Destructor */
  ~ConnectionList();
};

/** Definition of access to a single entry in a channel.

    Reads the latest current data in the channel, upon a dummy
    message from a connected websocket.

    After opening the read, a first message provides the
    structure of the data.

    Subsequent messages follow after a dummy (empty/null) message
    written to the websocket.
*/
struct SingleEntryRead : public ConnectionList
{

  /** Autostart callback function */
  Callback<SingleEntryRead> autostart_cb;

  /** Activity for receiving channel validatation */
  ActivityCallback do_valid;

  /** Channel access token */
  ChannelReadToken r_token;

  /** Data type */
  std::string datatype;

  /** Flag to remember init state. */
  bool inactive;

  /** Constructor, based on entry id

      @param channelname Name of the channel.
      @param datatype    Type of data read.
      @param eid         Entry number in the channel.
      @param master      ID for creating the read token.
  */
  SingleEntryRead(const std::string &channelname, const std::string &datatype,
                  entryid_type eid, const WebSocketsServerBase *master,
                  const PrioritySpec &ps);

  /** Destructor */
  ~SingleEntryRead();

  /** Close connection */
  void close(const char *reason, int status = 1000);

  /** Check the token valid */
  bool checkToken();

  /** Add an additional data reading connection */
  template <typename C> void addConnection(C &c);

  /** Code data */
  template <typename C> void passData(const TimeSpec &ts, C &connection);

private:
  /** Callback invoked when the token is valid. */
  void tokenValid(const TimeSpec &ts);
};

/** Access to a single entry in a channel

    Reads all data from a channel, sends it to zero or more connected
    websockets as new data comes in.

    When the channel token is valid, each connection will first receive
    a definition of the data. Subsequent data follows as it is received
    on the channel.
  */
struct SingleEntryFollow : public ConnectionList
{
  /** Autostart callback function */
  Callback<SingleEntryFollow> autostart_cb;

  /** Activity for receiving channel validatation */
  ActivityCallback do_valid;

  /** Channel access token */
  ChannelReadToken r_token;

  /** Callback object */
  Callback<SingleEntryFollow> cb;

  /** Activity for getting more data */
  ActivityCallback do_calc;

  /** Data type */
  std::string datatype;

  /** Initial stage */
  bool inactive;

  /** Flag to remember first write access */
  bool firstwrite;

  /** Trigger regulation, if applicable */
  boost::intrusive_ptr<TriggerRegulatorGreedy> regulator;

  /** Destructor */
  ~SingleEntryFollow();

  /** Add an additional data reading connection */
  template <typename C> void addConnection(C &c);

  /** Pass data, callback from DUECA */
  void passData(const TimeSpec &ts);

  /** Constructor.

      @param channelname Name of the channel.
      @param datatype    Type of data read.
      @param eid         Entry number in the channel.
      @param master      Master ID, for allocating activity and tokens.
      @param ps          Reading priority.
      @param ts          If span non-zero, defines data rate
      @param autostart   Start when channel valid
  */
  SingleEntryFollow(const std::string &channelname, const std::string &datatype,
                    entryid_type eid, const WebSocketsServerBase *master,
                    const PrioritySpec &ps, const DataTimeSpec &ts);

  /** Check the token valid */
  bool checkToken();

  /** Start following the channel data */
  bool start(const TimeSpec &ts);

  /** Stop following the channel data */
  bool stop(const TimeSpec &ts);

  /** Disconnect from the triggering channel */
  void disconnect();

  /** Close the connection */
  void close(const char *reason, int status = 1000);

private:
  /** Callback invoked when the token is valid. */
  void tokenValid(const TimeSpec &ts);
};

/** Configuration of monitorable channels

    Creates a watcher to monitor changes in a channel. Publicises to
    all connected websockets */
struct ChannelMonitor : public ChannelWatcher, public ConnectionList
{
  /** Channel name */
  std::string channelname;

  /** Time specification, for data rate reduction */
  DataTimeSpec time_spec;

  /** Type for list of active entries and their datatype */
  typedef std::vector<std::string> entrydataclass_t;

  /** Current list of active entries and their datatype */
  entrydataclass_t entrydataclass;

  /** Callback from the ChannelWatcher */
  void entryAdded(const ChannelEntryInfo &info);

  /** Callback from the ChannelWatcher */
  void entryRemoved(const ChannelEntryInfo &info);

  /** Return non-zero string if a corresponding entry exists */
  const std::string &findEntry(unsigned entryid);

  /** Send existing data to connection, if required */
  void addConnection(std::shared_ptr<WsServer::Connection> &c);

  /** Send existing data to connection, if required */
  void addConnection(std::shared_ptr<WssServer::Connection> &c);

  /** Constructor */
  ChannelMonitor(const WebSocketsServerBase *server,
                 const std::string &channelname, const DataTimeSpec &ts);

    /** Destructor */
  virtual ~ChannelMonitor();
};

/** Single written entry.

    This encapsulates a writing token for a single entry in a channel.

    - Channel name and data class type are defined when the WriteEntry is
   created.

    - The token is created on the first message from the socket. This should
      contain the label for the channel, stream/event type, and whether the
      socket will contain timing information, otherwise timing is simply
      derived from time of arrival.

*/
struct WriteEntry INHERIT_REFCOUNT(WriteEntry)
{
  INCLASS_REFCOUNT(WriteEntry);

  /** State for this entry */
  enum WEState {
    UnConnected,     /**< Not connected to a socket */
    Connected, /**< Connected to a socket, but no entry, or not confirmed */
    Linked /**< Connected, and linked to an entry */
  };

  /** State for this entry */
  WEState state;

  /** for id callback */
  const WebSocketsServerBase *master;

  /** Autostart callback function */
  Callback<WriteEntry> autostart_cb;

  /** Activity for receiving channel validatation */
  ActivityCallback do_valid;

  /** Write token */
  boost::scoped_ptr<ChannelWriteToken> w_token;

  /** for error messages */
  std::string identification;

  /** channel name */
  std::string channelname;

  /** data type */
  std::string datatype;

  /** Flag indicating timing is controlled by client */
  bool ctiming;

  /** Activity monitor */
  bool active;

  /** to be expeditious, remember stream or not */
  bool stream;

  /** bulk sending, if applicable  */
  bool bulk;

  /** differential pack, if applicable */
  bool diffpack;

  /** Connections through non-secure server */
  typedef std::shared_ptr<WsServer::Connection> connection_t;

  /** Connection through secure server */
  typedef std::shared_ptr<WssServer::Connection> sconnection_t;

  /** List of connections through non-secure server */
  connection_t connection;

  /** List of connections through secure server */
  sconnection_t sconnection;

  /** Set the connection link */
  template <typename C> void setConnection(C &connection);

  /** global id */
  const GlobalId &getId();

  /** Verify token OK */
  bool checkToken();

  /** Indicate this is no longer connected to a socket. */
  inline void doDisconnect() { state = UnConnected; }

  /** From now on connected to a socket. */
  inline void doConnect() { state = Connected; }

  /** Available is for re-used PresetWriters, needs unconnected. */
  inline bool isAvailable() { return state == UnConnected; }

  /** Send data */
  void sendOne(const std::string &data, const char *desc);

  /** Constructor

      @param channelname  Name for the to-be-written channel
      @param datatype     Data class to be written
   */
  WriteEntry(const std::string &channelname, const std::string &datatype,
             const WebSocketsServerBase *master, const PrioritySpec &ps,
             bool bulk = false, bool diffpack = false,
             WriteEntry::WEState initstate = WriteEntry::Connected);

  /** Destructor */
  virtual ~WriteEntry();

  /** Completion, using information in the first written message

      This will create the write token, using information in the first
      written message.

      @param message1     JSON-encoded first message
      @param master       ID of controlling entity, for assigning channel entry.
  */
  virtual void complete(const std::string &datatype, const std::string &label,
                        bool stream, bool ctiming, bool bulk, bool diffpack);

  /** Check whether completion has been done

      @returns            True if first message processed and token created
  */
  inline bool isComplete() const { return state == Linked; }

  /** Write data to channel using JSON

      @param json         JSON encoded data
  */
  template <typename Encoder> void writeFromCoded(const Encoder &coded);

  /** Close the connection */
  void close(const char *reason, int status = 1000);

private:
  /** Callback valid token */
  void tokenValid(const TimeSpec &ts);
};

/** Single written entry, static and valid from specification

    This encapsulates a writing token from socket data.

    - The writing token is created instantly

    - On the first message, the configuration for this token is checked; it
      must match the information in the first message, with the exception of
      any label, which is ignored.
 */
struct PresetWriteEntry : public WriteEntry
{
  /** Constructor */
  PresetWriteEntry(const std::string &channelname, const std::string &datatype,
                   const std::string &label, const WebSocketsServerBase *master,
                   const PrioritySpec &ps, bool ctiming, bool stream, bool bulk,
                   bool diffpack);

  /** Destructor */
  ~PresetWriteEntry();

  /** Disconnect the old link */
  void *disConnect();

  /** Close the connection */
  void close(const char *reason, int status = 1000);

  /** Complete

      A PresetWriteEntry alread has its write token created. It still accepts
      the first message, as a normal WriteEntry, but only uses it to check
      against configuration.

      @param datatype     Data class
      @param label        Label for the entry
      @param stream       Stream entry
      @param ctiming      Provide controlling timing
      @param bulk         Bulk data transfor
      @param diffpack     Differential packing
      @param master       ID of controlling entity, ignored here.

      @throws presetmismatch  When the data in message1 does not match
                          configuration.
  */
  void complete(const std::string &datatype, const std::string &label,
                bool stream, bool ctiming, bool bulk, bool diffpack);
};

/** Configuration of entry writing reading combination */
struct WriteReadSetup
{
  /** Channel name for writing */
  std::string w_channelname;

  /** Channel name for reading */
  std::string r_channelname;

  /** bulk sending, if applicable  */
  bool bulk;

  /** differntial pack, if applicable */
  bool diffpack;

  /** Constructor */
  WriteReadSetup(const std::string &wchannelname,
                 const std::string &rchannelname);

  /** Return the next connection ID */
  static unsigned getNextId();
};

/** Single write + read entry.

    This encapsulates a writing token for a single entry in a channel, and
    a matching read token for a single entry.

    - Needs two channel names. The dataclass for the write end is given
      in the first message, the dataclass for the read end is determined
      by the "server" that creates a matching reply.

    - The write token is created on the first message from the socket. The
      read token creation is detected from monitoring the read channel. The
      first written message should contain the dataclass for the write entry.

    - The first reply on the url will be with information on the write entry
      and the read entry.
*/
struct WriteReadEntry :
  INHERIT_REFCOUNT_COMMA(WriteReadEntry) public ChannelWatcher
{
  INCLASS_REFCOUNT(WriteReadEntry);

  /** Autostart callback function */
  Callback<WriteReadEntry> autostart_cb;

  /** Activity for receiving channel validatation */
  ActivityCallback do_valid;

  /** close off marker */
  unsigned char marker;

  /** State for this entry */
  enum WRState {
    UnConnected, /**< Not yet connected to a socket, right after creation of the
                    entry. */
    Connected, /**< Connected to a socket, but no entry, or not confirmed */
    ValidatingTokens, /**< Waiting until writing end and reading ends are
                         connected */
    ActivateClient, /**< Send a message with read/write config to client */
    Linked, /**< Connected, and linked to two entries */
    DisConnected /**< Disconnected by the server after use */
  };

  /** State for this entry */
  WRState state;

  /** Connections through non-secure server */
  typedef std::shared_ptr<WsServer::Connection> connection_t;

  /** Connection through secure server */
  typedef std::shared_ptr<WssServer::Connection> sconnection_t;

  /** List of connections through non-secure server */
  connection_t connection;

  /** List of connections through secure server */
  sconnection_t sconnection;

  /** Write token */
  boost::scoped_ptr<ChannelWriteToken> w_token;

  /** Read token */
  boost::scoped_ptr<ChannelReadToken> r_token;

  /** for error messages */
  std::string identification;

  /** channel name writing */
  std::string w_channelname;

  /** channel name reading */
  std::string r_channelname;

  /** data type writing */
  std::string w_dataclass;

  /** data type reading */
  std::string r_dataclass;

  /** label for connecting data */
  std::string label;

  /** master id */
  WebSocketsServerBase *master;

  /** Activity monitor */
  bool active;

  /** bulk sending, if applicable  */
  bool bulk;

  /** differntial pack, if applicable */
  bool diffpack;

  /** Extended JSON */
  bool extended;

  /** Verify token OK */
  bool checkToken();

  /** Available is for re-used PresetWriters, needs unconnected. */
  inline bool isAvailable() { return state == UnConnected; }

  /** Callback object */
  Callback<WriteReadEntry> cb;

  /** Activity for getting more data */
  ActivityCallback do_calc;

  /** Create a write+read combination for a single URL

      @param w_channelname  Name for the to-be-written channel
      @param r_channelname  Name for the read channel
      @param id             Sequence number, to differentiate from
                            other clients/connections
      @param initstate      Starting state of the object
  */
  WriteReadEntry(std::shared_ptr<WriteReadSetup> setup,
                 WebSocketsServerBase *master, const PrioritySpec &ps,
                 bool extended);

  /** Destructor */
  virtual ~WriteReadEntry();

  /** Set the connection link, right after creation */
  void setConnection(connection_t connection);

  /** Set the connection link */
  void setConnection(sconnection_t connection);

  /** Disconnect */
  inline void doDisconnect() { state = DisConnected; }

  /** Return host ID */
  const GlobalId &getId() const;

  /** Completion, uses information in the first written message

      This will create the write token, using information on the data type in
      the first received on the socket. State will change to ValidatingWrite.

      @param datatype     Type of data for the write channel.
  */
  void complete(const std::string &datatype, const std::string &addtolabel);

  /** Close the connection */
  void close(const char *reason, int status = 1000);

  /** Check whether communication is possible. Both read and write tokens
      need to be valid.

      @returns            True if first message processed and token created
  */
  inline bool isComplete() const { return state == Linked; }

  /** Write data to channel using JSON

      @param coded        Encoded data
  */
  template <typename Decoder> void writeFromCoded(const Decoder &code);

  /** Process new entry in the channel watch */
  void entryAdded(const ChannelEntryInfo &i) final;

  /** Monitor for removal of my entry */
  void entryRemoved(const ChannelEntryInfo &i) final;

private:
  /** Callback on token validity */
  void tokenValid(const TimeSpec &ts);

  /** Send data */
  void sendOne(const std::string &data, const char *desc);

  /** Callback on data entry */
  void passData(const TimeSpec &ts);
};

/** Configuration of entry writing */
struct WriteableSetup
{

  /** Channel name */
  std::string channelname;

  /** Entry data class */
  std::string dataclass;

  /** Constructor */
  WriteableSetup(const std::string &channelname, const std::string &dataclass);
};

/** Key type for this single entry */
struct NameEntryId
{

  /** URL name */
  std::string name;

  /** URL entry id */
  unsigned id;

  /** Constructor */
  NameEntryId(const std::string &name, unsigned id);

  /** Comparison */
  bool operator<(const NameEntryId &other) const;
};

/** Key type for a single entry, followed with token */
struct NameEntryTokenId
{

  /** URL name */
  std::string name;

  /** URL entry id */
  unsigned id;

  /** Token */
  std::string token;

  /** Constructor */
  NameEntryTokenId(const std::string &name, unsigned id,
                   const std::string token);

  /** Comparison */
  bool operator<(const NameEntryTokenId &other) const;
};

/** Key type for a channel and token */
struct NameTokenId
{

  /** URL name */
  std::string name;

  /** Token */
  std::string token;

  /** Constructor */
  NameTokenId(const std::string &name, const std::string token);

  /** Comparison */
  bool operator<(const NameTokenId &other) const;
};

/** Map to find the single reads, given name and entry id */
typedef std::map<NameEntryId, std::shared_ptr<SingleEntryRead>> singleread_t;

/** Map to find the single reads given a connection */
typedef std::map<void *, std::shared_ptr<SingleEntryRead>> singlereadmap_t;

/** Map to find follow/push entries */
typedef std::map<NameEntryId, std::shared_ptr<SingleEntryFollow>> followread_t;

/** Map to find monitors */
typedef std::map<std::string, std::shared_ptr<ChannelMonitor>> monitormap_t;

/** Map with writeable channels */
typedef std::map<std::string, std::shared_ptr<WriteableSetup>> writeables_t;

/** Map with pre-configured write channels */
typedef std::map<std::string, boost::intrusive_ptr<PresetWriteEntry>>
  presetwrites_t;

/** Map with set-ups for WriterReader combinations */
typedef std::map<std::string, std::shared_ptr<WriteReadSetup>> writereadables_t;

/** Map with active writers */
typedef std::map<void *, boost::intrusive_ptr<WriteEntry>> writers_t;

/** Map with active writers/reader combination */
typedef std::map<void *, boost::intrusive_ptr<WriteReadEntry>> writersreaders_t;

DUECA_NS_END;
WEBSOCK_NS_END;

#endif
