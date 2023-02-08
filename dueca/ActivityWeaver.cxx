/* ------------------------------------------------------------------   */
/*      item            : ActivityWeaver.cxx
        made by         : Rene' van Paassen
        date            : 000908
        category        : body file
        description     :
        changes         : 000908 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ActivityWeaver_cc
#include "ActivityWeaver.hxx"
#include "ActivityBit.hxx"
#include "ActivityLog.hxx"
#include "Exception.hxx"
#include "ActivityDescriptions.hxx"
#include <dassert.h>
//#define I_STS
#include "debug.h"
#include <debprint.h>
DUECA_NS_START
#if 0
ActivityLister::ActivityLister(const ActivityWeaver* weaver,
                               int sources, int focus, int match_key) :
  weaver(weaver),
  current_locus(-1),
  focus(focus),
  match_key(match_key)
{
  index.resize(sources);
  activity_on.resize(sources, false);
  for (int ii = 0; ii < sources; ii++) {
    index[ii] = weaver->spoolLog(ii, match_key);
  }
  new_locus = findLocusOfControl();
}

ActivityLister::~ActivityLister()
{ }

/* Note: this function finds the new locus of control, i.e. the thread
   which will follow functioning. To advance the traversal of the logs,
   the caller is responsible for advancing the bit that indicated a
   new locus of control; i.e. by advancing
   index[max(old_locus, new_locus)]

   A second function, findSlipping must be called to find any
   activities on low-priority threads that should not be there -- and
   to clear them out. These activities either indicate non-reported
   blocking events, or priority inversions.
*/

int ActivityLister::findLocusOfControl()
{
  const uint32_t t_final = 0xffffffff;
  uint32_t t_up = t_final, t_down = t_final,
    t_stay = t_final, t_new;
  int up = -1;

  // find the first event that would move the locus of control up,
  // i.e. a start or a continue
  for (int ii = current_locus + 1; ii < int(index.size()); ii++) {
    if (index[ii] && index[ii]->goesActive() &&
        ((t_new = index[ii]->getLongT()) < t_up)) {
      up = ii; t_up = t_new;
    }
  }

  // find the time corresponding to a downward move in the current locus
  if (current_locus >= 0 &&
      index[current_locus] && index[current_locus]->goesInactive()) {
    t_down = index[current_locus]->getLongT();
  }

  // and find the time corresponding to a new activity in the current
  // locus
  if (current_locus >= 0 &&
      index[current_locus] && index[current_locus]->goesActive()) {
    t_stay = index[current_locus]->getLongT();
  }

  // what if this is the end of the ride?
  if (t_up == t_final && t_stay == t_final && t_down == t_final) {
    return -2;
  }

  // now find out what happens first
  if (t_down <= t_stay && t_down <= t_up) {

    // down is the earliest event, locus moves down
    // indicate that the activity leaves the current locus,
    int new_locus = current_locus;
    while (--new_locus >= 0 && !activity_on[new_locus]);
    activity_on[current_locus] = false;
    assert(new_locus == -1 || activity_on[new_locus] == true);
    return new_locus;
  }
  else if (t_stay <= t_up) {

    // locus stays in this thread
    return current_locus;
  }
  else {

    // locus moves up
    activity_on[up] = true;
    return up;
  }
}

int ActivityLister::findSlipping()
{
  // nothing can be wrong if the locus is in the lowest prio thread
  // or when there is no new locus yet
  if (new_locus <= 0 ) {
    return -1;
  }

  // find anything below the calculated new locus of control that "should
  // not be there", indicating priority inversions and blocks that are
  // not logged.
  uint32_t locus_time = index[new_locus]->getLongT();
  int problem = -1;
  for (int ii = 0; ii < new_locus; ii++) {
    if (index[ii] &&
        index[ii]->getLongT() < locus_time) {
      problem = ii;
      locus_time = index[ii]->getLongT();
    }
  }
  return problem;
}

int ActivityLister::searchFollowing(const ActivityBit*& bit,
                                    bool& problem_found)
{
  // At this point. We either just search for a change/idem locus,
  // and now have to check whether there are "problems", low thread
  // actions we missed, or we enter the second, third .. time after
  // finding a "problem" in a previous invocation
  // check whether there are more problems, spool them off and return
  int old_locus = -3;

#if DEBPRINTLEVEL > 0
  DEB("current locus " << current_locus << " new locus"
      << new_locus << " looking at:");
  for (unsigned int ii = 0; ii < index.size(); ii++) {
    if (index[ii] != NULL) {
      DEB(*index[ii] << ' ' << activity_on[ii]);
    }
    else {
      DEB("none");
    }
  }
#endif

  int problem = findSlipping();
  if (problem >= 0) {

    // found a lost log bit somewhere. return the bit, time and
    // clear it out
    bit = index[problem];
    problem_found = true;
    index[problem] = bit->getNext();
    return problem;
  }

  // at this point. No problems were found, so we return the bit
  // indicated by the new locus of control. Spool off the bit giving
  // the locus change, and search for the new locus
  int bl;
  if (new_locus != -2 && (bl = max(new_locus, current_locus)) >= 0) {

    // the case where there is more data
    bit = index[bl];
    assert(index[bl] != NULL);
    // temp fix/hack: spool off multiple suspends on the #0 activitymanager
    if (bl == 0 && new_locus == -1) {
      while(index[bl] && index[bl]->goesInactive()) {
        index[bl] = index[bl]->getNext();
      }
    }
    else {
      index[bl] = index[bl]->getNext();
    }
    old_locus = current_locus;
    current_locus = new_locus;
    new_locus = findLocusOfControl();
  }
  else {

    // end of the line
    current_locus = new_locus;
    bit = NULL;
  }
  problem_found = false;
  return current_locus;
}

