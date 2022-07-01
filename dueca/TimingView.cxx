/* ------------------------------------------------------------------   */
/*      item            : TimingView.cxx
        made by         : Rene' van Paassen
        date            : 020225
        category        : body file
        description     :
        changes         : 020225 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TimingView_cxx

// this module is conditional on having and using libglade

#include "TimingView.hxx"
#include "ParameterTable.hxx"
#include "ActivityViewBase.hxx"
#include "NodeManager.hxx"
#include "DuecaPath.hxx"
#include <iomanip>
#include <time.h>
#include <sstream>

//#define I_SYS
#define W_CNF
#define E_CNF
#include "debug.h"

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <debprint.h>

// only useful if interfaces present

DUECA_NS_START

const char* const TimingView::classname = "timing-view";

const struct ParameterTable* TimingView::getParameterTable()
{
  static ParameterTable table[] = {
    {NULL, NULL,
     "Visualises timing data for the synchronisation between different\n"
     "DUECA nodes, and timing results for activities, i.e. the TimingCheck\n"
     "results. When created, the TimingView inserts an item in the DUECA\n"
     "main interface"} };
  return table;
}

TimingView::TimingView(Entity* e, const char* part,
                       const PrioritySpec& ps) :
  Module(e, classname, part),
  token_valid(this, &TimingView::tokenValid),
  token_action(true),
  result_channel(getId(), NameSet(getEntity(), "TimingResults", ""),
                 "TimingResults", entry_any, Channel::Events,
                 Channel::ZeroOrMoreEntries, Channel::ReadReservation, 0.0,
                 &token_valid),
  request_report(getId(), NameSet(getEntity(), "SyncReportRequest", ""),
                 "SyncReportRequest", std::string(), Channel::Events,
                 Channel::OnlyOneEntry, Channel::OnlyFullPacking,
                 Regular, &token_valid),
  result_report(getId(), NameSet(getEntity(), "SyncReport", ""),
                "SyncReport", entry_any, Channel::Events,
                Channel::OneOrMoreEntries, Channel::ReadAllData, 0.0,
                &token_valid),
  cb1(this, &TimingView::readReport),
  cb2(this, &TimingView::readSync),
  read_report(getId(), "read timing report", &cb1, ps),
  read_sync(getId(), "read sync report", &cb2, ps),
  can_start(true),
  num_rows(0),
  no_nodes(NodeManager::single()->getNoOfNodes()),
  timing_log("dueca.timinglog")
{
  // check and warn when the priority is not 0, 0 priority is needed
  // because this must run in the same thread as the graphics update
  if (ps.getPriority() != 0) {
    /* DUECA UI.

       Configuration error, specify priority 0 for TimingView */
    W_CNF(getId() << " TimingView should have 0 priority!");
    can_start = false;
  }

  // check that the ActivityView has been made. The ActivityView
  // provides names for the activities
  if (ActivityViewBase::single() == NULL) {
    /* DUECA UI. 

      Configuration error, when using the TimingView, the ActivityView
      must also be used, otherwise names for the activities cannot be
      decoded. */
    W_CNF(getId() << " TimingView needs ActivityView!");
    can_start = false;
  }

  // write a header to the text-based timing log
  time_t now = time(NULL);
  timing_log << "Dueca timing log, run start at " << ctime(&now) << endl
             << setw(42) << "activity"
             << setw(10) << "strt tim"
             << setw(8)  << "min st"
             << setw(8)  << "avg st"
             << setw(8)  << "max st"
             << setw(8)  << "min cp"
             << setw(8)  << "avg cp"
             << setw(8)  << "max cp"
             << setw(5)  << "warn"
             << setw(5)  << "crit"
             << setw(5)  << "user" << endl;
}

void TimingView::tokenValid(const TimeSpec& ts)
{
  if (token_action && result_channel.isValid() &&
      request_report.isValid() && result_report.isValid()) {

    // set triggers on the incoming data
    read_report.setTrigger(result_channel);
    read_sync.setTrigger(result_report);

    // switch on the activities now. Use a TimeSpec(0,0)
    read_report.switchOn(TimeSpec(0,0));
    read_sync.switchOn(TimeSpec(0,0));

    token_action = false;
  }
}


TimingView::~TimingView()
{
  // remove the gtk windows?
}


void TimingView::startModule(const TimeSpec& ts)
{
  //
}


void TimingView::stopModule(const TimeSpec& ts)
{
  //
}


bool TimingView::isPrepared()
{
  bool res = true;

  CHECK_TOKEN(result_channel);
  CHECK_CONDITION(can_start);
  return res;
}


void TimingView::readReport(const TimeSpec& ts)
{
  // do not catch, will be stopped when anything goes wrong
  // EventReader<TimingResults> r(result_channel, ts);
  DataReader<TimingResults,VirtualJoin> r(result_channel);

  /* DUECA UI.

     Informational message. Timing report received.
  */
  I_SYS(getId() << " timing " << r.origin() << ' ' << r.data());

  // append log to the list, updates the maker_and_act string
  //std::string maker_and_act;
  std::stringstream maker_and_act;
  maker_and_act << r.data().owner_id << setw(32) <<
    ActivityViewBase::single()->getActivityName(r.origin().getLocationId(),
                                                r.data().activity_no);

  appendReport(maker_and_act.str(), ts.getValidityStart(), r.data());

  // print out to the timing log
  timing_log << setw(42) << maker_and_act.str()
             << setw(10) << ts.getValidityStart()
             << setw(8)  << r.data().min_start
             << setw(8)  << r.data().avg_start
             << setw(8)  << r.data().max_start
             << setw(8)  << r.data().min_complete
             << setw(8)  << r.data().avg_complete
             << setw(8)  << r.data().max_complete
             << setw(5)  << r.data().n_warning
             << setw(5)  << r.data().n_critical
             << setw(5)  << r.data().n_user << endl;
}

void TimingView::readSync(const TimeSpec& ts)
{

  // do not catch, will be stopped when anything goes wrong
  DataReader<SyncReport, VirtualJoin> r(result_report);

  DEB(getId() << r.data());

  // check this is in range
  if (r.origin().getLocationId() < 0 ||
      r.origin().getLocationId() > no_nodes) {
    /* DUECA UI.

       A report on synchronization was received from a non-existent
       node. Either a mis-configuration, or an error in decoding
       communicated data. */
    W_SYS(getId() << " sync report from non-existent node " <<
          r.origin().getLocationId());
    return;
  }

  int node = r.origin().getLocationId();
  updateSync(node, r.data());
}

DUECA_NS_END



