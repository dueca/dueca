/* ------------------------------------------------------------------   */
/*      item            : Trigger.cxx
        made by         : Rene' van Paassen
        date            : 990517
        category        : body file
        description     :
        changes         : 990517 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Trigger_cc
#include "Trigger.hxx"
#include "TimeSpec.hxx"
#include "GenericPacker.hxx"
#include "ActivityManager.hxx"
#include "ArenaPool.hxx"
#include "Arena.hxx"
#include "debug.h"
#include <algorithm>
#include <iostream>

#define DEBPRINTLEVEL -1
#define DEBA(A)
#define DEBL(A)

#define PRELOAD_REF 1

#include "debprint.h"
#ifndef DEBL
#ifdef BUILD_DEBPRINT
#define DEBL(A) std::cout << A << std::endl;
#else
#define DEBL(A)
#endif
#endif
#ifndef DEBA
#ifdef BUILD_DEBPRINT
#define DEBA(A) std::cout << A << std::endl;
#else
#define DEBA(A)
#endif
#endif

DUECA_NS_START

CODE_REFCOUNT(TriggerTarget);

TriggerPuller::TargetData::
TargetData(const boost::intrusive_ptr<TriggerTarget>& t,
           unsigned id) :
  target(t),
  id(id),
  manager(NULL)
{
  //
}

TriggerPuller::TargetData::TargetData(const TriggerPuller::TargetData& o) :
      target(o.target),
      id(o.id),
      manager(o.manager)
{ }

TriggerPuller::TargetData&
TriggerPuller::TargetData::operator=(const TriggerPuller::TargetData& o)
{
  if (this == &o) return *this;
  this->target = o.target;
  this->id = o.id;
  this->manager = o.manager;
  return *this;
}

TriggerPuller::TriggerPuller(const std::string& name) :
  targets(),
  activitylevels(),
  name(name)
{
  //
}

TriggerPuller::~TriggerPuller()
{
  for (targetlist_type::iterator ii = targets.begin();
       ii != targets.end(); ii++) {
    if (ii->target) {
      ii->target->forgetTrigger(this);
    }
  }
}

const std::string& TriggerPuller::getTriggerName() const
{
  return name;
}


bool TriggerPuller::setManager(ActivityManager* mgr,
                               ActivityManager* oldmgr,
                               const boost::intrusive_ptr<TriggerTarget>& t)
{
  // also update the activitylevels bitset
  activitylevels = 0;
  int setcorrect = 0;

  for (targetlist_type::iterator ii = targets.begin();
       ii != targets.end(); ii++) {
    if (ii->target && ii->target == t) {
      setcorrect = (ii->manager == oldmgr) ? 1 : 2;
      ii->manager = mgr;
    }
    if (ii->manager) {
      activitylevels.set(ii->manager->getPrio());
    }
  }
  assert(setcorrect != 0);
  return (setcorrect == 1);
}

void TriggerPuller::addTarget(const boost::intrusive_ptr<TriggerTarget>& t,
                              unsigned id)
{
#if 0
  // recycle targets? this still interacts with setcorrect
  for (targetlist_type::iterator tt = targets.begin();
       tt != targets.end(); tt++) {
    if (tt->manager == NULL) {
      *tt = TargetData(t, id);
      return;
    }
  }
#endif
  targets.push_back(TargetData(t, id));
}

void TriggerPuller::removeTarget(const TriggerTarget* t)
{
  for (targetlist_type::iterator tt = targets.begin();
       tt != targets.end(); tt++) {
    if (t == tt->target.get()) {
      tt->manager = NULL;
      tt->target.reset();
      return;
    }
  }
  assert(0);
}

void TriggerPuller::setTriggerName()
{
  // default implementation no change
}

void TriggerPuller::pull(const DataTimeSpec& ts)
{
  // for each of the targets, insert a triggering atom with the associated
  // activitymanager
  for (targetlist_type::iterator ii = targets.begin();
       ii != targets.end(); ii++) {
    if (ii->manager != NULL) {
      ii->manager->addAtom(ii->target.get(), ii->id, ts);
    }
  }

  // and tell my "own" activitymanager which levels were triggered
  ActivityManager::triggeredLevels(activitylevels);
}

// --------------------------------------------------------------------

TriggerTarget::PullerData::TimingList::TimingList(const DataTimeSpec& ts,
                                                  TimingListPtr prev) :
  ts(ts),
  endtime(ts.getValidityEnd()),
  next(NULL)
{
  DEBL("new timinglist " << reinterpret_cast<void*>(this) << " " << ts);
  if (prev) {
    prev->next = this;
  }
}

TriggerTarget::PullerData::TimingList::~TimingList()
{
  // delete next;
}

void* TriggerTarget::PullerData::TimingList::operator new(size_t size)
{
  static Arena* my_arena = arena_pool.findArena
    (sizeof(TimingList));
  return my_arena->alloc(size);
}

void TriggerTarget::PullerData::TimingList::operator delete(void* p)
{
  static Arena* my_arena = arena_pool.findArena
    (sizeof(TimingList));
  my_arena->free(p);
}

TriggerTarget::PullerData::PullerData(TriggerPuller* p) :
  puller(p),
  time_head(new TimingList(0U)),
  time_tail(time_head)
{
  //
}

TriggerTarget::PullerData::PullerData(const PullerData& o) :
  puller(o.puller),
  time_head(NULL),
  time_tail(NULL)
{
  TimingListPtr p = o.time_head;
  time_tail = time_head = new TimingList(p->ts);
  p = p->next;
  while (p != NULL) {
    time_tail = new TimingList(p->ts, time_tail);
    p = p->next;
  }
}

TriggerTarget::PullerData::~PullerData()
{
  TimingListPtr to_delete = time_head;
  while(to_delete) {
    time_head = time_head->next;
    delete to_delete;
    to_delete = time_head;
  }
}



TriggerTarget::TriggerTarget() :
  INIT_REFCOUNT_COMMA
  manager(NULL)
{
#if PRELOAD_REF
  intrusive_ptr_add_ref(this);
#endif

  DEB("TriggerTarget created " << reinterpret_cast<void*>(this) <<
      " count " << use_count());
}

TriggerTarget::~TriggerTarget()
{
  DEB("TriggerTarget deletion " << reinterpret_cast<void*>(this) <<
      " count " << use_count());
  for (pullers_type::iterator pp = pullers.begin();
       pp != pullers.end(); pp++) {
    if (pp->puller) {
      pp->puller->removeTarget(this);
    }
  }
}

void TriggerTarget::setTrigger(TriggerPuller& p)
{
  if (pullers.size()) {
    /* DUECA activity.

       You are trying to set a new trigger on an already triggered
       target (activity in most cases). Correct your code, if you need
       to set a new trigger, clear the old one first with
       dueca::TriggerTarget::clearTriggers(). */
    W_ACT("This target is already triggered, ignoring new trigger");
    return;
  }
  pullers.push_back(PullerData(&p));
  p.addTarget(boost::intrusive_ptr<TriggerTarget>(this), 0U);
  DEB("setting puller " << p.name << " for target "
      << reinterpret_cast<void*>(this) << " " << this->getTargetName());
  if (!p.setManager(manager, NULL, boost::intrusive_ptr<TriggerTarget>(this))) {
    /* DUECA activity.

       You are trying to re-use an already connected combining trigger
       (and or or) for a new target. This is not possible, create a
       new trigger if needed.
     */
    E_ACT("setTrigger; this (combination) trigger cannot be re-used");
  }
}

