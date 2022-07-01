/* ------------------------------------------------------------------   */
/*      item            : @Module@.cxx
        made by         : @author@
        from template   : HardwareModuleTemplate.cxx (2022.06)
        date            : @date@
        category        : body file
        description     :
        changes         : @date@ first version
        language        : C++
        copyright       : (c)
*/


#define @Module@_cxx
// include the definition of the module class
#include "@Module@.hxx"

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
//#define D_MOD
//#define I_MOD
#include <debug.h>

// class/module name
const char* const @Module@::classname = "@smodule@";

// initial condition/trim table
const IncoTable* @Module@::getMyIncoTable()
{
  static IncoTable inco_table[] = {
    // enter pairs of IncoVariable and VarProbe pointers (i.e.
    // objects made with new), in this table.
    // For example
//    {(new IncoVariable("example", 0.0, 1.0, 0.01))
//     ->forMode(FlightPath, Constraint)
//     ->forMode(Speed, Control),
//     new VarProbe<_ThisModule_,double>
//       (REF_MEMBER(&_ThisModule_::i_example))}

    // always close off with:
    { NULL, NULL} };

  return inco_table;
}

// parameters to be inserted
const ParameterTable* @Module@::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_,std::vector<int> >
      (&_ThisModule_::checkTiming), check_timing_description },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "please give a description of this module" } };

  return parameter_table;
}