vstring ActivityLister::reportVerbal()
{
  // verbal reports don't use focus, and return almost anything
  // find the first bit
  const ActivityBit* bit;
  bool problem_found;

  // return a report if ok
  if (searchFollowing(bit, problem_found) >= -1)
    return bit->reportVerbal(max(current_locus,new_locus), weaver);

  // no report to return
  return vstring("");
}

int ActivityLister::reportBit(const ActivityBit*& bit, bool& problem_found)
{
  int idx;

  // ignore everything below my focus
  while ((idx = searchFollowing(bit, problem_found)) < focus &&
         idx >= 0);

  return idx;
}
#endif

ActivityWeaver::ActivityWeaver() :
  no_of_logs(0),
  logs_complete(false),
  node(-1)
{
  //
}

ActivityWeaver::~ActivityWeaver()
{
  // remove all my data
  for (int ii = no_of_logs; ii--; ) {
    delete(current_logs[ii]);
  }
}

void ActivityWeaver::includeLog(const ActivityLog* log)
{
  // check for re-sizing of the log vector
  if (log->manager_number >= no_of_logs) {
    no_of_logs = log->manager_number + 1;
    current_logs.resize(no_of_logs, NULL);
  }

  // enter the log, throw away old logs if these are in the way
  delete (current_logs[log->manager_number]);
  current_logs[log->manager_number] = log;

  // check for completenes. Complete is when all logs have the same
  // start time
  if (areTheLogsComplete()) {
    current_key = current_logs[0]->base_tick;
  }
}

void ActivityWeaver::resetLogs()
{
  if (current_logs.size() > 0 && current_logs[0] != NULL) {
    delete (current_logs[0]);
    current_logs[0] = NULL;
  }
}

bool ActivityWeaver::areTheLogsComplete()
{
  // check that at least log 0 has come in
  TimeTickType base_tick;
  if (current_logs.size() > 0 && current_logs[0] != NULL) {
    base_tick = current_logs[0]->base_tick;
    logs_complete = true;
  }
  else {
    // no luck
    logs_complete = false;
    return logs_complete;
  }

  // check that the rest of the logs matches log 0
  for (int ii = no_of_logs; ii--; ) {
    logs_complete = logs_complete && (current_logs[ii] != NULL) &&
      current_logs[ii]->base_tick == base_tick;
  }

  if (I_ACT_INITIAL_ON && !logs_complete) {
    /* DUECA UI.

       Logs for ActivityView have not been fully received. May
       indicate timing issues or happen when nodes are not running
       multi-threaded.
    */
    I_STS("incomplete logs");
    for (int ii = no_of_logs; ii--; ) {
      if (current_logs[ii]) {
        cerr << " M" << ii << ":" << current_logs[ii]->base_tick;
      }
      else {
        cerr << " M" << ii << ":NULL";
      }
    }
    cerr << endl;
  }

  return logs_complete;
}

ActivityLister ActivityWeaver::startInvestigation(int focus) const
{
  if (!logs_complete || focus < -1 || focus >= no_of_logs) {
    throw(WeaverKeyInvalid(GlobalId(0,0), GlobalId(0,0)));
  }

  return ActivityLister(this, no_of_logs, focus, current_key);
}

const ActivityDescription& ActivityWeaver::getActivityDescription(int i) const
{
  ActivityContext cntxt(node, 0, i);
  return ActivityDescriptions::single()[cntxt];
}

const ActivityBit* ActivityWeaver::spoolLog(int i, uint32_t key) const
{
  // check the key
  if (key != current_key || i < 0 || i >= no_of_logs)
    throw(WeaverKeyInvalid(GlobalId(0,0), GlobalId(0,0)));

  // skip the traditional LogStart
  return current_logs[i]->bit_list->getNext();
}

double ActivityWeaver::getScale() const
{
  if (current_logs[0] != NULL) {
    return current_logs[0]->fraction_mult;
  }
  return 1.0;
}

TimeTickType ActivityWeaver::getOffset() const
{
  if (logs_complete) {
    return current_logs[0]->base_tick;
  }
  return 0;
}

bool ActivityWeaver::checkValidity(uint32_t key) const
{
  return key == current_key;
}
DUECA_NS_END

