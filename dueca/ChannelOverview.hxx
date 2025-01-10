/* ------------------------------------------------------------------   */
/*      item            : ChannelOverview.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Mon Apr 23 18:18:36 2018
        category        : header file
        description     :
        changes         : Mon Apr 23 18:18:36 2018 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelOverview_hxx
#define ChannelOverview_hxx

// include headers for functions/classes you need in the module
#include "dueca_ns.h"
#include <Callback.hxx>
#include <CallbackWithId.hxx>
#include <ChannelReadToken.hxx>
#include <ChannelWriteToken.hxx>
#include <Module.hxx>
#include <AperiodicAlarm.hxx>
#include <SimTime.hxx>
#include <dueca/ChannelReadInfo.hxx>
#include <dueca/ChannelWriteInfo.hxx>
#include <dueca/ChannelManager.hxx>
#include <dueca/ChannelWatcher.hxx>
#include <dueca/UChannelEntryData.hxx>
#include <dueca/ChannelCountRequest.hxx>
#include <dueca/ChannelCountResult.hxx>
#include <list>
#include <fstream>
#include <memory>

DUECA_NS_START

class ChannelDataMonitor;

/** Provide an overview of channel use.

    This module monitors information on creation of read access and
    write access to channels.

    The instructions to create an module of
    this class from the Scheme script are:

    \verbinclude channel-view.scm
 */
class ChannelOverview : public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ChannelOverview _ThisModule_;

private: // simulation data
  // declare the data you need in your simulation
  /** Quick access to the ChannelManager */
  ChannelManager *cmanager;

  /** Request count counter */
  unsigned countid;

  /** File with read information summaries */
  std::ofstream readinfo_file;

  /** File with write information summaries */
  std::ofstream writeinfo_file;

private: // channel access
  // declare access tokens for all the channels you read and write

  /** Monitoring object, whenever a channel triggers this, calls back into
      the handling class */
  struct MonitorEntry
  {

    /** Access token */
    ChannelReadToken r_info;

    /** Callback object for collecting change information. */
    CallbackWithId<ChannelOverview, ChannelReadToken *> cb;

    /** Activity for simulation calculation. */
    ActivityCallback get_info;

    /** Constructor
        @param ns        Channel name
        @param entry_id  Monitored entry
        @param classname Datatype class
        @param origin    Writer
        @param ptr       Encapsulating object
        @param h         Callback function
    */
    MonitorEntry(const NameSet &ns, entryid_type entry_id,
                 const char *classname, const GlobalId &origin,
                 ChannelOverview *ptr,
                 void (ChannelOverview::*h)(const TimeSpec &t,
                                            ChannelReadToken *&r));
  };

  /** List of monitoring entries */
  typedef std::list<std::shared_ptr<MonitorEntry>> t_monitorentrylist;

  /** Channels  watcher derivative  */
  struct WatchReadInfo : public ChannelWatcher
  {

    /** Pointer back to the handling object */
    ChannelOverview *ptr;

    /** Constructor */
    WatchReadInfo(ChannelOverview *ptr);

    /** List of monitors that will each listen out a channel entry */
    t_monitorentrylist r_readinfo;

    /** Callback for new entry */
    void entryAdded(const ChannelEntryInfo &i);

    /** Callback for removal */
    void entryRemoved(const ChannelEntryInfo &i);
  };

  /** Monitoring object, monitors the channel with entries signalling
      read changes of entries. Each node will have one entry in this
      channel */
  WatchReadInfo watch_readinfo;

  /** Channel watcher derivative */
  struct WatchWriteInfo : public ChannelWatcher
  {

    /** Pointer back to the handling object */
    ChannelOverview *ptr;

    /** Constructor */
    WatchWriteInfo(ChannelOverview *ptr);

    /** List of monitors that will each listen out a channel entry */
    t_monitorentrylist r_writeinfo;

    /** Callback for new entry */
    void entryAdded(const ChannelEntryInfo &i);

    /** Callback for removal */
    void entryRemoved(const ChannelEntryInfo &i);
  };

  /** Monitoring object, monitors the channel with entries signalling
      written entry changes. Each node will have an entry in this
      channel */
  WatchWriteInfo watch_writeinfo;

  /** Channel for count requests */
  ChannelWriteToken w_countreq;

  /** Channel for count results */
  ChannelReadToken r_countres;

  /** Channel for monitor requests */
  ChannelWriteToken w_monitorreq;

  /** Channel for monitor results */
  ChannelReadToken r_monitorres;

