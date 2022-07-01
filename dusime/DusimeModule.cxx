/* ------------------------------------------------------------------   */
/*      item            : DusimeModule.cxx
        made by         : Rene' van Paassen
        date            : 20001010
        category        : body file
        description     :
        changes         : 20001010 RvP first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define DusimeModule_cxx
#include "DusimeModule.hxx"
#include <dueca-conf.h>
#include <Snapshot.hxx>
#include <EventAccessToken.hxx>
#include <EventReader.hxx>
#include <GenericVarIO.hxx>
#include <Callback.hxx>
#include <Activity.hxx>
#include <IncoSpec.hxx>
#include <IncoNotice.hxx>
#include <IncoTable.hxx>

#define W_MOD
#define E_MOD
#include <debug.h>

#define DO_INSTANTIATE
#include <EventAccessToken.hxx>
#include <EventWriter.hxx>
#include <Callback.hxx>
#include <Event.hxx>
#include <WrapSendEvent.hxx>

#include <debprint.h>

DUECA_NS_START;

DusimeModule::DusimeModule(Entity* e,
                           const char* m_class, const char* part,
                           const IncoTable* inco_table,
                           int state_size) :
  Module(e, m_class, part),

  // snapshot
  state_size(state_size),
  snap_state(SnapshotState::SnapClear),
  future_snap_time(MAX_TIMETICK),
  t_snapshot_send(),
  t_snapshot_read(),
  cb1(this, &DusimeModule::localLoadSnapshot),
  load_snapshot(),

  // trim calculation
  t_inco_spec(),
  t_inco_input(),
  t_inco_feedback(),
  inco_table(inco_table),
  inco_table_size(0),
  cb2(this, &DusimeModule::localIncoCalculation),
  cb3(this, &DusimeModule::sendIncoSpecification),
  do_inco_calculation(NULL)
{
  // check the inco table size
  if (inco_table != NULL) {
    while (inco_table[inco_table_size].incovar != NULL &&
           inco_table[inco_table_size].probe != NULL) inco_table_size++;
  }

  // connect the snapshot facilities
  if (state_size) {
    // create a send and read token
    t_snapshot_send.reset
      (new ChannelWriteToken
       (getId(), NameSet(getEntity(), Snapshot::classname, "get"),
        Snapshot::classname, this->getNameSet().name, Channel::Events,
        Channel::OneOrMoreEntries, Channel::MixedPacking, Channel::Bulk));
    t_snapshot_read.reset
      (new ChannelReadToken
       (getId(), NameSet(getEntity(), Snapshot::classname, "set"),
        Snapshot::classname, entry_any, Channel::Events,
        Channel::ZeroOrMoreEntries, Channel::ReadAllData));

    // create the activity to react to the arrival of data
    load_snapshot.reset
      (new ActivityCallback
       (getId(), "load snapshot", &cb1, PrioritySpec(0, 0)));

    // connect this activity
    load_snapshot->setTrigger(*t_snapshot_read);
    load_snapshot->switchOn(TimeSpec(0,0));
  }

  // connect the inco calculation
  if (inco_table != NULL) {

    // create a token to send off the trim specification
    t_inco_spec.reset
      (new ChannelWriteToken
       (getId(), NameSet(getEntity(), IncoSpec::classname, ""),
        IncoSpec::classname, this->getNameSet().name,
        Channel::Events, Channel::OneOrMoreEntries,
        Channel::MixedPacking, Channel::Bulk, &cb3));

    // tokens to read and write trim commands and results
    t_inco_input.reset
      (new ChannelReadToken
       (getId(), NameSet(getEntity(), IncoNotice::classname, "ctrl"),
        IncoNotice::classname, this->getNameSet().name, Channel::Events,
        Channel::ZeroOrMoreEntries, Channel::ReadAllData));
    t_inco_feedback.reset
      (new ChannelWriteToken
       (getId(), NameSet(getEntity(), IncoNotice::classname, "res"),
        IncoNotice::classname, this->getNameSet().name, Channel::Events,
        Channel::OneOrMoreEntries, Channel::MixedPacking, Channel::Bulk));

    // the activity for trim calculation
    do_inco_calculation.reset
      (new ActivityCallback
       (getId(), "trim calculation", &cb2, PrioritySpec(0, 0)));

    // connect this activity
    do_inco_calculation->setTrigger(*t_inco_input);
    do_inco_calculation->switchOn(TimeSpec(0,0));
  }
}


DusimeModule::~DusimeModule()
{
  //
}

void DusimeModule::localLoadSnapshot(const TimeSpec& ts)
{
  if (!t_snapshot_read->isValid()) {
    /* DUSIME system.

       Snapshot channel token is not valid, cannot read the snapshot
       channel. Check the configuration, and check that the channel is
       being written. */
    W_MOD("cannot read snapshot channel " << t_snapshot_read->getName());
    return;
  }

  try {
    // read the snapshot from the channel
    DataReader<Snapshot,VirtualJoin> r(*t_snapshot_read, ts);

    // check that it is for me
    if (r.data().originator != getNameSet()) {
      /* DUSIME system.

         This snapshot is for another module, ignored. */
      I_MOD(getId() << " snapshot ignored, not for me");
    }
    else {

      // get the descendant to load it
      loadSnapshot(ts, r.data());
      DEB1(getId() << " restored state from snapshot at " << ts);
    }
  }
  catch (const std::exception& e) {
    /* DUSIME system.

       Unforeseen error in reading a snapshot. */
    W_MOD("Problem reading snapshot at " << ts << " " << e.what());
  }
}

