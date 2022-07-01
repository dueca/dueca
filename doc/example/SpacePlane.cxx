/* ------------------------------------------------------------------   */
/*      item            : SpacePlane.cxx
        made by         : Rene' van Paassen
        date            : 001003
        category        : body file
        description     :
        changes         : 001003 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define SpacePlane_cxx

// include the definition of the module class
#include "SpacePlane.hxx"

// include additional files needed for your calculation here
#include "StatesOutputs.hxx"
#define D_MOD
#define E_MOD
#include <debug.h>

extern "C" {
// here we include the file with (static) routines for the RTW/Simulink model
#define RTW_MODEL complete_reg
#include "complete.c"
#include "rtw_prototypes.h"
}

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>

// define conversion factor to degrees
const double RAD_DEG = 180.0 / 3.1415926535897931E+000;

// class/module name
const char* const SpacePlane::classname = "space-plane";

// initial condition/trim table
// this does not work yet, so the table is empty
const IncoTable* SpacePlane::getMyIncoTable()
{
  static IncoTable inco_table[] = {
    // enter pairs of IncoVariable and VarProbe pointers (i.e.
    // objects made with new), in this table.
    // For example
//    {(new IncoVariable("example", 0.0, 1.0, 0.01))
//     ->forMode(FlightPath, Constraint)
//     ->forMode(Speed, Control),
//     new VarProbe<SpacePlane,double>
//       (REF_MEMBER(&SpacePlane::i_example))}

    // always close off with:
    { NULL, NULL} };

  return inco_table;
}

// parameters to be inserted
const ParameterTable* SpacePlane::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<SpacePlane,TimeSpec>
        (&SpacePlane::setTimeSpec)},

    { "set-stop-height",
      new VarProbe<SpacePlane,double>
      (REF_MEMBER(&SpacePlane::z_stop))},

    { "check-timing",
      new MemberCall<SpacePlane,vector<int> >
      (&SpacePlane::checkTiming)},

    // always close off with:
    { NULL, NULL} };

  return parameter_table;
}
/** e06 */

// constructor
SpacePlane::SpacePlane(Entity* e, const char* part, const
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
  SimulationModule(e, classname, part, getMyIncoTable(),
                   NSTATES*sizeof(real_T)),

  // initialize the data you need in your simulation
  S(NULL),
  z_stop(0.0),

  // initialize the data you need for the trim calculation

  // initialize the channel access tokens
  controls(getId(), NameSet(getEntity(), "PrimaryControls", part)),
  output(getId(), NameSet(getEntity(), "SpacePlaneY", part)),
  state(getId(), NameSet(getEntity(), "SpacePlaneState", part), 21),

  // activity initialization
  cb1(this, &SpacePlane::doCalculation),
  do_calc(getId(), "simulation step", &cb1, ps)
{
  // for a simulink model, you need to initialise infinity, - inf and
  // Not-a-number.
  rt_InitInfAndNaN(sizeof(real_T));

  // create a SimuLink/rtw model
  S = RTW_MODEL ();

  // initialize it
  MdlInitializeSizes(S);
  MdlInitializeSampleTimes(S);
  rt_CreateIntegrationData(S);
  const char* ret;
  if ((ret = rt_InitTimingEngine(S)) != NULL) {
    E_MOD(getId() << ' ' << ret);
  }
  MdlStart(S);

  // set the initial time
  ssSetT(S, 0.0);

  // and also calculate the initial state
  MdlUpdate(S, 0);
  /** e00 */
  // specify that the control input is the trigger for the calculation
  do_calc.setTrigger(controls);

  // just a check on the states defined and the model
  assert(X_no_states == NSTATES);
  assert(Y_no_outputs == NOUTPUTS);
}
/** e01 */

bool SpacePlane::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}
/** e02 */

// destructor
SpacePlane::~SpacePlane()
{
  // delete the model
  MdlTerminate(S);
}
/** e03 */

/** s02 */
// the setTimeSpec function
bool SpacePlane::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()
  // in this case, the Simulink model has to be told of the time base
  ssSetStepSize(S, ts.getDtInSeconds());

  // return true if everything is acceptable
  return true;
}
/** e04 */

// and the checkTiming function
bool SpacePlane::checkTiming(const vector<int>& i)
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
/** e05 */

// tell DUECA you are prepared
bool SpacePlane::isPrepared()
{
  // assume I am ready if all tokens are valid
  return controls.isValid() && output.isValid();
}

// start the module
void SpacePlane::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void SpacePlane::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}
/** e07 */

