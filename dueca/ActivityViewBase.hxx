/* ------------------------------------------------------------------   */
/*      item            : ActivityViewBase.hh
        made by         : Rene' van Paassen
        date            : 000830
        category        : header file
        description     :
        changes         : 000830 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        api             : DUECA_API
*/

#ifndef ActivityViewBase_hh
#define ActivityViewBase_hh

#include "Module.hxx"
#include "ChannelReadToken.hxx"
#include "ChannelWriteToken.hxx"
#include "ActivityLogRequest.hxx"
#include "ActivityLog.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include "ActivityWeaver.hxx"
#include "AperiodicAlarm.hxx"
#include <list>
#include <fstream>
using namespace std;

#include <dueca_ns.h>
DUECA_NS_START

struct ParameterTable;

/** This is a module that can generate an overview of the activity
    (timelines) in a set of connected DUECA nodes. This module can be
    created in the model script. See the description of TimingView for
    more information about how to set up the "dueca" part of the model
    script for your application.
*/
class ActivityViewBase: public Module
{
public:
  /** Name of the module class. */
  static const char* const       classname;

  /** Table with adjustable parameters. */
  static const ParameterTable*   getParameterTable();

protected:
  /** singleton pointer, there can only be one view in an executable. */
  static ActivityViewBase*           singleton;

  /** Function on token completion */
  Callback<ActivityViewBase>         token_valid;

  /** Function on token completion. */
  void tokenValid(const TimeSpec& ts);

  /** Flag to remember token completion. */
  bool token_action;

  /** Flag to indicate start possible */
  bool can_start;

  /** Access token for a channel over which log requests are sent. */
  ChannelWriteToken            send_request;

  /** Access token for a channel with the incoming logs. */
  ChannelReadToken             log_channel0;

  /** Access token for a channel with the incoming logs. */
  ChannelReadToken             log_channelO;

  /** A vector with the current logs. ActivityWeaver objects assemble
      and interpret the raw logs, and can be queried to obtain a nice
      time-line. Each node needs one weaver. */
  vector<ActivityWeaver>                      current_logs;

  /** Callback object 1, for processing log reports from local node. */
  Callback<ActivityViewBase>                      cb1;

  /** Callback object 2, for processing log reports from elsewhere. */
  Callback<ActivityViewBase>                      cb2;

  /** Callback object 3, for sending out a sweep-up request. */
  Callback<ActivityViewBase>                      cb3;

  /** The activity that handles the incoming logs. */
  ActivityCallback                            process_log_reports0;

  /** The activity that handles the incoming logs. */
  ActivityCallback                            process_log_reportsO;

  /** Sweep up logs. */
  ActivityCallback                            send_sweep_request;

  /** Time for sweeping. */
  AperiodicAlarm                              sweep_alarm;

  /** Flag to remember sweeping. */
  bool                                        sweep_done;

  /** Number of clock ticks per second, needed to calculate look-ahead
      times. */
  int                                         ticks_per_sec;

  /** The time to look ahead for requests (should be time needed for
      all requests to arrive) */
  int                                         lookahead;

  /** The time at which the previous request for activity information
      ends. */
  TimeTickType                                prev_request_end;

  /** Tick at which request has been sent. */
  TimeTickType                                first_base_tick;

  /** Tick at which current request has been / will be sent. */
  TimeTickType                                current_base_tick;

  /** Span of recording period, in seconds. */
  double                                      dspan;

  /** Span of the view, in seconds. */
  double                                      vspan;

  /** File for dumping activity logs. */
  ofstream                                    activity_log;

  /** Highlighted and listed area. */
  struct HighLight {

    /** Node number that is highlighted. */
    int                                       node;

    /** Activity level. */
    int                                       level;

    /** Start, integer coordinate on screen. */
    int                                       start;

    /** End, integer coordinate on screen. Also serves as the flag for
        having or not having a highlighted area. */
    int                                       end;

    /** Constructor. */
    HighLight();
  };

  /** Details of the highlighted area. */
  HighLight                                   hl;

  /** Details of a new highlighted area. */
  HighLight                                   hlnew;

  /** Request a list with descriptions from managers. */
  void requestDescriptionList(const TimeSpec& time);

public:
  /** Obtain a pointer to the singleton. */
  inline static ActivityViewBase* single() {return singleton;};

  /** Constructor. */
  ActivityViewBase(Entity* e, const char* part, const PrioritySpec& ps);

  /** Destructor. */
  ~ActivityViewBase();

  /** Completion, creates the window. */
  virtual bool complete() = 0;

  /** Start the viewer, not used, since it will always work. */
  void startModule(const TimeSpec& time);

  /** Stop the viewer, not used. */
  void stopModule(const TimeSpec& time);

  /** Re-draw the display of activities. */
  void readLogOwn(const TimeSpec& time);

  /** Re-draw the display of activities. */
  void readLogOthers(const TimeSpec& time);

  /** Re-draw the display of activities. */
  void updateLines(const ActivityLog* log);

  /** Read a list of activity descriptions. */
  void getActivityList(const TimeSpec& time);

  /** Send a request to sweep up logs. */
  void sendSweepRequest(const TimeSpec& time);

  /** Call for a measurement of the activities. */
  void triggerUpdate();

  /** Pre-define a position and size for the window */
  virtual bool setPositionAndSize(const std::vector<int>& ps);
  
  /** Change the look-ahead time. */
  bool setLookAhead(const double& ahead);

  /** Change the span of the logs. */
  inline void setSpan(double span) {this->dspan = span;}

  /** Returns true if the module is prepared for starting. */
  bool isPrepared();

  /** Return the name of an activity (if known) */
  const string32& getActivityName(int node, int acno);

  /** Process a single log */
  void processLog(const ActivityLog* log);

  /** draw the log in a view */
  virtual void updateLines(unsigned node) = 0;
};

DUECA_NS_END
#endif
