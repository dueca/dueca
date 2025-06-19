/* ------------------------------------------------------------------   */
/*      item            : Environment.cxx
        made by         : Rene' van Paassen
        date            : 980211
        category        : body file
        description     :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define Environment_cc

#include <dueca-conf.h>
#include <unistd.h>
#include "Environment.hxx"
#include "NodeManager.hxx"
#include "EntityManager.hxx"
#include "TimeSpec.hxx"
#include "ChannelManager.hxx"
#include "Ticker.hxx"
#include "ScriptInterpret.hxx"
#include "ObjectManager.hxx"
#include "ActivityManager.hxx"
#include <oddoptions.h>
#include <TimeKeeper.hxx>
#include <Condition.hxx>
#include <Su.hxx>
#include <PackerManager.hxx>
#include <ParameterTable.hxx>
#include <LogConcentrator.hxx>
#include <LogPoint.hxx>
#include <LogPoints.hxx>
#include <ActivityDescription.hxx>
#include <ActivityDescriptions.hxx>
#include <DuecaEnv.hxx>
#include "CPULowLatency.hxx"
#include <locale>
#include <sstream>
#include <dueca/ChannelReadToken.hxx>
#include <sys/resource.h>
#define W_CNF
#define I_SYS
#define W_SYS
#define E_SYS
#include <debug.h>
#include "dueca_assert.h"
// include (possible) graphic interfaces
#include "GuiHandler.hxx"

#ifdef USE_POSIX_THREADS
#include <pthread.h>
#endif
#ifdef HAVE_MLOCKALL
#include <sys/mman.h>
#endif

#include <algorithm>
#include <cstdlib>

#define DO_INSTANTIATE
#include "VarProbe.hxx"
#include "MemberCall.hxx"
#include <Callback.hxx>
#include <InformationStash.ixx>
#include <debprint.h>

/* PRIORITY_BASE is a base number of priority levels that is
   added to the minimum priority level, and at that level the
   no 1 priority will start.

   On QNX, max prio level is 63, and "normal" processes are seen until
   prio level 21. To be on the safe side, start at 22.

   On linux, max prio level is 100. 10 makes a good start, since there are
   normally no real-time priority processes running on Linux. */
#ifdef __QNX__
#define PRIORITY_BASE 22
#else
#define PRIORITY_BASE 10
#endif

DUECA_NS_START

#ifdef USE_POSIX_THREADS
/** A condition variable for the thread coordination. */
Condition create_control("gui/script thread coordination");

#if defined(USE_POSIX_THREADS)
pthread_t activity0_thread;
#endif
#endif

InformationStash<ActivityDescription> &Activity_stash();

static bool checkGuiName(const string &guiname)
{
  return (GuiHandler::all().find(guiname) != GuiHandler::all().end());
}

static GuiHandler *selectGuiHandler(const string &guiname)
{
  if (checkGuiName(guiname)) {
    return GuiHandler::all()[guiname];
  }
  cerr << "Do not have graphic interface \"" << guiname << "\" linked in"
       << endl;
  std::exit(1);
}

Environment *Environment::instance = NULL;

Environment::SchedPriority::SchedPriority(int mode, int prio) :
  sched_mode(mode),
  sched_prio(prio)
{
  //
}

// new constructor, no parameters, relies on default parameters and
// the "complete" method
Environment::Environment() :
  NamedObject(
    NameSet("dueca", "Environment", ObjectManager::single()->getLocation())),
  run_mode(MultiThread),
  highest_priority(0),
  current_highprio(0),
  running_multithread(false),
  init_complete(false),
  need_to_start_others(true),
  statuscheck(true),
  exitcode(0),
  create_cmd(Wait),
  cbw(this, &Environment::waitAdditional),
  wait_additional(NULL),
  graphic_interface("none"),
  graphic_depth_buffer_size(16),
  graphic_stencil_buffer_size(4),
  xlib_lock(false),
  share_gl_contexts(false),
  gui_handler(NULL),
  command_interval(0.1),
  command_lead(0.1),
  command_interval_ticks(1),
  command_lead_ticks(0)
{
  instance = this;
  sched_priorities.push_back(SchedPriority(SCHED_OTHER, 0));
}

