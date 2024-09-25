/* ------------------------------------------------------------------   */
/*      item            : WebSocketsServer.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Tue Nov 27 13:58:45 2018
        category        : header file
        description     : DUECA_API
        changes         : Tue Nov 27 13:58:45 2018 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef WebSocketsServer_hxx
#define WebSocketsServer_hxx

// include the dusime header
#include "WebsockExceptions.hxx"
#include <ChannelWatcher.hxx>
#include <StateGuard.hxx>
#include <boost/asio.hpp>
#include <boost/version.hpp>
#include <dueca.h>
#include <dueca_ns.h>
#if BOOST_VERSION < 106600
namespace boost {
namespace asio {
typedef io_service io_context;
}
} // namespace boost
#define BOOST1_65
#endif

// include headers for functions/classes you need in the module
#include <simple-web-server/server_http.hpp>
#include <simple-web-server/server_https.hpp>
#include <simple-websocket-server/server_ws.hpp>
#include <simple-websocket-server/server_wss.hpp>

#include "CommonChannelServer.hxx"

DUECA_NS_START;
WEBSOCK_NS_START;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpsServer = SimpleWeb::Server<SimpleWeb::HTTPS>;

/** Common base type for websocket servers

*/
class WebSocketsServerBase : public dueca::Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef WebSocketsServerBase _ThisModule_;

protected: // simulation data
  /** Marker token for the websocket send calls */
  unsigned char marker;

  /** Server, uncoded */
  boost::scoped_ptr<WsServer> server;

  /** Server, coded */
  boost::scoped_ptr<WssServer> sserver;

  /** Http server, uncoded */
  boost::scoped_ptr<HttpServer> http_server;

  /** Http Server, coded */
  boost::scoped_ptr<HttpsServer> https_server;

  /** Certificate for ssl use, if certificate and key are configured, the
      ssl version is used. */
  std::string server_crt;

  /** Associated key */
  std::string server_key;

  /** IO context to perform a run */
  std::shared_ptr<boost::asio::io_context> runcontext;

  /** Port to be used */
  unsigned port;

  /** Port for http server */
  unsigned http_port;

  /** Folder with files for http server */
  std::string document_root;

  /** Mime types map */
  std::map<std::string, std::string> mimemap;

  /** Flag to indicate aggressive reconnection to preset entries */
  bool aggressive_reconnect;

  /** Immediate start, do not wait on DUECA's commands */
  bool immediate_start;

  /** Start flag for immediate_start */
  bool auto_started;

  /** Access lock for shared data */
  StateGuard thelock;

  /** Priority for reading */
  PrioritySpec read_prio;

  /** Timing specification */
  TimeSpec time_spec;

  /** If true, use the extended, non-official JSON specification */
  bool extended;

  /** Mapping connecting URL details to an object that reads a
      specific channel entry.

      This is a map, indexed by Name+Entry, mapping URL basename +
      entry ID to SingleEntryRead objects with a read token. These are
      configured through the start script.
  */
  singleread_t readsingles;

  /**  Mapping connecting URL details to an object that reads a
       specific channel entry.

       Same type as the readsingles, however, created automatically
       from channel entries detected in one of the monitor locations,
       and only created when a client asks for access.
  */
  singleread_t autosingles;

  /** Mapping connection id to SingleEntryRead objects

      A mapping from connection ID/pointer to the SingleEntryRead
      objects in use by these connections. These objects are either
      common with the readsingles or autosingles maps.
  */
  singlereadmap_t singlereadsmapped;

  /** Mapping connecting URL details to an object that reads and
      follows the data in a specific channel entry.

      Maps url name and entry id to a SingleEntryFollow object. The
      SingleEntryFollow object has a list of connections as clients
      that will get new data from the followed entry as it comes in.
  */
  followread_t followers;

  /** Mapping connecting URL details to an object that reads and
      follows the data in a specific channel entry.

      Maps url name and entry id to a SingleEntryFollow object,
      created on the basis of a monitor. The SingleEntryFollow object
      has a list of connections as clients that will get new data from
      the followed entry as it comes in.
  */
  followread_t autofollowers;

  /** map from URL to monitors */
  monitormap_t monitors;

  /** map from URL to writer setup */
  writeables_t writersetup;

  /** map with preconfigured writeables; these have the channel token created
   */
  presetwrites_t presetwriters;

  /** map from connection pointers to the actual writers */
  writers_t writers;

  /** map from URL to write/read combination setup */
  writereadables_t writereadsetup;

  /** map with active write/read combinations */
  writersreaders_t writersreaders;

private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm myclock;

  /** Callback object for simulation calculation. */
  Callback<WebSocketsServerBase> cb1;

  /** Activity for simulation calculation. */
  ActivityCallback do_transfer;

