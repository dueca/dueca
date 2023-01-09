/* ------------------------------------------------------------------   */
/*      item            : NetUseOverview.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Mon Apr 19 23:01:51 2021
        category        : header file
        description     :
        changes         : Mon Apr 19 23:01:51 2021 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NetUseOverview_hxx
#define NetUseOverview_hxx

// include the dusime header
#include <dueca.h>

// include headers for functions/classes you need in the module
#include "dueca_ns.h"
#include <Callback.hxx>
#include <CallbackWithId.hxx>
#include <ChannelReadToken.hxx>
#include <Module.hxx>
#include <NetCapacityLog.hxx>
#include <NetTimingLog.hxx>
#include <fstream>
#include <ChannelWatcher.hxx>

DUECA_NS_START;

/** A module that gathers network usage statistics.

    The DUECA udp communication master can produce statistics on
    network use; time needed for communication and amount of data
    sent. This module gathers and prints these to file. Derived
    modules are available with gtk2 and gtk3 interfaces for a graphic,
    real-time overview of network use.

    The instructions to create a module of this class from the Scheme
    script are:

    \verbinclude net-use-overview.scm
 */
class NetUseOverview: public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef NetUseOverview _ThisModule_;

private: // simulation data
  // declare the data you need in your simulation

  /** File with net timing summaries */
  std::ofstream                     nettiming_file;

  /** File with network load summaries */
  std::ofstream                     netload_file;

private: // channel access

  /** Callback object for simulation calculation. */
  Callback<NetUseOverview>          cb0;

  /** Timing data */
  ChannelReadToken                  r_timingdata;

  /** NetLoading object, whenever a channel triggers this, calls back into
      the handling class */
  struct NetLoadEntry {

    /** Access token */
    ChannelReadToken      r_info;

    /** Callback object for collecting change information. */
    CallbackWithId<NetUseOverview,ChannelReadToken*>  cb;

    /** Activity for simulation calculation. */
    ActivityCallback      get_info;

    /** Constructor
        @param ns        Channel name
        @param entry_id  NetUseed entry
        @param classname Datatype class
        @param origin    Writer
        @param ptr       Encapsulating object
        @param h         Callback function
    */
    NetLoadEntry
    (const NameSet& ns, entryid_type entry_id,
     const char* classname, NetUseOverview *ptr,
     void (NetUseOverview::*h)(const TimeSpec& t, ChannelReadToken *&r));
  };

  /** List of netload entries */
  typedef std::list<std::shared_ptr<NetLoadEntry> > t_netloadlist;

  /** To watch the channel with timing and use information */
  struct WatchNetLoadInfo: public ChannelWatcher {

    /** Pointer back to the handling object */
    NetUseOverview* ptr;

    /** Constructor */
    WatchNetLoadInfo(NetUseOverview* ptr);

    /** List of monitors that will each listen out a channel entry */
    t_netloadlist loads;

    /** Callback for new entry */
    void entryAdded(const ChannelEntryInfo& i);
  };

  /** Monitoring object, monitors the channel with entries on net use. */
  WatchNetLoadInfo watch_useinfo;

private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  //PeriodicAlarm        myclock;

  /** Callback object for simulation calculation. */
  Callback<NetUseOverview>  cb1;

  /** Activity for simulation calculation. */
  ActivityCallback      do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  NetUseOverview(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  virtual bool complete();

  /** Destructor. */
  ~NetUseOverview();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const std::vector<int>& i);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities

  /** accept information on load. */
  void processLoadInfo(const TimeSpec& ts, ChannelReadToken *&r);

  /** the method that implements the main calculation. */
  void processNetUseData(const TimeSpec& ts);

  /** callback on first entry validity */
  void channelOpen(const TimeSpec& ts);

protected:
  /** update timing */
  virtual void updateTiming(const NetTimingLog& data);

  /** update load */
  virtual void updateLoad(const NetCapacityLog& data);
};


DUECA_NS_END;

#endif