void TriggerTarget::forgetTrigger(const TriggerPuller* p)
{
  for (pullers_type::iterator pp = pullers.begin();
       pp != pullers.end(); pp++) {
    if (pp->puller == p) {
      DEB("forgeting puller " << p->name << " for target "
          << reinterpret_cast<void*>(this));
      pp->puller = NULL;
    }
  }
}

void TriggerTarget::clearTriggers()
{
  for (pullers_type::iterator pp = pullers.begin();
       pp != pullers.end(); pp++) {
    pp->puller->removeTarget(this);
  }
  pullers.clear();
}

bool TargetAndPuller::removeTerm(TriggerPuller& p)
{
  for (pullers_type::iterator pp = pullers.begin();
       pp != pullers.end(); pp++) {
    if (pp->puller == &p) {
      pp->puller->removeTarget(this);
      pullers.erase(pp);
      return true;
    }
  }
  return false;
}

void TriggerTarget::setTrigger(boost::intrusive_ptr<TargetAndPuller> p)
{
  if (pullers.size()) {
    /* DUECA activity.

       You are trying to set a new trigger on an already triggered
       target (activity in most cases). Correct your code, if you need
       to set a new trigger, clear the old one first with
       dueca::TriggerTarget::clearTriggers(). */
    W_ACT("This target is already triggered, ignoring new trigger");
    return;
  }
  DEB("setting puller " << p->name << " for target "
      << reinterpret_cast<void*>(this) << " " << this->getTargetName());
  pullers.push_back(PullerData(p.get()));
  p->addTarget(boost::intrusive_ptr<TriggerTarget>(this), 0U);
  if (!p->setManager(manager, NULL,
                     boost::intrusive_ptr<TriggerTarget>(this))) {
    /* DUECA activity.

       You are trying to re-use an already connected combining trigger
       (and or or) for a new target. This is not possible, create a
       new trigger if needed.
     */
    E_ACT("setTrigger; this (combination) trigger cannot be re-used");
  }
}

