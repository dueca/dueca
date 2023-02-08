/* ------------------------------------------------------------------   */
/*      item            : IncoCalculator.cxx
        made by         : Rene' van Paassen
        date            : 000412
        category        : body file
        description     :
        changes         : 000412 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define IncoCalculator_cc
#include "IncoCalculator.hxx"
#include <dueca-conf.h>

#ifdef BUILD_DMODULES

#define E_TRM
#define W_TRM
#define I_TRM
#define D_TRM
#include <debug.h>
#include <stringoptions.h>
#include <IncoCollaborator.hxx>
#include <IntervalCalculation.hxx>
#include <ParameterTable.hxx>
#include <dassert.h>
#include <IncoSpec.hxx>
#include <dueca/DataReader.hxx>
#include <dueca/DataWriter.hxx>

#define DO_INSTANTIATE
#include "MemberCall.hxx"
#include "Callback.hxx"
#include <TrimView.hxx>
#include <SimTime.hxx>

#include <debprint.h>
DUECA_NS_START

const char* const IncoCalculator::classname = "inco-calculator";

const ParameterTable* IncoCalculator::getParameterTable()
{
  static ParameterTable table[] = {
    {NULL, NULL,
    "The IncoCalculator module is a helper for calculating initial or trim\n"
    "conditions. Unfortunately, the thing is not finished yet."} };
  return table;
}


IncoCalculator::IncoCalculator(Entity* e,
                               const char* part,
                               const PrioritySpec& ps) :
  // first of all, this is a normal module
  Module(e, classname, part),

  // phase of the calculation
  calculation(Ready),

  // object that does the actual math, the IncoCalculator only
  // facilitates input and output
  calculator(new IntervalCalculation()),

  // mode for the calculation
  current_mode(FlightPath),

  // counter that can keep a check on whether all data has been
  // received.
  current_cycle(0),

  // check in with the view, this returns an integer handle for future
  // communications
  view_id(TrimView::single()->addEntity(getEntity(), this)),

  // no of targets used in a specific mode
  n_targets(0),

  n_controls(0),

  n_variables(0),

  // channel for initial condition specifications
  t_inco_spec(getId(),
              NameSet(getEntity(), "IncoSpec", ""),
              getclassname<IncoSpec>(), entry_any,
              Channel::Events),

  // this for listening to inco specifications
  cb1(this, &IncoCalculator::receiveNewIncoSpec),
  receive_spec(getId(), "receive inco spec", &cb1, ps)
{
  receive_spec.setTrigger(t_inco_spec);
  receive_spec.switchOn(TimeSpec(0,0));
}

bool IncoCalculator::isPrepared()
{
  // to do what?
  return true;
}

void IncoCalculator::startModule(const TimeSpec& ts)
{ }

void IncoCalculator::stopModule(const TimeSpec& ts)
{ }

IncoCalculator::~IncoCalculator()
{
  //TrimView::single().removeEntity(getEntity());
}

void IncoCalculator::receiveNewIncoSpec(const TimeSpec& t)
{
  // there is a new initial condition specification. Read it, and extend the
  // present infrastructure
  //const Event<IncoSpec>* e;
  //const IncoSpec* d;
  t_inco_spec.isValid();
  DataReader<IncoSpec, VirtualJoin> d(t_inco_spec);
  //  t_inco_spec.getNextEvent(e, t); d = e->getEventData();
  DEB1("inco specification " << d.data());

  // find the sender, and ensure it does not occur yet in our list
  const IncoCollaborator* c = findCollaborator(d.data().module);
  if (c != NULL) {
    cerr << "Got a second inco specification from " << d.data().module << endl;
    return;
  }

  // insert the collaborator's data. This also creates the
  // communication channels to receive inco results and for
  // controlling the inco calculation with this partner.
  IncoCollaborator* i = new IncoCollaborator(d.data(), this, n_variables);
  partners.push_back(i);
  n_variables += i->noVariables();
}


const IncoCollaborator*
IncoCalculator::findCollaborator(const NameSet& col) const
{
  for (list<IncoCollaborator*>::const_iterator ii = partners.begin();
       ii != partners.end(); ii++) {
    if ((*ii)->getModuleName() == col)
      return (*ii);
  }
  return NULL;
}

void IncoCalculator::processIncoResults(const TimeSpec& ts,
                                        IncoCollaborator*& col)
{
  // check the correct implementation of the calculation
  if (!col->processEvent(ts, current_mode)) {
    cerr << "IncoCalculator warning, mode=" << current_mode
         << " incorrect implementation by " << col->getModuleName() << endl;
  }

  // if the input cycle is complete, do a next iteration in the trim
  // value calculation
  if (cycleComplete()) {
    DEB1("A cycle completed");
    iterate();
  }
}

bool IncoCalculator::cycleComplete() const
{
  for (list<IncoCollaborator*>::const_iterator ii = partners.begin();
       ii != partners.end(); ii++) {
    if (current_cycle != (*ii)->getMark()) return false;
  }
  return true;
}

void IncoCalculator::iterate()
{
  // check whether all targets are met
  bool have_targets = true;
  for (list<IncoCollaborator*>::const_iterator ii = partners.begin();
       ii != partners.end(); ii++) {
    have_targets = have_targets &&
      (*ii)->haveTargets(current_mode);
  }

  if (have_targets) {
    calculation = Complete;
    DEB1("calculation complete");
    // \todo feedback to the interface
    return;
  }

  // we were called after a cycleComplete, i.e., all results are now
  // in the collaborators. Merge the result into the
  // intervalcalculation
  Vector y(n_targets);
  while (work_ids.size()) {

    // collect the results
    unsigned int idx = 0;
    for (list<IncoCollaborator*>::const_iterator ii = partners.begin();
         ii != partners.end(); ii++) {
      (*ii)->insertTargetResults(y, current_mode, idx);
    }
    assert(y.size() == idx);

    // feed these back into the calculator
    calculator->mergeResult(work_ids.front(), y);

    // this work id has been handled, pop it
    work_ids.pop_front();
  }

  // do a single update step
  calculator->step();

  // initiate the next iteration calculations
  newCalculations();
}

void IncoCalculator::initiate(IncoMode mode)
{
  if (calculation != Ready) {
    /* DUSIME system.

       Trim calculation is in progress, cannot start a new calculation
       at this time, command is ignored. */
    W_TRM(getId() << " Calculation ongoing, cannot start new");
    return;
  }

  // remember the mode we are running in
  current_mode = mode;
  calculation = Initialise;

  // count the number of targets and controls for this mode
  n_targets = 0; n_controls = 0;
  for (list<IncoCollaborator*>::const_iterator ii = partners.begin();
       ii != partners.end(); ii++) {
    (*ii)->count(current_mode, n_targets, n_controls);
  }

  // check that we have something to work with
  if (n_targets == 0 || n_controls == 0) {
    /* DUSIME system.

       The requested trim mode is not properly configured for trim
       calculation. */
    W_TRM(getId() << " Cannot calculate in mode " << current_mode <<
          " targets=" << n_targets << " controls=" << n_controls);
    calculation = Ready;
    return;
  }

  // find the ranges for the controls
  Vector xmin(n_controls), xmax(n_controls); unsigned int idx = 0;
  for (list<IncoCollaborator*>::const_iterator ii = partners.begin();
       ii != partners.end(); ii++) {
    (*ii)->fillMinMax(current_mode, idx, xmin, xmax);
  }
  assert(idx == n_controls);

  // now re-initialise the interval calculation with this
  calculator->initialise(xmin, xmax, n_targets);

  // and give the calculator the opportunity to set out calculation
  // queries
  newCalculations();

  // remember the phase of the calculation we are in
  calculation = Continue;
}

