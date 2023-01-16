/* ------------------------------------------------------------------   */
/*      item            : ActivityViewBase.cxx
        made by         : Rene' van Paassen
        date            : 000830
        category        : body file
        description     :
        changes         : 000830 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ActivityViewBase_cc

#include <dueca-conf.h>

#include "ActivityViewBase.hxx"
#include "PrioritySpec.hxx"
#include "NodeManager.hxx"
#include "NameSet.hxx"
#include "Ticker.hxx"
#include "SimTime.hxx"
#include "ParameterTable.hxx"
#include "ActivityLog.hxx"
#include <ActivityDescriptions.hxx>
#include <cmath>
#include <time.h>
#include <DataReader.hxx>
#include <DataWriter.hxx>
#include <sstream>

#define W_STS
#define E_STS
#include <debug.h>

#define DO_INSTANTIATE
#include "MemberCall.hxx"
#include "Callback.hxx"

// localized development debugging
#define DEBPRINTLEVEL -1
#include <debprint.h>

/** Pixel distance between activity lines on the display. */
#define LINESPACE 8
/** If defined, also give an alphanumeric output on the log. */
#undef DO_PRINT

DUECA_NS_START

const char* const ActivityViewBase::classname = "activity-view";
ActivityViewBase* ActivityViewBase::singleton = NULL;

ActivityViewBase::HighLight::HighLight() :
  end(0)
{
  // By setting end to 0, highlight is off
}


ActivityViewBase::ActivityViewBase(Entity* e, const char* part,
                     const PrioritySpec& ps) :
  Module(e, classname, part),

  token_valid(this, &ActivityViewBase::tokenValid),
  token_action(true),
  can_start(true),

  send_request(getId(), NameSet("dueca", ActivityLogRequest::classname, ""),
               ActivityLogRequest::classname, "exclusive", Channel::Events,
               Channel::OnlyOneEntry, Channel::OnlyFullPacking,
               Channel::Regular, &token_valid),
  log_channel0(getId(), NameSet("dueca", ActivityLog::classname, "zero"),
               ActivityLog::classname, entry_any, Channel::Events,
               Channel::OneOrMoreEntries, Channel::ReadAllData, 0.0,
               &token_valid),
  log_channelO(getId(), NameSet("dueca", ActivityLog::classname, "others"),
               ActivityLog::classname, entry_any, Channel::Events,
               Channel::OneOrMoreEntries, Channel::ReadAllData, 0.0,
               &token_valid),

  // callback function object
  cb1(this, &ActivityViewBase::readLogOwn),
  cb2(this, &ActivityViewBase::readLogOthers),
  cb3(this, &ActivityViewBase::sendSweepRequest),

  // the current activities
  process_log_reports0(getId(), "process activitylog from 0", &cb1, ps),
  process_log_reportsO(getId(), "process activitylog from others", &cb2, ps),
  send_sweep_request(getId(), "sweep activitylogs", &cb3, ps),

  // waker for the sweep request
  sweep_alarm(),
  sweep_done(true),

  ticks_per_sec(int(rint(1.0/Ticker::single()->getTimeGranule()))),
  lookahead(int(max(1, int(0.1*ticks_per_sec + 0.5)))),
  prev_request_end(0),
  first_base_tick(0),
  current_base_tick(0),
  dspan(0.1),
  vspan(0.1),
  activity_log("dueca.activitylog"),
  hl(),
  hlnew()
{
  // check and warn when the priority is not 0, 0 priority is needed
  // because this must run in the same thread as the graphics update
  if (ps.getPriority() != 0) {
    /* DUECA UI.

       Wrong priority supplied for ActivityView, should be 0. Fix your
       configuration file. */
    W_CNF("ActivityView should have 0 priority!");
    can_start = false;
  }

  // check that only one instance is made.
  if (singleton == NULL) {
    singleton = this;
  }
  else {
    /* DUECA UI.

       Only one ActivityView module should be created. Fix your
       configuration file. */
    W_CNF("Second creation of ActivityView is useless");
    can_start = false;
  }

  // reserve room for the logs
  current_logs.resize(NodeManager::single()->getNoOfNodes(),
                      ActivityWeaver());
  for (int ii = current_logs.size(); ii--; ) {
    current_logs[ii].setNode(ii);
  }
  DEB("ActivityView created");
}