bool TriggerTarget::passManager(ActivityManager* mgr, ActivityManager* oldmgr)
{
  manager = mgr;
  bool res = true;
  for (pullers_type::iterator ii = pullers.begin();
       ii != pullers.end(); ii++) {
    res = res &&
      ii->puller->setManager(mgr, oldmgr,
                             boost::intrusive_ptr<TriggerTarget>(this));
  }
  return res;
}

TargetAndPuller::TargetAndPuller(const std::string& name) :
  TriggerTarget(),
  TriggerPuller(name)
{
  //
}

TargetAndPuller::~TargetAndPuller()
{
  //
}

bool TargetAndPuller::setManager(ActivityManager* mgr,
                                 ActivityManager* oldmgr,
                                 const boost::intrusive_ptr<TriggerTarget>& t)
{
  return TriggerPuller::setManager(mgr, oldmgr, t) &&
    TriggerTarget::passManager(mgr, oldmgr);
}


ConditionOr::ConditionOr() :
  TargetAndPuller("Or()"),
  previous_tick(0)
{
  //
}

ConditionOr::~ConditionOr()
{
  //
}

const std::string& ConditionOr::getTargetName() const
{ return getTriggerName(); }

void ConditionOr::setTriggerName()
{
  std::string join = "(";
  this->name = "Or";
  for (pullers_type::iterator ii = pullers.begin();
       ii != pullers.end(); ii++) {
    this->name = this->name + join + ii->puller->getTriggerName();
    join = ", ";
  }
  this->name = this->name + ")";
}

void ConditionOr::trigger(const DataTimeSpec& ts, unsigned idx)
{
  if (previous_tick < ts.getValidityEnd()) {
    TriggerPuller::pull
      (DataTimeSpec(max(ts.getValidityStart(), previous_tick),
                    ts.getValidityEnd()));
    previous_tick = ts.getValidityEnd();
  }
}

ConditionAnd::ConditionAnd() :
  TargetAndPuller("And()"),
  previous_tick(0)
{
  DEB("ConditionAnd created " << reinterpret_cast<void*>(this) <<
      " count " << use_count());
}

ConditionAnd::~ConditionAnd()
{
  //
}

const std::string& ConditionAnd::getTargetName() const
{ return getTriggerName(); }