bool Environment::complete()
{
  {
    /* Check the set locale, and issue a warning if needed */
    setlocale(LC_NUMERIC, "");
    double f1, f2;
    {
      std::stringstream tester;
      tester.imbue(std::locale(setlocale(LC_NUMERIC, NULL)));
      tester << "1,2";
      tester >> f1;
    }
    {
      std::stringstream tester;
      tester.imbue(std::locale(setlocale(LC_NUMERIC, NULL)));
      tester << "1.2";
      tester >> f2;
    }
    if (f1 != 1.2 && f2 == 1.2) {
      // as expected
    }
    else {
      if (env_locale == setlocale(LC_NUMERIC, NULL)) {
        /* DUECA system.

           You have selected a non-standard locale. Ensure your
           datafiles are all correctly formatted.
         */
        I_SYS("Note that you are using an alternative locale that can affect"
              << " numeric input and output: " << env_locale);
      }
      else {
        /* DUECA system.

           LOCALE settings may determine the way in which data files
           are read; using ',' instead of '.', etc. Your system has a
           non-standard locale, and will use comma for the decimal
           point. The regular fix is to set a locale using decimal
           point. If you insist on a non-standard locale, set this in
           dueca_cnf.py, and ensure your datafiles are all correctly
           formatted.
         */
        E_SYS("Using a non-standard numeric locale "
              << setlocale(LC_NUMERIC, NULL)
              << ". Change locale settings (best solution) or allow in "
                 "dueca_cnf.py, and caution with data IO!");
        std::exit(1);
      }
    }
  }

  // at this point, the following may have been adjusted:
  // - graphic_interface
  // - multithread
  // - highest_priority
  // - list of scheduling priorities
  // so check:
  if (!checkGuiName(graphic_interface)) {
    /* DUECA system.

      A graphical user interface mode has been requested tht is not
      available in this DUECA process. Check the configburation file
      and compilation options on the affected node
    */
    E_CNF("Environment: Gui name \"" << graphic_interface << "\" not known");
    return false;
  }

  if (highest_priority < 0 ||
      (highest_priority && sched_priorities.size() > 1)) {
    /* DUECA system.

      The priorities for the activity managers should either be
      specified by means of highest priority level or by means of
      individual specification. You are using a combination of both
      methods, modify the configuration file on the affected node.
    */
    E_CNF("Environment: Priority specification error");
    return false;
  }

  // complete priority levels if only highest prio specified
  for (int p = 1; p <= highest_priority; p++) {
    sched_priorities.push_back(SchedPriority(
      SCHED_FIFO, sched_get_priority_min(SCHED_FIFO) + PRIORITY_BASE + p));
  }
  highest_priority = sched_priorities.size() - 1;

  // first update the max prio, so priority specifications (needed by
  // the activity managers) can be made for higher priorities
  ActivityManager::setMaxPrio(highest_priority);

  // create the activity managers
  int prio = 0;
  for (list<SchedPriority>::const_iterator ii = sched_priorities.begin();
       ii != sched_priorities.end(); ii++) {
    activity_manager.push_back(
      new ActivityManager(prio, ii->sched_mode, ii->sched_prio));
    prio++;
  }

  wait_additional = new ActivityCallback(getId(), "wait additional model copy",
                                         &cbw, PrioritySpec(0, 0));

  return true;
}

const char *Environment::getTypeName() { return "Environment"; }

const ParameterTable *Environment::getParameterTable()
{
  static const ParameterTable table[] = {
    { "multi-thread",
      new MemberCall<Environment, bool>(&Environment::setMultiThread),
      "(#t or #f), determines whether to use multi-threading (deprecated)!" },
    { "highest-priority",
      new VarProbe<Environment, int>(
        REF_MEMBER(&Environment::highest_priority)),
      ">=0, highest-priority activity manager (also deprecated)" },
    { "dummy-value",
      new VarProbe<Environment, int>(REF_MEMBER(&Environment::rt_mode)),
      "dummy, for compatibility with old .cnf files" },
    { "graphic-interface",
      new VarProbe<Environment, vstring>(
        REF_MEMBER(&Environment::graphic_interface)),
      "\"none\", \"gtk\", \"glut\", \"fltk\", \"gtk+fltk-gl\", \"gtk2\", "
      "\"gtk3\"\n"
      "determines which graphic interface code is run. Note that the\n"
      "availability of graphic interfaces can depend on your link options." },
    { "graphic-use-depth-buffer",
      new MemberCall<Environment, bool>(&Environment::setGraphicUseDepthBuffer),
      "take a depth buffer with default size (16bits)" },
    { "graphic-depth-buffer-size",
      new VarProbe<Environment, int>(
        REF_MEMBER(&Environment::graphic_depth_buffer_size)),
      "required size of the depth buffer, default=16" },
    { "graphic-stencil-buffer-size",
      new VarProbe<Environment, int>(
        REF_MEMBER(&Environment::graphic_stencil_buffer_size)),
      "required size of the stencil buffer, default=4" },
    { "run-mode",
      new MemberCall<Environment, vstring>(&Environment::setRunMode),
      "specify run mode of DUECA, either \"single\" for single thread, \n"
      "\"multi\" for multi-thread or \"fast\" for a fast-time simulation" },
    { "priority-nice",
      new MemberCall<Environment, vector<int>>(&Environment::setAMNice),
      "Add activity priority level with nice value, values should be between\n"
      "-20 (not very nice, eat up most of the processor), through 0 (deflt)\n"
      "to 20 (very nice, do not impose at all. Note that priority level 0 is\n"
      "include by default." },
    { "priority-rr",
      new MemberCall<Environment, vector<int>>(&Environment::setAMRoundRobin),
      "Add activity priority level with round-robin scheduling\n"
      "Linux priority levels currently go from 1 to 100" },
    { "priority-fifo",
      new MemberCall<Environment, vector<int>>(&Environment::setAMFiFo),
      "Add activity priority level with first-in, first-out scheduling\n"
      "See above for explanation on priority levels" },
    { "x-multithread-lock",
      new VarProbe<Environment, bool>(REF_MEMBER(&Environment::xlib_lock)),
      "initialise the Xlib lock, to allow for multi-threaded access to X\n"
      "with this, you might be able to run an Ogre or SDL window in another\n"
      "thread. Don't count on mixing threads in gtk or glut windows" },
    { "share-gl-contexts",
      new VarProbe<Environment, bool>(
        REF_MEMBER(&Environment::share_gl_contexts)),
      "share the gl context between the different GL windows. Note that this\n"
      "capability may not be present for certain windowing toolkits" },
    { "command-interval",
      new VarProbe<Environment, double>(
        REF_MEMBER(&Environment::command_interval)),
      "Interval (in sec) for rounding off module start and stop commands,\n"
      "default is 0.1." },
    { "command-lead",
      new VarProbe<Environment, double>(REF_MEMBER(&Environment::command_lead)),
      "Lead time (in sec) needed for module start and stop commands,\n"
      "default is 0.1." },
    { "locale",
      new VarProbe<Environment, std::string>(&Environment::env_locale),
      "If you want to use a non standard (. <> decimal point) locale, this\n"
      "needs to be specified in the configuration. Careful with datafiles!" },
    { NULL, NULL, "The Environment manages the control logic of dueca." }
  };

  return table;
}

