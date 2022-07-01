/* ------------------------------------------------------------------   */
/*      item            : ConfigStorage.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Wed Jun 17 21:21:15 2020
        category        : header file
        description     :
        api             : DUECA_API
        changes         : Wed Jun 17 21:21:15 2020 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ConfigStorage_hxx
#define ConfigStorage_hxx

// include the dusime header
#include <dueca.h>
#include <dueca/ChannelWatcher.hxx>
#include "ConfigFileData.hxx"
#include "ConfigFileRequest.hxx"
#include <boost/scoped_ptr.hpp>

// include headers for functions/classes you need in the module
DUECA_NS_START;

/** Store configuration files

    This module communicates over two channels to offer the storage and
    retrieval of configuration files. It will monitor a folder for files
    with a specific suffix, send a list of these files on request,
    send the contents of one of the files or accept content for a new file.

    Within DUECA, it is used for storing web-based quickview
    configuration files.

    The instructions to create an module of this class from a
    script are:

    \verbinclude config-storage.scm
 */
class ConfigStorage: public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ConfigStorage _ThisModule_;

public: // simulation data
  /// Suffix for the configuration files
  std::string                        file_suffix;

  /// Folder for storing and retrieving the configuration files
  std::string                        path_configfiles;

  /// Sending channel name
  std::string                        w_channelname;

  /// Receiving channel name
  std::string                        r_channelname;

  /// Do we allow overwrite
  bool                               allow_overwrite;

  /// Optional timing filename template
  std::string                        lftemplate;

private: // channel access

  /** Configuration information per client.

      Multiple clients can be served, there is a configuration with channel
      links per client.
  */
  struct ConfigClient {

    /// Pointer to the master data
    const ConfigStorage             *master;

    /// request input
    ChannelReadToken                 r_request;

    /// Response with configuration file data
    ChannelWriteToken                w_answer;

    /// Callback object for reacting to incoming messages
    Callback<ConfigClient>           cb;

    /// Activity object
    ActivityCallback                 do_respond;

    /** Construct a new client */
    ConfigClient(const _ThisModule_* master,
                 const ChannelEntryInfo& i);

    /** Action response */
    void respondRequest(const TimeSpec& ts);

    /** ID return */
    const GlobalId& getId() const;
  };

  /** Monitoring object */
  class MyWatcher: public ChannelWatcher
  {
    /** Pointer to master. */
    ConfigStorage                   *master;

  public:
    /** Constructor, sets up watching and relays results */
    MyWatcher(ConfigStorage *master, const std::string& channelname);

    /** respond to new entry in channel */
    void entryAdded(const ChannelEntryInfo& i) final;

    /** remove after disconnect and entry deletion */
    void entryRemoved(const ChannelEntryInfo& i) final;
  };

  /** Locate the monitoring object */
  boost::scoped_ptr<MyWatcher>      watcher;

  /** List of clients */
  typedef std::list<std::shared_ptr<ConfigClient> > clients_t;

  /** List of clients */
  clients_t                         clients;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const          classname;

  /** Return the parameter table. */
  static const ParameterTable*      getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  ConfigStorage(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. */
  bool complete() final;

  /** Destructor. */
  ~ConfigStorage();

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared() final;

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time) final;

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time) final;

  /** respond to new entry in channel */
  void entryAdded(const ChannelEntryInfo& i);

  /** remove after disconnect and entry deletion */
  void entryRemoved(const ChannelEntryInfo& i);
};

DUECA_NS_END;

#endif