void ConditionAnd::setTriggerName()
{
  std::string join = "(";
  this->name = "And";
  for (pullers_type::iterator ii = pullers.begin();
       ii != pullers.end(); ii++) {
    name = this->name + join + ii->puller->getTriggerName();
    join = ", ";
  }
  this->name = this->name + ")";
}

void TargetAndPuller::addTerm(TriggerPuller& p)
{
  pullers.push_back(&p);
  p.addTarget(boost::intrusive_ptr<TriggerTarget>(this), pullers.size() - 1);
  if (!p.setManager(manager, NULL,
                    boost::intrusive_ptr<TriggerTarget>(this))) {
    /* DUECA activity.

       You are trying to re-use an already connected combining trigger
       (and or or) for a new target. This is not possible, create a
       new trigger if needed.
     */
    E_ACT("setTrigger; this (combination) trigger cannot be re-used");
  }
  setTriggerName();
}

void TargetAndPuller::addTerm(const boost::intrusive_ptr<TargetAndPuller>& p)
{
  pullers.push_back(p.get());
  p->addTarget(boost::intrusive_ptr<TriggerTarget>(this), pullers.size() - 1);
  if (!p->setManager(manager, NULL,
                     boost::intrusive_ptr<TriggerTarget>(this))) {
    /* DUECA activity.

       You are trying to re-use an already connected combining trigger
       (and or or) for a new target. This is not possible, create a
       new trigger if needed.
     */
    E_ACT("setTrigger; this (combination) trigger cannot be re-used");
  }
  setTriggerName();
}

unsigned TriggerTarget::PullerData::newSpan
(const DataTimeSpec& tsext, TimeTickType previous_tick)
{
  // simple expansion of a repeat?
  if (tsext.getValidityStart() == time_tail->endtime &&
      tsext.getValiditySpan() == time_tail->ts.getValiditySpan()) {

    time_tail->endtime = tsext.getValidityEnd();
    DEBL("repeat extension" << time_tail->ts << " to " << time_tail->endtime);

    // is this grounds for new tipping?
    if (time_tail->endtime == time_tail->ts.getValiditySpan() + previous_tick &&
        time_tail->endtime > previous_tick) return 1U;
    return 0U;
  }

  // no simple expansion of the repeat, possibly because of a gap in the
  // timing, or irregular spans. Add a new TimingList entry
  if (tsext.extendsOrIsAfter(time_tail->endtime)) {
    TimeTickType previous_end = time_tail->endtime;
    time_tail = new PullerData::TimingList
      (tsext.modifyToAfter(time_tail->endtime), time_tail);
    DEBL("tail extension to " << time_tail->ts << " from " << tsext);

    // is this grounds for new tipping? Now the triggering may have changed
    // if the previous end prevented further triggering, and the current end
    // is past the last tick
    if (previous_end <= previous_tick &&
        time_tail->endtime > previous_tick) return 1U;
    return 0U;
  }

  // nothing added, no tipping
  return 0U;
}

bool TriggerTarget::PullerData::cleanUpTimingList(TimeTickType previous_tick)
{
  bool didclean = false;

  // total cleans
  while (time_head->next && time_head->endtime <= previous_tick) {
    PullerData::TimingListPtr to_delete = time_head;
    DEBL("cleaning " << time_head->ts << " vs " << previous_tick);
    time_head = time_head->next;
    delete to_delete;
    didclean = true;
  }

  // period updates
  if (time_head->endtime > previous_tick) {

    // common case first
    if (time_head->ts.getValidityEnd() == previous_tick) {
      DEBL("repeat clean " << time_head->ts << " vs " << previous_tick);
      time_head->ts += time_head->ts.getValiditySpan();
      return true;
    }

    if (time_head->ts.getValidityEnd() < previous_tick) {
      DEBL("burst clean " << time_head->ts << " vs " << previous_tick);
      TimeTickType span = time_head->ts.getValiditySpan();
      time_head->ts +=
        ((previous_tick - time_head->ts.getValidityStart()) / span) * span;
      DEBL("burst result " << time_head->ts);
      return true;
    }
  }
  return didclean;
}