Environment::~Environment()
{
  //
}

bool Environment::setGraphicUseDepthBuffer(const bool &y)
{
  if (y)
    graphic_depth_buffer_size = 16;
  else
    graphic_depth_buffer_size = 0;
  return true;
}

bool Environment::setMultiThread(const bool &v)
{
  // this call is a relic to accomodate simulations that set/reset
  // multi-thread mode.
  if (v) {
    run_mode = MultiThread;
  }
  else {
    run_mode = SingleThread;
  }
  return true;
}

bool Environment::setRunMode(const vstring &mode)
{
  if (mode == vstring("single")) {
    run_mode = SingleThread;
  }
  else if (mode == vstring("multi")) {
    run_mode = MultiThread;
  }
  else if (mode == vstring("fast")) {
    run_mode = FastTime;
  }
  else {
    /* DUECA system.

       A run mode has been configured that does not exist. Adjust the
       DUECA configuration on this node. */
    E_CNF("Environment: No such run-mode \"" << mode << '"');
    return false;
  }
  return true;
}

bool Environment::setAMNice(const vector<int> &level)
{
  for (unsigned int ii = 0; ii < level.size(); ii++) {
    if (level[ii] < -20 || level[ii] > 20) {
      /* DUECA system.

         An error in your activity level configuration, this "nice"
         level is not available. Adjust configuration file on the
         affected node. */
      E_CNF("Environment: Illegal nice level " << level[ii]);
      return false;
    }
    sched_priorities.push_back(SchedPriority(SCHED_OTHER, level[ii]));
  }
  return true;
}

bool Environment::setAMRoundRobin(const vector<int> &level)
{
  for (unsigned int ii = 0; ii < level.size(); ii++) {
    if (level[ii] < sched_get_priority_min(SCHED_RR) ||
        level[ii] > sched_get_priority_max(SCHED_RR)) {
      /* DUECA system.

        An error in your activity level configuration, this
        "round-robin" level is not available. Adjust configuration
        file on the affected node.
      */
      E_CNF("Environment: Illegal RR priority " << level[ii]);
      return false;
    }
    sched_priorities.push_back(SchedPriority(SCHED_RR, level[ii]));
  }
  return true;
}

bool Environment::setAMFiFo(const vector<int> &level)
{
  for (unsigned int ii = 0; ii < level.size(); ii++) {
    if (level[ii] < sched_get_priority_min(SCHED_FIFO) ||
        level[ii] > sched_get_priority_max(SCHED_FIFO)) {
      /* DUECA system.

        An error in your activity level configuration, this "fifo"
        level is not available. Adjust configuration file on the
        affected node.
      */
      E_CNF("Environment: Illegal FIFO priority " << level[ii]);
      return false;
    }
    sched_priorities.push_back(SchedPriority(SCHED_FIFO, level[ii]));
  }
  return true;
}

#if defined(USE_POSIX_THREADS)
static void *Environment_graphicRun(void *arg)
{
  static_cast<Environment *>(arg)->graphicRun();
  return NULL;
}
#endif

