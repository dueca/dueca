/* ------------------------------------------------------------------   */
/*      item            : ActivityLister.cxx
        made by         : Rene' van Paassen
        date            : 20001125
        category        : body file
        description     :
        changes         : 20001125 RvP first version
                          20060513 RvP improvements print format
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ActivityLister_cc
#include "ActivityLister.hxx"
#include "ActivityWeaver.hxx"
#include "ActivityBit.hxx"
#include "ObjectManager.hxx"
#include "ActivityDescription.hxx"
#include "debug.h"
#include <cstdio>
#include <debprint.h>

DUECA_NS_START


ActivityLine::ActivityLine(Type t, int x0, int x1) :
  type(t),
  x0(x0),
  x1(x1)
{
  //
}

ActivityStrings::ActivityStrings()
{
  strlist[0] = tick;
  strlist[1] = offset;
  strlist[2] = ts;
  strlist[3] = dt;
  strlist[4] = module;
  strlist[5] = activity;
}

ActivityStrings::ActivityStrings(const ActivityStrings& as)
{
  strncpy(this->tick, as.tick, 13);
  strncpy(this->offset, as.offset, 9);
  strncpy(this->ts, as.ts, 15);
  strncpy(this->dt, as.dt, 8);
  strncpy(this->module, as.module, 65);
  strncpy(this->activity, as.activity, 34);
  strlist[0] = tick;
  strlist[1] = offset;
  strlist[2] = ts;
  strlist[3] = dt;
  strlist[4] = module;
  strlist[5] = activity;
}

char** ActivityStrings::str()
{
  return strlist;
}

std::ostream& operator << (std::ostream& os, const ActivityStrings& as)
{
  os << as.tick << '/' << as.offset << '/' << as.ts << '/'
     << as.dt << '/' << as.module << '/' << as.activity << '/' << endl;
  return os;
}

ActivityLister::ActivityLister(const ActivityWeaver* weaver,
                               int sources, int focus, int match_key) :
  weaver(weaver),
  current_locus(-1),
  locus_changing_locus(-1),
  focus(focus),
  match_key(match_key)
{
  bit_index.resize(sources);
  activity_on.resize(sources, false);
  for (int ii = 0; ii < sources; ii++) {
    bit_index[ii] = weaver->spoolLog(ii, match_key);
  }
  as_nodata.tick[0] = '\000';
  as_nodata.offset[0] = '\000';
  as_nodata.ts[0] = '\000';
  as_nodata.dt[0] = '\000';
  as_nodata.module[0] = '\000';
  ::strcpy(as_nodata.activity, "");
}

ActivityLister::~ActivityLister()
{ }

void ActivityLister::clearSuspends()
{
  for (int ii = bit_index.size(); ii--; ) {
    if (!activity_on[ii] && bit_index[ii] != NULL &&
        bit_index[ii]->goesInactive()) {

      // there is no activity on this thread, any suspends are
      // spurious ones (low prio thread) or from half-logged activity
      DEB1("Clearing suspend in " << ii << ' ' << *bit_index[ii]);
      bit_index[ii] = bit_index[ii]->getNext();
    }
  }
}

int ActivityLister::findRelevant()
{
  // find the first thing happening
  const uint32_t t_final = 0xffffffff;
  uint32_t t_next = t_final, t_new;
  int next = -1;

  for (int ii = bit_index.size(); ii--; ) {
    if (bit_index[ii] != NULL &&
        (t_new = bit_index[ii]->getLongT()) < t_next) {
      t_next = t_new; next = ii;
    }
  }
  return next;
}


ActivityLine ActivityLister::getNextActivity(int prio, float tick_start,
                                             float tick_end, unsigned winwidth)
{
  // if required, spool to starting tick
  while (bit_index[prio] && bit_index[prio]->getNext() &&
         bit_index[prio]->getNext()->getFloatT() < tick_start) {
    bit_index[prio] = bit_index[prio]->getNext();
  }

  while(1) {

    const ActivityBit* start = bit_index[prio];

    // if at end of data, say so
    if (!start) {
      return ActivityLine(ActivityLine::Blank, 0, 0);
    }

    // end of activity
    const ActivityBit* end = bit_index[prio]->getNext();

    // for next check, go to next index
    bit_index[prio] = end;

    // if at end of data, or already past range, return blank
    if (!end || start->getFloatT() > tick_end) {
      return ActivityLine(ActivityLine::Blank, 0, 0);
    }

    // figure out start and end time for this
    int x0 = int((start->getFloatT() - tick_start) /
      (tick_end - tick_start) * winwidth);
    int x1 = end ? int((end->getFloatT() - tick_start) /
                       (tick_end - tick_start) * winwidth) : winwidth;
    switch (start->type) {
    case ActivityBit::Start:
    case ActivityBit::Continue:
      return ActivityLine(ActivityLine::Run, x0, x1);

    case ActivityBit::Block:
      return  ActivityLine(ActivityLine::Block, x0, x1);

    case ActivityBit::Graphics:
      return ActivityLine(ActivityLine::Graphics, x0, x1);
    default:
      continue;
    }
  }
}


ActivityStrings ActivityLister::
getNextActivityDesc(int prio, float tick_start, float tick_end,
                    bool& more)
{
  // if required, spool to starting tick
  while (bit_index[prio] && bit_index[prio]->getFloatT() < tick_start) {
    bit_index[prio] = bit_index[prio]->getNext();
  }

  while(1) {

    const ActivityBit* start = bit_index[prio];

    // if at end of data, say so
    if (!start) {
      more = false;
      return as_nodata;
    }

    // end of activity
    const ActivityBit* end = bit_index[prio]->getNext();

    // for next check, go to next index
    bit_index[prio] = end;

    // if at end of data, or already past range, return blank
    if (!end || end->getFloatT() > tick_end) {
      more = false;
      return as_nodata;
    }

    // prepare return value
    ActivityStrings as;
    snprintf(as.tick, 13, "%12.2f", weaver->getOffset() + start->getFloatT());
    snprintf(as.offset, 9, "%8.3f",
             start->getFloatT(weaver->getScale()*0.001));
    snprintf(as.dt, 8, "%7.3f",
             (end->getFloatT(weaver->getScale()) -
              start->getFloatT(weaver->getScale())) * 0.001);

    more = end->getFloatT() < tick_end;

    switch (start->type) {
    case ActivityBit::Start:
    case ActivityBit::Continue:
    case ActivityBit::Block: {
      snprintf(as.ts, 15, "%9d,%4d", start->time_spec.getValidityStart(),
               start->time_spec.getValiditySpan());
      const NameSet& ns = ObjectManager::single()->getNameSet
        (weaver->getActivityDescription(start->activity).owner);
      snprintf(as.module, 65, "%s/%s/%s", ns.getEntity().c_str(),
               ns.getPart().c_str(), ns.getClass().c_str());
      snprintf(as.activity, 34, "%c:%s", start->getLetter(),
               weaver->getActivityDescription(start->activity).name.c_str());
      return as;
    }
    case ActivityBit::Graphics:
      as.module[0] = '\000';
      as.ts[0] = '\000';
      ::strcpy(as.activity, "graphics update");
      return as;

    default:
      continue;
    }
  }
}



pair<int,int> ActivityLister::reportWithBit(const ActivityBit*& bit)
{
  // clear all unnecessary suspends (e.g. at the start of a log,
  // thread 0 extra suspends, etc
  clearSuspends();

  // find relevant bit
  locus_changing_locus = findRelevant();

  // OK condition 1, no more logged bits
  if (locus_changing_locus < 0) {
    bit = NULL;
    return pair<int,int>(-2,-2);
  }

  // now we now that some bit was found
  bit = locus_changing_bit = bit_index[locus_changing_locus];
  bit_index[locus_changing_locus] = locus_changing_bit->getNext();

  // OK condition 2, something goes active, and locus goes up or stays
  // the same
  if (locus_changing_locus >= current_locus &&
      locus_changing_bit->goesActive()) {
    current_locus = locus_changing_locus;
    activity_on[current_locus] = true;
  }

  // OK condition 3, the current locus goes inactive
  else if (locus_changing_locus == current_locus &&
      locus_changing_bit->goesInactive()) {
    activity_on[current_locus] = false;

    // find the next lower locus, or end at -1
    while (--current_locus >= 0 && !activity_on[current_locus]);
  }
  else {

    DEB1("Unlogical locus change from " << current_locus <<
        " with bit " << *locus_changing_bit << " in " <<
        locus_changing_locus);

    if (locus_changing_bit->goesInactive()) {
      activity_on[locus_changing_locus] = false;
    }
    else {
      activity_on[current_locus] = false;
      current_locus = locus_changing_locus;
    }
  }

  return pair<int,int>(current_locus, locus_changing_locus);
}

vstring ActivityLister::reportVerbal()
{
  // verbal reports don't use focus, and return almost anything
  // find the first bit
  const ActivityBit* bit;
  pair<int,int> locuses;

  // return a report if ok
  locuses = reportWithBit(bit);
  if (locuses.first != -2)
    return bit->reportVerbal(locus_changing_locus, weaver);

  // no report to return
  return vstring("");
}

DUECA_NS_END