void ConditionAnd::trigger(const DataTimeSpec& ts, unsigned idx)
{
  if (pullers[idx].newSpan(ts, previous_tick)) {
    DEBA("puller " << idx << " with " << ts << " extension " <<
         pullers[idx].time_tail->ts);
  }
  else {
    DEBA("puller " << idx << " with " << ts << " no extension " <<
         pullers[idx].time_tail->ts);
    return;
  }

  // todo; does not yet correctly consider timing gaps?
  bool moretriggers = true;
  TimeTickType tstart = previous_tick;

  do {
    TimeTickType tend = MAX_TIMETICK;

#ifdef DEBA
    unsigned idxp = 0;
#endif
    for (pullers_type::iterator ii = pullers.begin();
         ii != pullers.end(); ii++) {

#ifdef DEBA
      if (ts.getValidityStart() > ii->time_head->ts.getValidityEnd() + 10000) {
        DEBA(this->getTriggerName() << " congestion, puller " << idxp
             << " range "
             << ii->time_head->ts << ' ' << ii->time_tail->ts <<
             " demand " << idx << ' ' << ts);
      }
#endif

      // first clean up old stuff, leave at least one in the list
      // (test for ->next)
      ii->cleanUpTimingList(previous_tick);

      // now determine remaining span
      tstart = max(tstart, ii->time_head->ts.getValidityStart());
      tend = min(tend, ii->time_head->ts.getValidityEnd());
      DEBA("result " << idxp << " " << tstart << " - " << tend);
#ifdef DEBA
      idxp++;
#endif
    }

    // see what triggering is left
    if (tend > tstart || (tend == tstart && tstart > previous_tick)) {
      //TimeTickType newend = max(tstart, tend);
      TriggerPuller::pull(DataTimeSpec(tstart, tend));
      DEBA("triggered " << DataTimeSpec(tstart, tend));
      previous_tick = tend;
      tstart = tend;
    }
    else if (tstart > previous_tick) {
      // gap,
      DEBA(this->getTriggerName() << " gap " << tstart);
      previous_tick = tstart;
    }
    else {
      moretriggers = false;
    }
  } while (moretriggers);

  // keep previous result; note that a gap will move tstart to the
  // beginning of the time line after the gap; in a next round, old
  // pieces of other timelines will be cleared
  // previous_tick = tstart;
}

ConditionOrPtr  operator || (TriggerPuller& p1, TriggerPuller& p2)
{
  // is created with a refcount of 1
  ConditionOr *or1 = new ConditionOr();
  or1->addTerm(p1); or1->addTerm(p2);

  // release the refcount, ref is taken over by p1 and p2
#if PRELOAD_REF
  intrusive_ptr_release(or1);
#endif
  return ConditionOrPtr(or1);
}

ConditionOrPtr  operator || (ConditionOrPtr or1, TriggerPuller& p)
{
  or1->addTerm(p);
  return or1;
}

ConditionOrPtr operator || (TriggerPuller& p, ConditionOrPtr or1)
{
  or1->addTerm(p);
  return or1;
}

// the overloaded operators
ConditionAndPtr operator && (TriggerPuller& c1, TriggerPuller& c2)
{
  ConditionAnd *and1 = new ConditionAnd();
  and1->addTerm(c1); and1->addTerm(c2);
#if PRELOAD_REF
  intrusive_ptr_release(and1);
#endif
  return ConditionAndPtr(and1);
}

ConditionAndPtr operator && (ConditionAndPtr c1, TriggerPuller& c2)
{
  c1->addTerm(c2);
  return c1;
}

ConditionAndPtr operator && (TriggerPuller& c1, ConditionAndPtr c2)
{
  c2->addTerm(c1);
  return c2;
}

DUECA_NS_END