// constructor
@Module@::@Module@(Entity* e, const char* part, const
                       PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments.
     If you give a NULL pointer instead of the inco table, you will not be
     called for trim condition calculations, which is normal if you for
     example implement logging or a display.
     If you give 0 for the snapshot state, you will not be called to
     fill a snapshot, or to restore your state from a snapshot. Only
     applicable if you have no state. */
  HardwareModule(e, classname, part, getMyIncoTable(), @statesize@),

  // initialize the data you need in your simulation
  hardware_ok(false),

  // initialize the data you need for the trim calculation

  // initialize the channel access tokens

  // activity initialization
  cb1(this, &_ThisModule_::doCalculation),
  cb2(this, &_ThisModule_::doSafeWork),
  do_calc(this, "@activityname@", &cb1, &cb2, ps)
{
  // do the actions you need for the simulation

  // connect the triggers for simulation
  do_calc.setTrigger(/* fill in your triggering channels */);

  // connect the triggers for trim calculation. Leave this out if you
  // don not need input for trim calculation
  trimCalculationCondition(/* fill in your trim triggering channels */);
}

bool @Module@::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
@Module@::~@Module@()
{
  //
}

// as an example, the setTimeSpec function
bool @Module@::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool @Module@::checkTiming(const std::vector<int>& i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

// tell DUECA you are prepared to start your safety work
bool @Module@::isInitialPrepared()
{
  // do whatever checks on the hardware that you need before working
  // with it. Return true when things are (seem?) OK
}

// tell DUECA you are prepared
bool @Module@::isPrepared()
{
  // do whatever additional calculations you need to prepare the model.

  bool res = true;

  // Example checking a token:
  // CHECK_TOKEN(w_somedata);

  // Example checking anything
  // CHECK_CONDITION(myfile.good());

  // return result of checks
  return res;
}

// start the module
void @Module@::initialStartModule(const TimeSpec &time)
{
  hardware_ok = false;
  do_calc.switchOn(time);
}

// start the module
void @Module@::startModule(const TimeSpec &time)
{
  do_calc.switchWork(time);
}

// stop the module work
void @Module@::stopModule(const TimeSpec &time)
{
  // unset the hardware_ok flag, so the safe strategy is forced to re-evaluate
  hardware_ok = false;
  do_calc.switchSafe(time);
}

// also stop the safe mode
void @Module@::finalStopModule(const TimeSpec& time)
{
  do_calc.switchOff(time);
}

// fill a snapshot with state data. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void @Module@::fillSnapshot(const TimeSpec& ts,
                            Snapshot& snap, bool from_trim)
{
  // The most efficient way of filling a snapshot is with an AmorphStore
  // object.
  AmorphStore s(snap.accessData(), snap.getDataSize());

  if (from_trim) {
    // use packData(s, trim_state_variable1); ... to pack your state into
    // the snapshot
  }
  else {
    // this is a snapshot from the running simulation. Dusime takes care
    // that no other snapshot is taken at the same time, so you can safely
    // pack the data you copied into (or left into) the snapshot state
    // variables in here
    // use packData(s, snapshot_state_variable1); ...
  }
}

// reload from a snapshot. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void @Module@::loadSnapshot(const TimeSpec& t, const Snapshot& snap)
{
  // access the data in the snapshot with an AmorphReStore object
  AmorphReStore s(snap.data, snap.getDataSize());

  // use unPackData(s, real_state_variable1 ); ... to unpack the data
  // from the snapshot.
  // You can safely do this, while snapshot loading is going on the
  // simulation is in HoldCurrent or the activity is stopped.
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void @Module@::doCalculation(const TimeSpec& ts)
{
  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::Inactive: {
    // in this state, the hardware must be kept in a safe condition.
    // this state is entered from the safe state (given that isPrepared
    // returned true) and is also the state from which we will normally
    // return to safe, so do the same stuff as in safe work, and there will
    // be no bumps in transition. Also repeat (possibly dummy) output, as
    // in HoldCurrent

  }
  break;

  case SimulationState::Inactive_HoldCurrent: {
    // do the transition from inactive to the position/state for the
    // hardware in HoldCurrent. Also repeat (possibly dummy) output, as
    // in HoldCurrent

    // if the state transition is complete, tell the module
    if (/* conditions for state transition complete*/) {
      transitionComplete();
    }
  }
  break;

  case SimulationState::HoldCurrent: {
    // only repeat the output, don not change the model state

  }
  break;

  case SimulationState::Calibrate_HoldCurrent: {
    // in this state, the calibration of the hardware has to be carried
    // Also repeat (possibly dummy) output, as in HoldCurrent

    // As soon as the calibration is complete, tell the module
    // after this you fall back immediately to HoldCurrent, so reset
    // the hardware to the HoldCurrent position before calling this!
    if (/* conditions for state transition complete*/) {
      transitionComplete();
    }
  }
  break;

  case SimulationState::Advance: {
    // access the input
    // example:
    // try {
    //   DataReader<MyInput> u(input_token, ts);
    //   throttle = u.data().throttle;
    //   de = u.data().de; ....
    // }
    // catch(Exception& e) {
    //   // strange, there is no input. Should I try to continue or not?
    // }
    /* The above piece of code shows a block in which you try to catch
       error conditions (exceptions) to handle the case in which the input
       data is lost. This is not always necessary, if you normally do not
       foresee such a condition, and you don t mind being stopped when
       it happens, forget about the try/catch blocks. Then the
       CriticalActivity will catch any exceptions, and revert to safe mode */

    // do the simulation calculations, one stp
  }
  break;

  case SimulationState::Advance_HoldCurrent: {
    // return the device from wherever the Advance state left it to
    // the holdcurrent position

    // after reaching HoldCurrent position, call this
    if (/* conditions for state transition complete*/) {
      transitionComplete();
    }
  }
  break;

  case SimulationState::HoldCurrent_Inactive: {
    // reset the hardware to the inactive position, and keep outputting
    // those dummy values!

    // after reaching inactive position, call this
    if (/* conditions for state transition complete*/) {
      transitionComplete();
    }
  }
  break;

  default:
    // other states should never be entered, however, if in the future
    // the system is expanded, warn that things have gone wrong.
    // throwing the exception will cause the safe activity to be called
    // in the future
    throw CannotHandleState(getId(),GlobalId(), "state unhandled");
  }

  // DUECA applications are data-driven. From the time a module is switched
  // on, it should produce data, so that modules "downstreams" are
  // activated
  // access your output channel(s)
  // example
  // DataWriter<MyOutput> y(output_token, ts);

  // write the output into the output channel, using the stream writer
  // y.data().var1 = something; ...

  if (snapshotNow()) {
    // keep a copy of the model state. Snapshot sending is done in the
    // sendSnapshot routine, later, and possibly at lower priority
    // e.g.
    // snapshot_state_variable1 = state_variable1; ...
    // (or maybe if your state is very large, there is a cleverer way ...)
  }
}

void @Module@::doSafeWork(const TimeSpec& ts)
{
  // you may not assume anything about communications, channels, etcetera
  // only control your hardware into a safe mode. No getAndCheckState either,
  // this runs completely alone, aside from comm with the hardware

  // only when you think you are ready to go "live", set a flag that
  // indicates this
  // hardware_ok = true;
}

void @Module@::trimCalculation(const TimeSpec& ts, const TrimMode& mode)
{
  // read the event equivalent of the input data
  // example
  // DataReader<MyInput> u(i_input_token, ts);

  // using the input, and the data put into your trim variables,
  // calculate the derivative of the state. DO NOT use the state
  // vector of the normal simulation here, because it might be that
  // this is done while the simulation runs!
  // Some elements in this state derivative are needed as target, copy
  // these out again into trim variables (see you TrimTable

  // trim calculation
  switch(mode) {
  case FlightPath: {
    // one type of trim calculation, find a power setting and attitude
    // belonging to a flight path angle and speed
  }
  break;

  case Speed: {
    // find a flightpath belonging to a speed and power setting (also
    // nice for gliders)
  }
  break;

  case Ground: {
    // find an altitude/attitude belonging to standing still on the
    // ground, power/speed 0
  }
  break;

  default:
    W_MOD(getId() << " cannot calculate inco mode " << mode);
  break;
  }

  // This works just like a normal calculation, only you provide the
  // steady state value (if your system is stable anyhow). So, if you
  // have other modules normally depending on your output, you should
  // also produce the equivalent output here.
  // DataWriter<MyOutput> y(output_token, ts);

  // write the output into the output channel, using the DataWriter

  // now return. The real results from the trim calculation, as you
  // specified them in the TrimTable, will now be collected and sent
  // off for processing.
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<@Module@> a(@Module@::getMyParameterTable());

