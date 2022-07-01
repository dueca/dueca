/* ------------------------------------------------------------------   */
/*      Item            : IncoCollaborator.cxx
        made by         : Rene' van Paassen
        date            : 010402
        category        : body file
        description     :
        changes         : 010402 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define IncoCollaborator_cxx

#include <dueca-conf.h>

#include "IncoCollaborator.hxx"
#include <EventReader.hxx>
#include <IncoCalculator.hxx>
#include <TrimView.hxx>
#include <dassert.h>
#include <Event.hxx>

#define DO_INSTANTIATE
#include <Event.hxx>
#include <CallbackWithId.hxx>
#include <EventAccessToken.hxx>
#include <EventReader.hxx>
#include <WrapSendEvent.hxx>
#define D_TRM
#define I_TRM
#define W_TRM
#define E_TRM
#include <debug.h>
#include <debprint.h>

DUECA_NS_START


IncoCollaborator::IncoCollaborator(const IncoSpec& spec,
                                   IncoCalculator* calculator,
                                   unsigned int offset) :
  specification(spec),
  offset(offset),
  mark(0),
  t_inco_feedback(calculator->getId(), NameSet
                  (specification.module.getEntity(),
                   "IncoResults",
                   (vstring(specification.module.getClass()) +
                    vstring("/") +
                    vstring(specification.module.getPart())).c_str())),
  t_inco_control(calculator->getId(), NameSet
                 (specification.module.getEntity(),
                  "IncoControl",
                  (vstring(specification.module.getClass()) +
                   vstring("/") +
                   vstring(specification.module.getPart())).c_str())),
  cb(calculator, &IncoCalculator::processIncoResults, this),
  process(calculator->getId(), "receive inco data", &cb, PrioritySpec(0,0))
{
  process.setTrigger(t_inco_feedback);
  process.switchOn(TimeSpec(0,0));

  table.resize(spec.table.size());
  for (int ii = spec.table.size(); ii--; ) {
    table[ii] = spec.table[ii];
  }

  // add the variables to the graphical interface
  vector<vstring> names;
  names.push_back(specification.module.getEntity());
  if (specification.module.getPart().size())
    names.push_back(specification.module.getPart());
  names.push_back(specification.module.getClass());
  names.push_back("");

#if defined(BUILD_DMODULES)
  for (unsigned int ii = 0; ii < table.size(); ii++) {
    names.back() = table[ii].getName();
    TrimView::single()->addVariable
      (names, calculator->getViewId(), offset + ii,
       table[ii]);
  }
#endif
}

bool IncoCollaborator::processEvent(const TimeSpec& ts, IncoMode mode)
{
  // read out the event
  EventReader<IncoNotice> e(t_inco_feedback, ts);

  DEB1("processing " << e.data());

  // merge the present data into the table, and check for possible trouble
  bool ok = true;
  for (IncoNotice::const_iterator ii = e.data().ivlist.begin();
       ii != e.data().ivlist.end(); ii++) {
    ok = ii->index < table.size() &&
      table[ii->index].merge(mode, ii->value) && ok;
  }

  // keep a vector with results for the controls
  results.push_back(vector<double>());
  for (unsigned int ii = 0; ii < table.size(); ii++) {
    if (table[ii].findRole(mode) == Target) {
      results.back().push_back(0.0);
    }
  }

  // remember that this came in
  assert(ts.getValidityStart() > mark);
  mark = ts.getValidityStart();

  return ok;
}

bool IncoCollaborator::haveTargets(IncoMode mode) const
{
  bool meet_targets = true;
  for (unsigned int ii = table.size();
       meet_targets && ii--; ) {
    // meet_targets &= table[ii].meetsTarget(mode);
  }
  return meet_targets;
}

bool IncoCollaborator::insertTargetResults(Vector& y, IncoMode mode,
                                           unsigned int& idx)
{
  if (results.size() == 0) return false;
  for (unsigned int ii = 0; ii < results.front().size(); ii++) {
    assert(idx < y.size());
    y[idx++] = results.front()[ii];
  }
  results.pop_front();
  return true;
}

void IncoCollaborator::count(IncoMode mode,
                             unsigned int& n_targets,
                             unsigned int& n_controls) const
{
  for (unsigned int ii = 0; ii < table.size(); ii++) {
    if (table[ii].findRole(mode) == Target) {
      n_targets++;
    }
    else if (table[ii].findRole(mode) == Control) {
      n_controls++;
    }
  }
}

void IncoCollaborator::fillMinMax(IncoMode mode, unsigned int& idx,
                                  Vector& xmin, Vector& xmax) const
{
  for (unsigned int ii = 0; ii < table.size(); ii++) {
    if (table[ii].findRole(mode) == Control) {
      assert(idx < xmin.size() && idx < xmax.size());
      xmin[idx] = table[ii].getMin();
      xmax[idx++] = table[ii].getMax();
    }
  }
}

void IncoCollaborator::initiateCalculation(IncoMode mode,
                                           TimeTickType tick,
                                           const Vector& x,
                                           unsigned int& idx)
{
  // create the inco notice
  IncoNotice *seed = new IncoNotice(mode);

  // insert the x data into the controls
  for (unsigned int ii = 0; ii < table.size(); ii++) {

    if (table[ii].findRole(mode) == Control) {
      assert(idx < x.size());

      // insert x data
      table[ii].setValue(x[idx]);

      // append the data to the notice
      seed->appendPair(ii, x[idx++]);
    }
    else if (table[ii].findRole(mode) == Constraint) {

      // just append the data to the notice. The constraint is
      // supposed to be set from an interface.
      seed->appendPair(ii, table[ii].getValue());
    }
  }

  // That's it, send off the inco notice
  wrapSendEvent(t_inco_control, seed, tick);
}

IncoVariableWork& IncoCollaborator::getIncoVariable(unsigned int variable)
{
  assert(variable >= offset);
  assert(variable < offset + noVariables());
  return table[variable - offset];
}
DUECA_NS_END