/** In case of single-thread running, this method is called directly
    from within proceed. The complete GUILE calls are then on the
    stack, and the method proceeds with some model code, and then
    enters the graphics toolkit main loop. When called in a
    multi-thread case, the thread doing this work is "clean", and only
    does the graphics work. This leaves the script thread free to read
    more configuration data, if this is so desired. */
void Environment::graphicRun()
{
  // first start by activating the graphics library (if we have one,
  // running without graphics ("none") is the same as running with, a
  // dummy handler is called)
  gui_handler = selectGuiHandler(graphic_interface);
  gui_handler->init(xlib_lock);

  // now directly ask the entitymanager in this node to create all the
  // modules. Completion is done later, then any intensive work (like
  // loading bitmaps, reading MBs of data, opening graphics windows,
  // initialising commmunication stacks) can be done.
  entity_manager->createEntityModules();

#if 0
  // once that is done, the entity manager can start collecting
  // information about the entities.
  entity_manager->startStatusCheck();
#endif

  // now enter the graphics control loop. The Environment will get
  // called again in the "idle" or callback loop, via doLoop

  /* DUECA system.

    Indicates that the graphics interaction code is now started.
  */
  I_SYS("Environment: Passing control to graphics lib");
  gui_handler->passControl();

  // the call above stays in the gui handler until the program exits.
  // However, glut (currently, apparently) cannot return from its main
  // loop call, so with Glut we never get here. Instead, the GLUT
  // thread will call exit directly

  /* DUECA system.

    Indicates that the graphics interaction code has ended.
  */
  I_SYS("Environment: Returned from graphics loop");

  if (run_mode == MultiThread) {
    // returned from the gui, and therefore also from possibly creation
    // and later joining of real-time threads. Now join this thread
    // again with the script thread, which should be waiting on the
    // condition in proceed, case 2
    create_control.enterTest();   // lock mutex
    create_cmd = Exit;            // tell we want to exit

    /* DUECA system.

       Sending the command to exit the process.
    */
    I_SYS("Environment: Signalling exit");
    create_control.signal();      // give signal
    create_control.leaveTest();   // leave

    // we exit this routine here, and thereby exit the thread
    pthread_exit(NULL);
  }

  // non-multithread, returns after the call in proceed, and make sure
  // "non-crew" is jettisoned
  create_cmd = Exit;
}

/** The number of cycles to run single-thread, before starting
    multithread running (if this is selected). */
static int n_comm_cycles = 10;

/** This method is called from within the graphics toolkit main
    loop. At the start, dueca is not running and there is a waiting
    period, equal for all dueca nodes. After this period, the
    higher-priority threads are started. */