public: // class name and trim/parameter tables
  /** Return the parameter table. */
  static const ParameterTable *getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  WebSocketsServerBase(Entity *e, const char *part, const PrioritySpec &ts,
                       const char *classname, unsigned char marker);

  /** Helper function, templated with the server type */
  template <typename S> bool _complete_http(S &server);

  /** Destructor. */
  virtual ~WebSocketsServerBase();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec &ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int> &i);

  /** Define a URL for reading latest data */
  bool setCurrentData(const std::vector<std::string> &i);

  /** Define a URL for following all data in a channel entry */
  bool setFollowData(const std::vector<std::string> &i);

  /** Define a URL for tracking changes in a channel */
  bool setChannelInfo(const std::vector<std::string> &i);

  /** Define a URL for writing to a channel */
  bool setWriterSetup(const std::vector<std::string> &i);

  /** Define a URL for writing to a channel */
  bool setPresetWriterSetup(const std::vector<std::string> &i);

  /** Two-way communication set-up */
  bool setWriteReadSetup(const std::vector<std::string> &i);

  /** Set SLL certificates; will convert to use SSL */
  bool setCertFiles(const std::vector<std::string> &i);

  /** Add a mime type */
  bool addMimeType(const std::vector<std::string> &i);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared() final;

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time) final;

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time) final;

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doTransfer(const TimeSpec &ts);

public: // coding function
  /** sending marker, binary or string */
  inline unsigned char getMarker() const { return marker; }

  /** Send data with, in a "tick"/"data" struct */
  virtual void codeData(std::ostream &s, const DCOReader &r) const = 0;

  /** Code empty, no data */
  virtual void codeEmpty(std::ostream &s) const = 0;

  /** Write type information to given stream */
  virtual void codeEntryInfo(std::ostream &s, const std::string &w_dataname,
                             unsigned w_entryid, const std::string &r_dataname,
                             unsigned r_entryid) const = 0;
};

