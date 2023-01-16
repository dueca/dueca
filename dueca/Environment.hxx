/* ------------------------------------------------------------------   */
/*      item            : Environment.hh
        made by         : Rene' van Paassen
        date            : 980211
        category        : header file
        description     : Environment for communication and scheduling.
                          This environment implements the transport of
                          data, and the notification and scheduling of
                          components, the registry of components and
                          the set-up of transport routes.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Environment_hh
#define Environment_hh

#include <vector>
#include <stringoptions.h>
#include "dstypes.h"
#include "NamedObject.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include "ScriptConfirm.hxx"
#include <boost/scoped_ptr.hpp>

using namespace std;

#include <dueca_ns.h>
DUECA_NS_START;

// class names
class ChannelManager;
class EntityManager;
class ActivityManager;
class TimeSpec;
class NodeManager;
class Activity;
struct DataTimeSpec;
class GuiHandler;
struct ParameterTable;
class CPULowLatency;
class ChannelReadToken;

void environment_main_thread(int phase);


/** This class handles the main thread of controls. It will dip in and
    out of Scheme code, start up the graphics thread and also start up
    the high-priority real-time threads. */
class Environment:
  public ScriptCreatable,
  public NamedObject
{
public: // scheme connectivity
  SCM_FEATURES_DEF;

private:
  /** points to the sole instance of this singleton. */
  static Environment *instance;

  /** Different options for running. */
  enum RunMode {
    SingleThread,   /**< Single threaded running, not adviseable for
                       production environments, ok for testing. */
    MultiThread,     /**< Preferred run mode for real-time
                       simulations. */
    FastTime,       /**< For off-line simulations, just run asap. Note
                       that speed of time depends on computer. */
  };

  /** run mode; multithread or not. */
  RunMode run_mode;

  /** Data type to remember scheduling priorities for activity
      managers. */
  struct SchedPriority {
    /** Selected mode. */
    int sched_mode;
    /** Priority within the mode. */
    int sched_prio;
    /** Constructor. */
    SchedPriority(int mode, int prio);
  };

  /** Scheduling priorities for the different activity managers. */
  list<SchedPriority> sched_priorities;

  /** dummy parameter. */
  int rt_mode;

  /** locale name, if explicitly configured */
  std::string env_locale;

  /** the dog managing the creation and starting of entities. */
  EntityManager *entity_manager;

  /** the vector with the activity managers. */
  vector<ActivityManager*> activity_manager;

  /** the cat checking that all nodes are up. */
  NodeManager *node_manager;

  /** an assorted list of callback for core components that wish to be
      notified when all of DUECA is up */
  std::list<GenericCallback*> call_when_up;

  /** the highest priority possible, no of activity managers + 1. */
  int highest_priority;

  /** holds the highest priority of currently scheduled Activities,
      only used in single thread mode */
  int current_highprio;

  /** a flag to determine whether the environment is in its autonomous
      control loop, together with a condition variable and mutex. */
  volatile bool in_control;

  /** This indicates that we are running multithread real-time */
  volatile bool running_multithread;

  /** This indicates that we are running full speed. */
  volatile bool init_complete;

  /** This is set to true, to indicate that the higher-level threads
      still have to be started */
  bool need_to_start_others;

  /** This is set to false after activation of statuscheck by
      entity manager has been started. */
  bool statuscheck;

  /** The time (as returned from a TimeKeeper::ReadClock() call, at
      which the real-time (re)connection with the other nodes should be
      attempted. */
  int64_t rt_start_time;

  /** scoped pointer for object that sets CPU to low latency mode */
  boost::scoped_ptr<CPULowLatency> cpu_lowlatency;

  /** An enumerated type to control the coordination between the GUILE
      reading thread and the other threads. */
  enum ScriptCoordination {
    Wait,         /**< wait with returning to scheme. */
    Read,         /**< return to scheme and read more data. */
    Exit          /**< exit the program. */
  };

  /** The flag to control thread coordination. */
  volatile ScriptCoordination create_cmd;

  /** Event channel over which confirmation for additional read action
      comes. */
  ChannelReadToken     *t_moreconf;

  /** If additional data has been read, we have to wait for
      copying. This is the callback function. */
  Callback<Environment> cbw;

  /** The waiting action. */
  ActivityCallback* wait_additional;

  /** The function. */
  void waitAdditional(const TimeSpec& ts);

  /** A friend function that initiates destruction of most objects. */
  friend void destruct_most(int e_val, void* arg);

public:
  /** Obtain reports from all activity managers, after an exit(n)
      call, to inform about who is doing what. */
  void activityManagerReports();

  /** Ask whether running already in multi-threading mode. */
  inline bool runningMultiThread() const {return running_multithread;}

  /** Ask whether initialisation phase is over */
  inline bool initialisationComplete() const {return init_complete;}

private:
  /** the name of the graphic interface library used (if any). */
  vstring graphic_interface;

  /** The option to use depth buffering on this grapic interface. */
  int    graphic_depth_buffer_size;

  /** Size of the stencil buffer. */
  int    graphic_stencil_buffer_size;

  /** Initialising Xlib lock */
  bool   xlib_lock;

  /** share gl contexts */
  bool   share_gl_contexts;

  /** The object that intitialises and controls the graphics
      library. There is also a GuiHandler for graphics library "none",
      so that with or without Gui, the code acts the same. */
  GuiHandler* gui_handler;

  /** interval for gui commands. */
  double command_interval;

  /** lead time for gui commands. */
  double command_lead;

  /** interval for gui commands, converted to ticks. */
  TimeTickType command_interval_ticks;

  /** lead time for gui commands, converted to ticks. */
  TimeTickType command_lead_ticks;

public:

  /** Constructor. */
  Environment();

  /** Complete method, called after construction and parameter
      insertion. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Obtain a pointer to the parameter table. */
  static const ParameterTable* getParameterTable();

  /** Destructor. */
  ~Environment();

  /** Call to advance the Environment to the next stage of DUECA
      running.
      \param stage           running mode. 0: right after running the
                             Scheme script, the various singletons
                             needed for DUECA are completed. At the
                             end of this, control returns back to
                             Scheme/the calling process.
                             1: Dueca starts to tick slowly, and the
                             nodes are being connected. When the nodes
                             are connected, model data is copied over
                             dueca.
                             2: Graphics is started up, and the
                             modules are created. After a small
                             startup time the DUECA nodes start
                             real-time, multithread running.
                             3: More module data has been read. */
  void proceed(int stage);

public:
  /** Method which may run in a separate thread. This starts up the
      GUI in use, asks the EntityManager to create the modules, and
      enters the graphics control loop. */
  void graphicRun();

public:
  /** Method that is called from the graphics control loop. Does the
      activities of the low-priority manager, and -- when the time is
      right, also starts up the threads for the higher-priority
      managers. */
  void doLoop();

private:
  /** Copying is not allowed. */
  Environment(const Environment& e);

  /** Neither is assignment. */
  Environment& operator = (const Environment& e);

public:

  /** the function that returns the sole instance of this singleton. */
  inline static Environment *getInstance() {
    if (instance == NULL) {
      cerr << "Environment says: Check your dueca.cnf" << endl;
      assert(0);
      std::exit(1);      // configuration wrong
    }
    return instance;
  }

  /** schedule an Activity. */
  void propagateTriggers(unsigned prio);

  /** wake a manager */
  void wakeActivityManager(unsigned  ii);

public:
  /** request data on the shared memory, or elsewhere if no shmem is
      used. */
  DataTimeSpec* requestDataSpace(int no_of_copies);

  /** Release data on the shared memory again. */
  void releaseDataSpace(DataTimeSpec* zero, int no_of_copies);

private:
  /** This method transfers control to the CSE. The CSE calls all
      Activities, waits for the next time frame, calls them again,
      etc. The method only returns *after* the quit method has been
      called. */
  void control();

public:
  /** This method stops the time looping by the CSE. */
  void quit();

public:
  /** this method does one update sweep of the CSE. invokes all
      Activities on the stack. */
  int update();

  /** this method clears the activities of the lowest-priority
      activity manager. */
  int doIdle();

  /** This reads -- if possible -- the specified file and uses this to
      update the running modules. */
  void readMod(const vstring& fname);

  /** Tell that this is a basic object of dueca. */
  ObjectType getObjectType() const {return O_Dueca;}

  /** Return the highest prirority. */
  inline int getHighestPrio() { return highest_priority; }

  /** Schedule a callback pointer */
  void informWhenUp(GenericCallback* cb);

  /** Return a pointer to a specific activity manager. */
  inline ActivityManager* getActivityManager(int prio)
  { return activity_manager[prio];}

  /** Specify (with a name) the graphic interface to be used. */
  inline void setGraphicInterface(const char* iface)
  { graphic_interface = iface; }

  /** Read out which graphic interface was chosen. */
  inline const char* getGraphicInterface() const
  {return graphic_interface.c_str(); }

  /** Return the size of the depth buffer. */
  inline bool getGraphicDepthBufferSize() const
  { return graphic_depth_buffer_size; }

  /** Return the size of the stencil buffer. */
  inline bool getGraphicStencilBufferSize() const
  { return graphic_stencil_buffer_size; }

  inline bool getShareGLContexts() const
  { return share_gl_contexts; }

  /** Member call. */
  bool setGraphicUseDepthBuffer(const bool& v);

  /** Deprecated call, selects or de-selects multi-thread mode. */
  bool setMultiThread(const bool& v);

  /** Call to select any of a number of possible run modes. */
  bool setRunMode(const vstring& mode);

  /** Call to add non-real-time threads. */
  bool setAMNice(const vector<int>& levels);

  /** Call to add real-time threads with RR scheduling. */
  bool setAMRoundRobin(const vector<int>& levels);

  /** Call to add real-time threads with FIFO scheduling. */
  bool setAMFiFo(const vector<int>& levels);

  /** Call to add real-time threads with RTAI scheduling. */
  bool setAMRTAI(const vector<int>& levels);

  /** Call to add real-time threads with XENOMAI scheduling. */
  bool setAMXENO(const vector<int>& levels);

  /** Get the current gui handler. */
  inline GuiHandler* getActiveGuiHandler() {return gui_handler; }

  /** Command interval, for if other interfaces want to use this */
  inline unsigned getCommandInterval() { return command_interval_ticks; }

  /** Command interval, for if other interfaces want to use this */
  inline unsigned getCommandLead() { return command_lead_ticks; }
};

DUECA_NS_END;
#endif