void Environment::doLoop()
{
  // also do the complete calls for modules
  DuecaEnv::callComplete();

  // and start the status check; after the callComplete, because then the
  // graphics windows for status feedback are open
  if (statuscheck) {
    entity_manager->startStatusCheck();
    statuscheck = false;
  }

  switch (run_mode) {
  case MultiThread:

    if (need_to_start_others) {

      if (TimeKeeper::readClock() > rt_start_time) {

        /* DUECA system.

          Start of real-time running.
        */
        I_SYS("Environment: Time to start real-time running");

#ifdef NEW_LOGGING
        // start the logging. A logconcentrator for each node
        LogConcentrator::single().initialise(TimeSpec(0.0, 0.2));
        if (static_node_id == 0) {
          LogPoints::single().initialise();
        }
#endif
        Activity_stash().initialise();
        if (static_node_id == 0) {
          ActivityDescriptions::single().initialise();
        }

        // do start the others
        Su::single().acquire();

        /* DUECA system.

          Start of activity scheduling and processing by activity
          managers.
        */
        I_SYS("Environment: Starting activity managers");
        for (int ii = 1; ii <= highest_priority; ii++) {
          activity_manager[ii]->startDoActivities();
          usleep(100000);
        }

        // start the Ticker in autonomous mode this needs:
        // 1 -- a checktick to jump ahead to the current time and to one
        //      logical time advance
        // 2 -- a runTick to flag that real-time ticking is needed
        // 3 -- a startTicking to indicate that the ticker has to run
        //      itself in a high-prio thread

        /* DUECA system.

          Starting real-time clock.
        */
        I_SYS("Environment: Setting ticker to autonomous");
        Ticker::single()->checkTick();
        Ticker::single()->runTick();
        Ticker::single()->startTicking();

        // done starting, remember this
        need_to_start_others = false;
        running_multithread = true;
        init_complete = true;

        // revert to original user for this (the graphics) thread
        // do not do this on QNX, since we will use all kinds of IO
        // (and thus need io permission), and this will work for the
        // entire process!
#if !defined(__QNXNTO__)
        //        Su::single().revert();
#endif
      }
      else {

        // run everything single-thread for some cycles, just to get all
        // confirmation across to the others too
        if (n_comm_cycles) {

          // wait some time, to prevent hugging the computer
          usleep(10000);

          // jump ahead to the current time, and do one logical time advance
          Ticker::single()->checkTick();
          activity_manager[0]->scheduleAll();

          // handle all activities triggered for this time advance
          while (current_highprio >= 0) {
            current_highprio--;
            activity_manager[int(current_highprio) + 1]->doActivities();
          }

          // one less waiting cycle to do
          n_comm_cycles--;
        }
        return;
      }
    }

    // do the actions of the low-priority manager. This will clear the
    // activities on the number 0 stack (with a maximum number in one
    // go). If there are no activities, this waits until something
    // comes in, then works and returns, or until the next trigger by
    // the ticker, whichever comes first.
    activity_manager[0]->doActivities0();
    break;

  case SingleThread:

    // imitate the multithread init stuff
    if (need_to_start_others && TimeKeeper::readClock() > rt_start_time) {

      // complete calls of modules
      DuecaEnv::callComplete();

#ifdef NEW_LOGGING
      // start the logging. A logconcentrator for each node
      LogConcentrator::single().initialise(TimeSpec(0.0, 0.2));
      if (static_node_id == 0) {
        LogPoints::single().initialise();
      }
#endif
      Activity_stash().initialise();
      if (static_node_id == 0) {
        ActivityDescriptions::single().initialise();
      }
      need_to_start_others = false;
    }

    // A single wait time, equal to the compatible tick time.
    // note that the running will be slower than real-time
    usleep(int(Ticker::single()->getTimeGranule() *
               Ticker::single()->getCompatibleIncrement() * 1000000));

    // run the required number of ticks
    for (int l = Ticker::single()->getCompatibleIncrement(); l > 0;
         l -= Ticker::single()->getBaseIncrement()) {
      Ticker::single()->newTick();

      // do the actions of all managers
      // one call to the ticker; any new stuff because of elapsed time is
      // inserted here
      // Ticker::single()->checkTick();
      activity_manager[0]->scheduleAll();

      while (current_highprio >= 0) {
        current_highprio--;
        activity_manager[int(current_highprio) + 1]->doActivities();
      }
    }
    break;

  case FastTime:

    // go to new time interval
    Ticker::single()->newTick();
    activity_manager[0]->scheduleAll();

    // process all work there
    while (current_highprio >= 0) {
      current_highprio--;
      activity_manager[int(current_highprio) + 1]->doActivities();
    }

    break;
  }
}

void Environment::readMod(const vstring &fname)
{
  // add the file information to the guile reading:
  if (ScriptInterpret::single()->readAdditionalModuleConf(fname)) {

    // works, so now wait for confirm
    /*  wait_additional.setTrigger(*Ticker::single());
    wait_additional.switchOn(TimeSpec(SimTime::getTimeTick(),
    SimTime::getTimeTick())); */
  }
  else {

    /* DUECA system.

       There was an error reading a file with additional
       model configuration.
    */
    W_CNF("Environment: Error reading file " << fname);
  }
}

void Environment::informWhenUp(GenericCallback *cb)
{
  call_when_up.push_back(cb);
}

void Environment::waitAdditional(const TimeSpec &ts)
{
  if (ScriptInterpret::single()->modelCopied()) {

    // stop this activity
    // wait_additional.switchOff(ts);
    // wait_additional.removeTrigger();

    // signal scheme reading
    create_control.enterTest();
    create_cmd = Read;
    create_control.signal();
    while (create_cmd == Read) {
      /* DUECA system.

        Thread 0, with activitymanager for priority 0, will halt, and
        wait until a model configuration script has been processed.
      */
      I_SYS("Environment: Prio 0 thread waiting on script reading");
      create_control.wait();
    }
    create_control.leaveTest();

    /* DUECA system.

      A model configuration script has been processed by the thread
      with the script language, thread 0, with activitymanager for
      priority 0 will now continue scheduling activities.
    */
    I_SYS("Environment: Prio 0 thread continues");
    entity_manager->createEntityModules();
  }
}

/** atexit function, calls the destructor for everything not named
    dueca. */
#if defined(HAVE_ON_EXIT)
void destruct_most(int e_val, void *arg)
#elif defined(HAVE_ATEXIT)
void destruct_most()
#else
#error "Need one of on_exit or atexit"
#endif
{
#if defined(HAVE_ON_EXIT)
  if (e_val != 0) {
    std::cerr << "exit() was called with a non-zero (error) exit value "
              << e_val << std::endl;
    CSE.activityManagerReports();
    return;
  }
#endif

  /* DUECA system.

    All objects and modules that are not part of the core DUECA system
    are now told to stop and then destructed.
  */
  I_SYS("Environment: Exit called, jettisoning everyone except crew members");
  ObjectManager::single()->destructAllButCrew();
}

extern int *p_argc;
extern char ***p_argv;