public:
  /** Per-channel collection of gathered information */
  struct ChannelInfoSet
  {
    /** Name of the channel, cached */
    std::string name;

    /** Channel number */
    unsigned chanid;

    /** Channel has an entry in zero */
    bool accessfromzero;

    /** Per-entry information */
    struct EntryInfoSet
    {
      /** Reader id counter, for cleaned up/added readers */
      unsigned readerid;

      /** Simply copy an entry info object */
      ChannelWriteInfo wdata;

      /** place for the write index */
      uchan_seq_id_t seq_id;

      /** If opened, a channel data monitor */
      ChannelDataMonitor *monitor;

      /** per reader information */
      struct ReadInfoSet
      {
        /** info ID */
        unsigned readerid;

        /** data */
        ChannelReadInfo rdata;

        /** sequence id */
        uchan_seq_id_t seq_id;

        /** Constructor */
        ReadInfoSet(unsigned readerid, const ChannelReadInfo &rdata);
      };

      /** List type */
      typedef std::list<std::shared_ptr<ReadInfoSet>> readerlist_t;

      /** Organise with all client data */
      readerlist_t rdata;

      /** Constructor */
      EntryInfoSet(const ChannelWriteInfo &wdata);

      /** Get an iterator to a reader */
      readerlist_t::const_iterator getReader(unsigned readerid) const;
    };

    /** List of entries */
    std::vector<std::shared_ptr<EntryInfoSet>> entries;

    /** Constructor */
    ChannelInfoSet(const std::string &name, unsigned chanid, bool accesszero);
  };

protected:
  /** wait period for checking counts */
  unsigned delay_countcollect;

private:
  /** Time-based check */
  AperiodicAlarm count_check;

  /** Callback object for incoming count reports. */
  Callback<ChannelOverview> cb1;

  /** Activity for count processing. */
  ActivityCallback do_count;

  /** Callback object for incoming count reports. */
  Callback<ChannelOverview> cb2;

  /** Activity for count processing. */
  ActivityCallback do_monitor;

public:
  /** type for of info sets */
  typedef std::vector<std::shared_ptr<ChannelInfoSet>> infolist_t;

protected:
  /** list of info sets */
  infolist_t infolist;

  /** temporary list of incoming ChannelReadInfo that waits for
      a ChannelWriteInfo item to open up */
  std::list<ChannelReadInfo> tmplist;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the parameter table. */
  static const ParameterTable *getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  ChannelOverview(Entity *e, const char *part, const PrioritySpec &ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~ChannelOverview();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec &ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int> &i);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** accept information on entry reading. */
  void processReadInfo(const TimeSpec &ts, ChannelReadToken *&r);

  /** accept information on newly written entries */
  void processWriteInfo(const TimeSpec &ts, ChannelReadToken *&r);

  /** incoming count data */
  void processCount(const TimeSpec &ts);

  /** incoming channel monitor data */
  void processMonitorData(const TimeSpec &ts);

protected:
  /** update model */
  virtual void reflectChanges(unsigned channelid);
  /** update model */
  virtual void reflectChanges(unsigned channelid, unsigned entryid);
  /** update model */
  virtual void reflectChanges(unsigned channelid, unsigned entryid,
                              unsigned readerid);
  /** update counts */
  virtual void reflectCounts();

  /** update counts for a specific channel */
  virtual void reflectCounts(unsigned chanid);

  /** redraw view */
  virtual void showChanges();

  /** refresh count numbers */
  void refreshCounts();

private:
  /** Helper function for processing read information */
  unsigned _processReadInfo(const ChannelReadInfo &data);

  /** Helper function for processing count overview results */
  void _processCount(const ChannelCountResult &cnt, const GlobalId &ori);

public:
  /** Call from an opened monitor for fresh data */
  void refreshMonitor(unsigned channelno, unsigned entryno);

  /** Close the monitor */
  virtual void closeMonitor(unsigned channelno, unsigned entryno);

  /** Get the channel name */
  const std::string &getChannelName(unsigned channelno) const;

protected:
  void setMonitor(unsigned channelno, unsigned entryno,
                  ChannelDataMonitor *monitor = NULL);
};

DUECA_NS_END

#endif