void IncoCalculator::newCalculations()
{
  // a vector for accepting the control input to the system
  Vector x(n_controls);

  // determine the time for which the events will be sent.
  sendtime = max(sendtime, SimTime::getTimeTick());

  int work_id;
  while ((work_id = calculator->needEvaluation(x)) != -1) {

    // yes, new calculation is needed, remember the work_id for this
    // calculation
    work_ids.push_back(work_id);

    // insert the desired x (vector with controls) into all the
    // collaborators and let these send out an event that initiates
    // the trim calculation
    unsigned int idx=0;
    for (list<IncoCollaborator*>::const_iterator ii = partners.begin();
         ii != partners.end(); ii++) {
      (*ii)->initiateCalculation(current_mode, sendtime, x, idx);
    }

    // we must have used all the elements of vector x
    assert(idx == x.size());

    // for the next round, a different time stamp must be used for the
    // events.
    sendtime++;
  }

  // remember the cycle (= last time stamp of the incoming events)
  // that we must wait for
  current_cycle = sendtime - 1;
}

IncoVariableWork& IncoCalculator::getIncoVariable(unsigned int variable)
{
  static IncoVariableWork dum;
  for (list<IncoCollaborator*>::iterator ii = partners.begin();
       ii != partners.end(); ii++) {
    if (variable >= (*ii)->getOffset() &&
        variable < (*ii)->getOffset() + (*ii)->noVariables()) {
      return (*ii)->getIncoVariable(variable);
    }
  }
  assert(0);
  return dum;
}
DUECA_NS_END
#endif