void Environment::activityManagerReports()
{
  for (int ii = activity_manager.size(); ii--;) {
    activity_manager[ii]->reportCurrent();
  }
}

static void react_ctrlc(int signum)
{
  /* DUECA system.

    A keyboard interrupt (Ctrl-C) signal was received and
    intercepted. The DUECA process will be stopped in a controlled
    fashion. Note that an additional Ctrl-C will result in immediate
    abort.
  */
  W_SYS("Environment: User signal Ctrl-C");

  // get reports on what was going on, as a hint on what might have
  // been stuck
  CSE.activityManagerReports();

  // initiate a "nice" exit
  NodeManager::single()->breakUp();
}

void Environment::proceed(int stage)
{
  /* DUECA system.

    Control returned from script reading stage.
  */
  I_SYS("Environment: Popped up from script, stage=" << stage);

  // for runing at a reasonably slow pace, but considerably below the
  // timeout values for the communication
  const int waitmsec = 100;

  switch (stage) {
  case 0: // no longer used
    break;

  case 1: {

    // create a node manager. Communicates and checks status of other
    // nodes
    /* DUECA system.

      Creation of the object that monitors and controls the status of
      the different DUECA nodes.
    */
    I_SYS("Environment: Creating the node manager");
    node_manager = new NodeManager(ObjectManager::single()->getLocation(),
                                   ObjectManager::single()->getNoOfNodes());

    // give previously created managers the opportunity to complete
    /* DUECA system.

      All DUECA core objects are given the opportunity to complete their
      initialization.
    */
    I_SYS("Environment: Completing dueca objects");
    ScriptInterpret::single()->completeCreation();
    ObjectManager::single()->completeCreation();
    ChannelManager::single()->completeCreation();
    Ticker::single()->completeCreation();
    for (unsigned int ii = 0; ii < activity_manager.size(); ii++) {
      activity_manager[ii]->completeCreation();
    }

    // if the command interval is specified, pass this to the entity manager
    command_interval_ticks =
      command_interval > Ticker::single()->getDT()
        ? int(round(command_interval / Ticker::single()->getTimeGranule()))
        : Ticker::single()->getBaseIncrement();
    command_lead_ticks =
      command_lead >= 0
        ? int(round(command_lead / Ticker::single()->getTimeGranule()))
        : 3 * Ticker::single()->getBaseIncrement();

    // create an entity manager. This one needs to know the number of nodes
    entity_manager =
      new EntityManager(ObjectManager::single()->getLocation(),
                        ObjectManager::single()->getNoOfNodes(),
                        command_interval_ticks, command_lead_ticks);

    // register an atexit function, which will call object destructors
    // for all non-dueca objects (modules and the comm accessors) to
    // clean up/close fd's etc.
#if defined(HAVE_ON_EXIT) && !defined(ACTIV_NOCATCH)
    if (on_exit(destruct_most, NULL) != 0) {
      perror("on_exit");
    }
#elif defined(HAVE_ATEXIT) && !defined(ACTIV_NOCATCH)
    if (std::atexit(destruct_most) != 0) {
      perror("atexit");
    }
#endif

    // register a function for ctrl-c press by the user. This will
    // print out current activities in all threads, and finishes the
    // program
    struct sigaction act;
    act.sa_handler = react_ctrlc;
    act.sa_flags = SA_RESETHAND;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGINT, &act, NULL) == -1) {
      /* DUECA system.

         The attempt to install a handler for keyboard interrupts
         failed. DUECA functions, but has no facilities to provide
         useful status information on a keyboard interrupt.
      */
      W_SYS("Environment: Cannot specify Ctrl-c handler " << strerror(errno));
    }

    // set the time spec for the waiting action
    /* wait_additional.setTimeSpec
       (PeriodicTimeSpec(0, int(1.0+1.0/Ticker::single()->getDT())));
    */
    // open an event channel that carries confirmations that additional
    // configuration data can be processed
    t_moreconf = new ChannelReadToken(
      getId(), NameSet("dueca", "ScriptConfirm", "processadditional"),
      getclassname<ScriptConfirm>(), 0, Channel::Events, Channel::OnlyOneEntry,
      Channel::JumpToMatchTime);
    // connect as trigger
    wait_additional->setTrigger(*t_moreconf);
    wait_additional->switchOn(TimeSpec(0, 0));

    // start a phase in which the running and ticking is slow, until
    // we are connected to the other nodes
    /* DUECA system.

       This node now enters a slow ticking phase, intended to initiate
       connection with all other nodes in the DUECA process.
    */
    I_SYS("Environment: Slow ticking");

    Ticker::single()->pauseTick();
    int countdown = 10;
    while (!NodeManager::single()->isDuecaComplete()) {

      // I_ SYS("Dueca nodes not yet complete");

      // ticker is not "ticking". The update() will call a checkTick
      // which will produce only one
      // time advance, and update the "last_tick_time". The activities
      // triggered by the checktick are then all despatched.
      update();

      // wait one period
      usleep(waitmsec * 1000);
    }

    /* DUECA system.

       All required nodes have been connected.
    */
    I_SYS("Environment: Dueca nodes now complete");

    // generic clients (currently new udpcom accessors)
    while (call_when_up.size()) {
      call_when_up.front()->operator()(TimeSpec(0, 0));
      call_when_up.pop_front();
    }

    // tell the object manager to send information about the objects
    // created up to now to node 0
    ObjectManager::single()->sendAllToNode0();

    // now read the configuration. The configuration lines will be
    // sent over a channel
    ScriptInterpret::single()->createObjects();

    // make sure the data is copied to the channel
    // and we add a little countdown
    countdown = 10;
    while (!ScriptInterpret::single()->modelCopied() || countdown--) {
      update();

      // I forgot why this checktick is inserted
      Ticker::single()->checkTick();
      usleep(waitmsec * 1000);
    }

    // After this, each node will go its own way,
    // At this point we read the time. By adding 2 seconds to this
    // time, we get the time at which all nodes will attempt
    // re-establishing the communication.
    rt_start_time = TimeKeeper::readClock() + 2 * 1000000;

    /* DUECA system.

       A start message for real-time clocked running has been sent to
       all nodes.
    */
    I_SYS("Environment: Real-time running, two seconds from now ....");
  }
  // now return to scheme to effectuate the model creation
  break;

  case 2:
    // 010618
    // new code for this case. The idea is that we give the gui (or no
    // gui at all) a thread of its own, away from the intricacies of
    // guile. This thread initialises the GUI. Then it runs in
    // single-thread mode until all entities have created their
    // modules, subsequently it starts the gui and keeps on running in
    // single-thread mode a bit more, and only then the other threads
    // are started.