// fill a snapshot with state data. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void SpacePlane::fillSnapshot(const TimeSpec& ts,
                              Snapshot& snap, bool from_trim)
{
  // The most efficient way of filling a snapshot is with an AmorphStore
  // object.
  AmorphStore s(snap.accessData(), snap.getDataSize());

  if (from_trim) {
    // not implemented
  }
  else {
    // this is a snapshot from the running simulation. Dusime takes care
    // that no other snapshot is taken at the same time, so you can safely
    // pack the data you copied into (or left into) the snapshot state
    // variables in here
    // use packData(s, snapshot_state_variable1); ...
    for (int ii = 0; ii < NSTATES; ii++) {
      packData(s, s_x[ii]);
    }
  }
}

// reload from a snapshot. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void SpacePlane::loadSnapshot(const TimeSpec& ts, const Snapshot& snap)
{
  // access the data in the snapshot with an AmorphReStore object
  AmorphReStore store(snap.data, snap.data_size);

  D_MOD(getId() << " loading snapshot");

  // copy the state data into the state vector
  real_T* X = ssGetX(S);
  double t;
  for (int ii = 0; ii < NSTATES; ii++ ) {
    unPackData(store, t);
    X[ii] = t;
  }
}

/** s07 */
void SpacePlane::doCalculation(const TimeSpec& ts)
{
  // check the state we are supposed to be in
  switch (getAndCheckState(t)) {
  case SimulationState::HoldCurrent: {
    // only repeat the output, don not change the model state
    StreamWriter<SpacePlaneY> y(output, ts);
    real_T* Y = ssGetY(S);
    for (int ii = NOUTPUTS; ii--; ) {
      y.data().Y[ii] = Y[ii];
    }

    // send out the state also
    StreamWriter<SpacePlaneState> x(state, ts);
    real_T *X = ssGetX(S);
    for (int ii = NSTATES; ii--; ) {
      x.data().X[ii] = X[ii];
    }
  }
  break;
  /** e08a */

  case SimulationState::Replay:
  case SimulationState::Advance: {

    // access the input
    StreamReader<PrimaryControls> u(controls, ts);

    // copy the input to the simulink input
    double* U = ssGetU(S);
    U[0] = u.data().stick_pitch * RAD_DEG * 0.05;
    U[1] = u.data().stick_roll * RAD_DEG;

    // calculate the output/update the normal model
    for (int ii = 0; ii < NSAMPLE_TIMES; ii++)
      MdlOutputs(S, ii);

    // at this point, the output should be taken
    // so request access to the output channel
    StreamWriter<SpacePlaneY> y(output, ts);
    real_T* Y = ssGetY(S);
    for (int ii = NOUTPUTS; ii--; ) {
      y.data().Y[ii] = Y[ii];
    }
    /** s08a */

    // only do a model update if the height is above the stop height
    // below the stop height the thing will be frozen
    if (Y[Y_z] > z_stop) {

      // discrete update of the model,
      for (int ii = 0; ii < NSAMPLE_TIMES; ii++)
        MdlUpdate(S, ii);

      // specify the end time of the integration step
      ssSetSolverStopTime(S, ssGetT(S) + ssGetStepSize(S));

      // and do a continuous time step
      rt_UpdateContinuousStates(S);

      // and check for discrete time jumps
      rt_UpdateDiscreteTaskSampleHits(S);
    }

    // send out the state also
    StreamWriter<SpacePlaneState> x(state, ts);
    real_T *X = ssGetX(S);
    for (int ii = NSTATES; ii--; ) {
      x.data().X[ii] = X[ii];
    }
  }
  break;

  default:
    // other states should never be entered for a SimulationModule,
    // HardwareModules on the other hand have more states. Throw an
    // exception if we get here,
    throw CannotHandleState(getId(),GlobalId(), "state unhandled");
  }
  /** s08b */

  if (snapshotNow()) {

    // keep a copy of the model state. Snapshot sending is done in the
    // sendSnapshot routine, later, and possibly at lower priority
    // e.g.
    // snapshot_state_variable1 = state_variable1; ...
    // (or maybe if your state is very large, there is a cleverer way ...)
    real_T *X = ssGetX(S);
    for (int ii = NSTATES; ii--; )
      s_x[ii] = X[ii];
  }
}
/** e09 */

/* the following is not implemented
void SpacePlane::trimCalculation(const TimeSpec& ts, const TrimMode& mode)
*/

/** s10 */
// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<SpacePlane> a(SpacePlane::getMyParameterTable());