/** Webserver providing access to DUECA channels.

    There are two possible variants for this server, "web-sockets-server", which
    communicates with JSON-coded messages, and "web-sockets-server-msgpack",
   which uses msgpack (binary) messages for communication.

    This server can define a number of URL's:

    <table>
    <tr><th>URL</th><th>Description</th></tr>

    <tr>
    <td> /configuration </td>

    <td> After opening the configuration URL, the client receives a message with
    all possible configured URL's, in the sections "current", "read",
    "info", "write" and "write-and-read".

    - current. These endpoints respond to a message from the client and then
      produce a reply with data. Information in the information section
   includes:
      * endpoint
      * dataclass
      * information on the dataclass members
      * entry id (numeric)

    - read. These endpoints will produce messages at the rate of channel
      writing, or "throttled", when a time specification is given. Inforation in
      the information section includes:
      * endpoint
      * dataclass
      * type info
      * numeric entry id

    - info. These endpoints provide information on the entries in a specific
      channel. Inforation only consists of
      * endpoint

    - write. These endpoints provide information on entries that can be
      written to. Information consists of
      * endpoint
      * dataclass
      * type info
      The latter two are only given if dataclass had been pre-configured. If
      not given, the dataclass needs to be specified in the first message from
      a client.

    - write-and-read. These endpoints can be used to establish a two-directional
      communication on two pre-defined channels. Information consists of
      * endpoint
      The first message upon opening needs to define the dataclass and label.
      Once bi-directional communication is established, a message is sent with
      information on both the write direction and read direction.

    - granule. This is the last element in the struct, it defines the value in
      seconds of a single integer time increment.

    </td>
    </tr>

    <tr>
    <td> /current/name?entry=.. </td>

    <td> From a channel labeled "name", reads the current/latest data
    from the entry given. To get fresh data, write an empty message on the
    socket (will be discarded). Returns a object with a member
    "tick", indicating the time tick and single set of data defined in
    a member "data". If omitted from the URL, the entry id is assumed
    to be 0. The entry may have been explicitly configured from the
    start script, and thus its information was provided in the
    /configuration URL, or it was added by a channelwatcher, and information
    was provided in an /info/... url (see later).
    </td>
    </tr>

    <tr><td> /read/name?entry=.. </td>

    <td> From a channel labeled "name", and given entry, open a
    following read token. Receives all data as it comes in. Data push
    is provided by the DUECA side. For each new set of data, this
    returns a JSON array with objects containing time tick and set of
    data defined. The read entries were either specifically
    configured (from /configuration), or they are available from
    information obtained through an info URL.
    </td></tr>

    <tr>
    <td> /info/name </td><td>

    Initiate information gathering for a specific channel. The watching
    capability (and thereby the url) must have been configured
    beforehand. For each entry in the channel, this returns a JSON
    object with the following elements; 'dataclass', for the class of
    object, 'entry', giving the entry number, 'typeinfo', with an
    array of type information (see below). </td>
    </tr>

    <tr>
    <td> /write/name </td>

    <td> Open a write entry for a channel. The data type may be specified
    in the start script of the module. The first message must be a
    json object with the member 'label'. Optionally, if this has a
    member 'ctiming' equal to true, the json client provides the
    timing. If this object has a member 'event' equal to false, a
    stream channel is created; in that case ctiming must be true. If
    the datatype has not been configured beforehand, this needs to
    be supplied in the first message.

    Optionally a /write/name entry can have been created as preset. In
    that case, the write token is created at startup of the server,
    and writing will be immediately available. The write token and
    channel entry will also persist after a connection is lost, and
    then a new connection can assume the write entry and continue.
    </td>
    </tr>

    <tr>
    <td> /write-and-read/name </td>

    <td> This uses a single bi-directional websocket connection for
    writing a single entry and reading a corresponding entry in another
    channel. Information on the data types is provided after completion
    of the connection. This needs a DUECA module that acts as a server,
    monitors the channel that is written to from the websocket connection,
    and creates a matching entry in the reply channel.

    The first message from the client must contain a member "dataclass",
    indicating the DCO type the client wishes to write. An entry in the
    configured channel is created with this dataclass.

    After the server has created a corresponding entry in the reply
    channel, (having the exact same label), a websocket message is
    sent back, defining the read and write entry numbers, dataclass and
    type information. After this, messages can be written, to which
    the server module should provide replies.

    </td>

    </tr></table>

    Type information on the members of a dataclass, as returned by the
    channelinfo URL has the following structure:

    <table>

    <tr><th>key</th><th>Description</th></tr>

    <tr><td>name</td>
    <td> Name of the object's member</td></tr>

    <tr><td>type</td>
    <td> Type / data class of the object (c++ name)</td></tr>

    <tr><td>size (optional)</td>
    <td> For fixed size arrays, the size will be given</td></tr>

    <tr><td>array (optional)</td>
    <td> For array/list types, this value will be True</td></tr>

    <tr><td>typeinfo (optional)</td>
    <td> If the member is a nested DCO type, the typeinfo will be
    recursively presented in an array </td></tr>

    <tr><td>map (optional)</td>
    <td> If the member is a mapped type (e.g. stl map), this value
    will be true</td></tr>

    <tr><td>keytype (optional)</td>
    <td> If the member is a mapped type (e.g. stl map), this will
    contain the type / class of the used key </td></tr>
    </table>

    The instructions to create an module of this class from the script are:

    \verbinclude web-sockets-server.scm

    For the configuration of the server, the following options are available.

    <ol>
    <li> Manually setting up a /current URL. This specifies a channel
    entry from a specific channel. Meaningful only for channels with
    stream data (otherwise you might miss events if reading is not
    fast enough). </li>

    <li> Manually setting up a /read URL. Useful for following
    events and for obtaining all data written in a channel, e.g. for
    plotting. </li>

    <li> Setting up a /info URL. This can be read to follow
    changes, and to obtain (initially) the configuration of a
    channel. With the data one can choose to either open read
    URL's or current URL's; with an info URL configured, the
    corresponding read or current URL become available,
    given that the entries are created.
    </li>

    <li> Setting up a /write URL. The /write URL links the endpoint
    to the channel name. The first message to such a write URL needs the
    selection of dataclass, label, and timing control (timing supplied by
    the websockets side, or added by DUECA on basis of actual time).

    <li> Setting up a /write URL with preset/pre-defined entry. In this
    case, the channel is opened immediately, and a single entry is entered
    in the channel. This mode is useful if you want the channel entry to
    be present for the rest of the simulation. If for some reason the
    websockets connection is broken, a new connection will take its place.

    <li> Setting up a /write-and-read URL.
    </ol>

    To provide static datafiles (e.g., to start off your javascript
    program), the module may be configured to act as a very simple web
    server as well, serving files from a single folder. To get better
    compatibility with browsers, also configure the mime types for
    these files.
 */
template <typename Encoder, typename Decoder>
class WebSocketsServer : public WebSocketsServerBase
{
  /** self-define the module type, to ease writing the parameter table */
  typedef WebSocketsServer<Encoder, Decoder> _ThisModule_;

private: // simulation data
public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  WebSocketsServer(Entity *e, const char *part, const PrioritySpec &ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete() final;

  /** Helper function, templated with the server type */
  template <typename S> bool _complete(S &server);

  /** Code the data in the reader object. */
  void codeData(std::ostream &s, const DCOReader &r) const final;

  /** Code the data in the reader object. */
  void codeEmpty(std::ostream &s) const final;

  /** Code the type information in the reader object. */
  void codeEntryInfo(std::ostream &s, const std::string &w_dataname,
                     unsigned w_entryid, const std::string &r_dataname,
                     unsigned r_entryid) const final;

  /** Destructor. */
  ~WebSocketsServer();
};

WEBSOCK_NS_END;
DUECA_NS_END;

#endif