#ifdef USE_POSIX_THREADS
    if (run_mode == MultiThread) {

      // first try to get memory locked.
#ifdef HAVE_MLOCKALL
      // check for begin root, if so, memlock the program
      if (Su::single().isCapable()) {
        Su::single().acquire();

        if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
          /* DUECA timing.

             It was not possible to aacuire memlock and prevent
             swap. If you are deploying a real-time system, consider
             configuring the workstation for real-time running. When
             developing, you may generally ignore this message. */
          W_SYS("Environment: Cannot memlock the DUECA executable: " << strerror(errno));
        }
        else {
          // also try to set the CPU to low-latency
          cpu_lowlatency.reset(new CPULowLatency(0));
        }

#if !defined(__QNXNTO__)
        Su::single().revert();
#endif
      }
      else {

        // first check the limit; it has no use (Ubuntu 22.04)
        // requesting mlock when the limit is low, as this will lead
        // to alloc failures
        rlimit mlcklim;
        int res = getrlimit(RLIMIT_MEMLOCK, &mlcklim);
        if (res != 0) {
          /* DUECA system.

             Attempt to find out the memlock limit failed. */
          W_SYS("Environment: Cannot detect memlock limit: " << strerror(errno));
          mlcklim.rlim_cur = 0;
        }

        if (mlcklim.rlim_cur != RLIM_INFINITY) {

          /* DUECA system.

             The system indicates that there is a limited amount of
             memory available for locking; avoiding the use of
             mlockall, because this may lead to memory allocation
             failure and crashes. Use ulimit and adapt with a
             configuration file in /etc/security/limits.d if you want
             memory locking and better real-time performance */
          W_SYS("Environment: Not attempting to lock memory, raise limits if needed");
        }
        else {

          if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
            /* DUECA system.

               Attempt to load and lock the memory for the DUECA
               executable failed. This is normal during development,
               when real-time and memory locking priorities are not
               used, but should be avoided during deployment. Check
               the page on 'tuning linux workstations' for
               guidance. This may also happen when not enough memory
               is available.
            */
            W_SYS("Environment: Cannot memlock the DUECA executable: " << strerror(errno));
          }
          else {
            // succeeded in memory locking. Also attempt to set CPU
            // to low latency mode
            cpu_lowlatency.reset(new CPULowLatency(0));
          }
        }
      }
#endif // HAVE_MLOCKALL

      // create the the thread for running dueca
      /* DUECA system.

         Starting a thread for priority 0 and graphics.
       */
      I_SYS("Environment: Creating graphics thread with (e)uids: " << geteuid()
                    << ", " << getuid());

#if defined(USE_POSIX_THREADS)
      pthread_attr_t thread_attrib;
      pthread_attr_init(&thread_attrib);
      pthread_attr_setstacksize(&thread_attrib, 8 * 1024 * 1024);
      int err = pthread_create(&activity0_thread, &thread_attrib,
                               Environment_graphicRun, this);
      if (err == EAGAIN) { // lack of memory????
        /* DUECA system.

           Running into a problem creating the graphics
           thread. Re-trying with other options */
        W_SYS("Environment: Cannot create graphics thread, trying with default stack size");
        pthread_attr_init(&thread_attrib);
        err = pthread_create(&activity0_thread, &thread_attrib,
                             Environment_graphicRun, this);
      }
      if (err != 0) {
        perror("Problem creating graphics thread");
        std::exit(1);
      }