void ActivityViewBase::tokenValid(const TimeSpec& ts)
{
  DEB("ActivityView token valid callback");

  bool res = send_request.isValid() && log_channel0.isValid();
  if (NodeManager::single()->getNoOfNodes() > 1) {
    res = res && log_channelO.isValid();
  }

  if (token_action && res) {

    DEB("ActivityView all tokens valid 1st time");

    // connect the triggers for incoming reports and descriptions
    process_log_reports0.setTrigger(log_channel0);
    process_log_reports0.switchOn(TimeSpec(0,0));
    //  process_incoming_list.setTrigger(description_channel);
    //  process_incoming_list.switchOn(TimeSpec(0,0));
    send_sweep_request.setTrigger(sweep_alarm);
    send_sweep_request.switchOn(TimeSpec(0,0));

    if (NodeManager::single()->getNoOfNodes() > 1) {
      process_log_reportsO.setTrigger(log_channelO);
      process_log_reportsO.switchOn(TimeSpec(0,0));
    }
#if 0
    // send a short probe to initialise the ActivityWeavers.
    first_base_tick = 1;
    {
      DataWriter<ActivityLogRequest> r(send_request, first_base_tick);
      r.data().span = 2;
      r.data().start = first_base_tick;
    }
    prev_request_end = first_base_tick +
      Ticker::single()->getCompatibleIncrement();

    // start up the sweeper
    sweep_alarm.requestAlarm();
    sweep_done = false;
#endif

    // initialized
    token_action = false;
  }
}

bool ActivityViewBase::isPrepared()
{
  return true;
}

ActivityViewBase::~ActivityViewBase()
{
  activity_log.close();
}

bool ActivityViewBase::setLookAhead(const double& ahead)
{
  if (ahead < 0.0) return false;

  lookahead = max(1, int(ahead * ticks_per_sec + 0.5));
  return true;
}


void ActivityViewBase::startModule(const TimeSpec& time)
{
  // does nothing
}

void ActivityViewBase::stopModule(const TimeSpec& time)
{
  // does nothing again
}

void ActivityViewBase::sendSweepRequest(const TimeSpec& time)
{
  DEB("ActivityView sending sweep request " << time);
  DataWriter<ActivityLogRequest> r(send_request, time);
  r.data().span = 0;
  r.data().start = time.getValidityStart();
  sweep_done = true;
}

template<class T>
class VirtualJoinWithStealing: public VirtualJoin<T>
{
public:
  /** Constructor. Essential to initialize data pointer to NULL */
  VirtualJoinWithStealing(DataReaderBase& r): VirtualJoin<T>(r) {}

  /** Release a previous access */
  inline const void release(ChannelReadToken& token)
  {
    if (VirtualJoin<T>::data_ptr) {
      DataReaderBaseAccess::releaseAccessKeepData
        (token, VirtualJoin<T>::data_ptr);
    }
  }
};

void ActivityViewBase::readLogOwn(const TimeSpec& time)
{
  const ActivityLog* log;
  {
    DataReader<ActivityLog,VirtualJoinWithStealing>
      r(log_channel0, TimeSpec::end_of_time);
    log = &r.data();
    DEB("ActivityView local log read mgr=" << int(r.data().manager_number) <<
        " basetick=" << r.data().base_tick << " " << r.timeSpec());
  }
  processLog(log);

  DEB("other channel " << log_channelO.getNumVisibleSets());

}


void ActivityViewBase::readLogOthers(const TimeSpec& time)
{
  const ActivityLog* log;
  {
    DataReader<ActivityLog,VirtualJoinWithStealing>
      r(log_channelO, TimeSpec::end_of_time);
    log = &r.data();
    DEB("ActivityView rem=" << int(r.data().node_id) << " log read mgr="
        << int(r.data().manager_number) <<
        "basetick=" << r.data().base_tick << " " << r.timeSpec());
  }
  processLog(log);
}


void ActivityViewBase::processLog(const ActivityLog* log)
{
  DEB("received log " << *log);

  // store it in the right place
  current_logs[log->node_id].includeLog(log);

  // if this is not the first report (which is used to gather the
  // number of activitymanagers per node), print out the new lines
  if (log->base_tick != first_base_tick &&
      current_logs[log->node_id].areTheLogsComplete()) {

    ActivityLister l = current_logs[log->node_id].startInvestigation(0);

#ifdef DO_PRINT
    // first try printing the stuff
    cout << "Log from node " << int(log->node_id) << endl;
    vstring to_print = l.reportVerbal();
    while (to_print != vstring("")) {
      cout << to_print << endl;
      to_print = l.reportVerbal();
    }
#endif

    // print on log file
    time_t now = ::time(NULL);
    activity_log << "Activity log from node " << int(log->node_id)
                 << " received at " << ctime(&now) << endl;
    vstring to_print = l.reportVerbal();
    while (to_print != vstring("")) {
      activity_log << to_print << endl;
      to_print = l.reportVerbal();
    }

    updateLines(log->node_id);
  }
}


const ParameterTable* ActivityViewBase::getParameterTable()
{
  static ParameterTable table[] = {
    {"set-lookahead",
     new MemberCall<ActivityViewBase,double>(&ActivityViewBase::setLookAhead),
     "advance time needed for request sending" },
    {NULL, NULL,
     "Visualises activity data for the different DUECA nodes. When included,\n"
     "this module adds a menu entry to a suitable dueca-view\n"} };
  return table;
}

const string32& ActivityViewBase::getActivityName(int node, int acno)
{
  static const string32 illegal("illegal request");
  if (node < 0 || node >= int(current_logs.size()) || acno < 0) {
    return illegal;
  }
  else {
    return (current_logs[node].getActivityDescription(acno)).name;
  }
}

DUECA_NS_END