void DusimeModule::localSendSnapshot(const TimeSpec& ts, bool from_trim)
{
  if (state_size && t_snapshot_send->isValid()) {

    // allocate the snapshot
    DataWriter<Snapshot> ew(*t_snapshot_send, ts, state_size);

    // set the originator
    ew.data().originator = getNameSet();

    // call the routine that actually fills the snapshot data
    fillSnapshot(ts, ew.data(), from_trim);

    DEB1(getId() << " sent off snapshot at " << ts
          << ", trim=" << from_trim);
  }

  // update the state
  future_snap_time = MAX_TIMETICK;
  snap_state = SnapshotState::SnapSent;
}

void DusimeModule::sendIncoSpecification(const TimeSpec& ts)
{
  if (!t_inco_spec->isValid()) {
    /* DUSIME system.

       Channel token for writing inco specification is not valid,
       check the channel configuration. */
    W_MOD("cannot write inco specification");
    return;
  }

  DataWriter<IncoSpec> ew(*t_inco_spec, ts);
  ew.data().module = getNameSet();
  ew.data().setTable(inco_table);
}

void DusimeModule::localIncoCalculation(const TimeSpec& ts)
{
  // channel must be valid for this to happen
  t_inco_input->isValid();

  // read the inco notice that has come in
  DataReader<IncoNotice> r(*t_inco_input, ts);

  // the data points to an event with a list of {index,value}
  // pairs. From the mode, also contained in that event, we determine
  // what should happen to the inco var
  for (IncoNotice::const_iterator ii = r.data().ivlist.begin();
       ii != r.data().ivlist.end(); ii++) {
    if (ii->index < inco_table_size ) {
      // this takes the supplied value (if appropriate) and modifies
      // one of the states
      if (inco_table[ii->index].incovar->
          queryInsertForThisMode(r.data().mode))
        inco_table[ii->index].probe->poke(this, ii->value);
    }
    else {
      /* DUSIME system.

         Received a trim command that exceeds the inco table, probably
         an error in DUSIME trim code. */
      W_MOD("Index " << ii->index << " exceeds inco table size" << endl);
    }
  }

  // Now the variables have been set. Let the module calculate the
  // trim condition
  trimCalculation(ts, r.data().mode);

  // get the results back to the inco modules. For all table entries
  // that are targets in this inco mode, the result is sent back
  // as a side effect, it is checked whether controls and constraints
  // have their original value
  IncoNotice * result = new IncoNotice(r.data().mode);
  for (int ii = 0; ii < inco_table_size; ii++) {
    // as a default, everything INCO is sent back, so that constraints
    // and controls can be checked.
    double d; float f; int i;
    switch(inco_table[ii].probe->getType()) {
    case Probe_double:
      inco_table[ii].probe->peek(this, d);
      f = d;
      break;
    case Probe_int:
      inco_table[ii].probe->peek(this, i);
      f = i;
      break;
    case Probe_float:
      inco_table[ii].probe->peek(this, f);
      break;
    default:
      cerr << "Cannot use type " << inco_table[ii].probe->getType()
           << " in inco calculation for "
           << inco_table[ii].incovar->getName() << endl;
      f = 0.0;
    }
    result->appendPair(ii, f);
  }

  // That's it, send off the inco notice
  if (!t_inco_feedback->isValid()) {
    /* DUSIME system.

       Channel token for initial condition feedback is not
       valid. Check that all required trim modules are active. */
    W_MOD("cannot send inco results");
    return;
  }

  wrapSendEvent(*t_inco_feedback, result, ts.getValidityStart());

  DEB1(getId() << " performed trim calculation");
}


bool DusimeModule::snapshotNow()
{
  return snap_state == SnapshotState::SnapNow;
}


void DusimeModule::fillSnapshot(const TimeSpec& t,
                                Snapshot& snap, bool inco)
{
  // check one thing, if the fillSnapshot was not overloaded, then
  // the snapshot contains random data. That is at least a warning!
  /* DUSIME system.

     fillSnapshot function is not overloaded, not sending data. */
  W_MOD("module " << getId() <<
	" has state, but fillSnapshot not defined");
}


void DusimeModule::loadSnapshot(const TimeSpec &ts,
                                const Snapshot &snap)
{
  // check one thing, if the loadSnapshot was not overloaded, then
  // snapshot loading did not work
  /* DUSIME system.

     loadSnapshot function is not overloaded, not interpreting
     snapshot data. */
  W_MOD("module " << getId() <<
	" received snapshot, but loadSnapshot not defined");
}

void DusimeModule::trimCalculationCondition(TriggerPuller& cond)
{
  if (inco_table != NULL) {

    // override the current trim calculation condition
    do_inco_calculation->setTrigger(*t_inco_input && cond);
    //  (reinterpret_cast<const TriggerPuller&>(conditionAnd));
  }
  else {
    /* DUSIME system.

       Inco table missing for this module, cannot do trim
       calculations. */
    W_MOD(getId() <<
          " specified a trim calculation condition but no inco table");
  }
}

void DusimeModule::trimCalculation(const TimeSpec& t, const TrimMode& mode)
{
  // if called, this must have been overridden. So a warning is in
  // order
  /* DUSIME system.

     Trim calculation function not implemented for this module, cannot
     do trim calculations. */
  W_MOD("Module " << getId() << " trimCalculation not implemented!");
}
DUECA_NS_END