#endif
      // stop here until collected/called again

      /* DUECA system.

         Information on the run status, stopping the scripting thread. */
      I_SYS("Environment: Halting script thread");
      create_control.enterTest();
      while (create_cmd == Wait) {
        create_control.wait();
      }
      /* DUECA system.

         Information message that the script thread resumes. */
      I_SYS("Environment: Script thread continues");

      // at this point, the activity0 thread told us either to exit
      // the program, or to re-read more configuration data
      if (create_cmd == Exit) {

        create_control.leaveTest();

        // join with the graphics thread
#if defined(USE_POSIX_THREADS)
        pthread_join(activity0_thread, NULL);
#endif
        /* DUECA system.

          Script and graphics threads are combined again. */
        I_SYS("Environment: Joined script and graphics threads");
      }
      else {
        create_control.leaveTest();
      }
    }

    else
#endif
    {
      // in single thread mode, the only option is to run the
      // graphics on top of the guile code, meaning that we cannot
      // ever return to guile to read more configuration data
      init_complete = true;
      graphicRun();
    }

    // at this point, control returns to guile for reading more model
    // information
    break;

  case 3:
    // have possibly done additional model reading, wait again for the
    // commands from gui stop here until collected/called again
    create_control.enterTest();
    create_cmd = Wait;
    create_control.signal();
    while (create_cmd == Wait) {
      create_control.wait();
    }
    if (create_cmd == Exit) {
      create_control.leaveTest();
      // join with the graphics thread
#if defined(USE_POSIX_THREADS)
      pthread_join(activity0_thread, NULL);
#endif
      /* DUECA system.

        Script and graphics threads are combined again. */
      I_SYS("Environment: Joined script and graphics threads");
    }
    else {
      create_control.leaveTest();
    }
    break;

  default:
    /* DUECA system.

      Internal error, a run stage that has not been configured is
      requested. */
    E_SYS("Environment: no stage " << stage << " to proceed to");
  }

#if defined(ACTIV_NOCATCH)
  if (create_cmd == Exit) {
    // if catching the exit is not configured, deleting
    // modules is done here
    /* DUECA system.

      At this point the client modules will be deleted.
    */
    I_SYS("Environment: Jettisoning everyone except crew members");
    ObjectManager::single()->destructAllButCrew();
  }
#endif

  /* DUECA system.

     Control returns to the scripting language interpretation.
  */
  I_SYS("Environment: Returning to script");
}

void Environment::propagateTriggers(unsigned prio)
{
  if (activity_manager[prio]->propagateTriggers() &&
      int(prio) > current_highprio) {
    current_highprio = prio;
  }
}

void Environment::wakeActivityManager(unsigned prio)
{
  activity_manager[prio]->wakeThis();
}

int Environment::update()
{
  // one call to the ticker; any new stuff because of elapsed time is
  // inserted here
  Ticker::single()->checkTick();
  activity_manager[0]->scheduleAll();

  while (current_highprio >= 0) {
    current_highprio--;
    activity_manager[int(current_highprio) + 1]->doActivities();
  }

  return in_control;
}

void Environment::quit()
{
  in_control = false;

  // stop the ticker
  Ticker::single()->stopTicking();
  Ticker::single()->pauseTick();

  /* DUECA system.

    Data packing for transport is stopped, breaking communication with
    the other nodes.
  */
  I_SYS("Environment: Stopping packers");
  PackerManager::single()->stopPackers();

  // flag that multithread running is over
  running_multithread = false;

  // do some "manual" ticks again, helps communicators to end their
  // blocking, one second
  for (int ii = 20; ii--;) {
    usleep(100000); // 0.1 second
    Ticker::single()->checkTick();

    // all higher thread activities will go on
  }

  if (run_mode == MultiThread) {
    // join all high-priority threads into this one
    for (int ii = highest_priority; ii > 0; ii--) {
      activity_manager[ii]->stopDoActivities();
    }
  }

  // tell the gui to return control. This the call to
  // gui_handler->passControl() should now return, so control will pop
  // up in graphicRun. The story continues there
  gui_handler->returnControl();
}

/** Set the exit code, mainly used in testing */
void Environment::setExitCode(int ecode)
{
  if (exitcode != 0 && ecode == 0) {
    /* DUECA system.

       A previous call to Environment::setExitCode set the exit code to
       nonzero, indicating an issue. Now the exitcode is reset to zero.
       Check that this is your desired behaviour.
    */
    W_SYS("Environment: The application is resetting the exit code to zero");
  }
  exitcode = ecode;
}

DUECA_NS_END
