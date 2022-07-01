/* ------------------------------------------------------------------   */
/*      item            : IncoCalculator.hh
        made by         : Rene' van Paassen
        date            : 000412
        category        : header file
        description     :
        changes         : 000412 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef IncoCalculator_hh
#define IncoCalculator_hh

#include <list>
using namespace std;

#include "Module.hxx"
#include "IncoSpec.hxx"
#include "IncoNotice.hxx"
#include "EventAccessToken.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include "EventReader.hxx"
#include <dueca_ns.h>
#include "IncoVariableWork.hxx"

DUECA_NS_START
class IncoCollaborator;
class IntervalCalculation;
typedef IntervalCalculation TrimCalculator;

/** A "big" class, a module that does trim condition
    calculations. This module should be part of the entity that it
    performs the calculations for (i.e. with two planes, you get two
    independent interfaces for trim condition calculation.

    This class receives specifications for the trim condition
    calculation, opens an interface on the experiment leaders'
    console, and offers the possiblilty for calculating trim
    conditions. */
class IncoCalculator: public Module
{
  /** Different phases in the calculation of a trim condition. */
  enum CalculationPhase {
    Ready,
    Initialise,
    Continue,
    Complete
  };

  /** The current phase of the calculation. */
  CalculationPhase calculation;

  /** The helper that does the actual calculation. */
  TrimCalculator*  calculator;

  /** Mode for which the calculation takes place. */
  IncoMode         current_mode;

  /** Cycle number in the calculation. This is used to link the
      incoming results to the try. */
  TimeTickType     current_cycle;

  /** ID that links me to the view. */
  int           view_id;

  /** No of targets. This will vary from mode to mode, so this has to
      be re-calculated at the start of each trim calculation. */
  unsigned int  n_targets;

  /** No of controls. This will vary from mode to mode, so this has to
      be re-calculated at the start of each trim calculation. */
  unsigned int  n_controls;

  /** Total number of inco variables handled here. */
  unsigned int  n_variables;

  /** The simulation time for which a query is sent out. */
  TimeTickType  sendtime;

  /** A list with "work id's". The calculator will give me several
      calculations to be done in one go. Each calculation has a work
      id, and these must be given back when the data are to be fed
      into the calculator. */
  list<int> work_ids;

  /** A list with all the collaborators. A collaborator handles the IO
      with one of the modules participating in the calculation,
      i.e. it will send out new values for the controls, and handle
      the results from that module. */
  list<IncoCollaborator*> partners;

  /** Communication, for reception of inco specifications. */
  EventChannelReadToken<IncoSpec>         t_inco_spec;

  /** Callback object to implement the activity. */
  Callback<IncoCalculator>                cb1;

  /** Activity of receiving specifications. */
  ActivityCallback                        receive_spec;

  /** The And condition, that lets me wait on all incoming
      results. */
  ConditionAnd                            update_trigger;

public:
  /** Name of the module class. */
  static const char* const           classname;

  /** Table, still without tunable parameters. */
  static const ParameterTable* getParameterTable();

public:
  /** Constructor. */
  IncoCalculator(Entity* e, const char* part, const PrioritySpec& ps);

  /** Destructor. */
  ~IncoCalculator();

  /** Return the view id. */
  inline int getViewId() {return view_id;}

  /** Function that receives the Initial Condition specification from
      "participating" modules. Upon reception the calculation will be
      extended with contribution from these modules. */
  void receiveNewIncoSpec(const TimeSpec& t);

  /** Function receiving Initial Condition calculation results. */
  void processIncoResults(const TimeSpec& t, IncoCollaborator*& col);

  /** Would officially start the module. */
  void startModule(const TimeSpec& ts);

  /** Would officially stop the module. */
  void stopModule(const TimeSpec& ts);

  /** Indicate that the module is ready to start -- always. */
  bool isPrepared();

  /** Return a reference to an inco variable, given the variable
      no. */
  IncoVariableWork& getIncoVariable(unsigned int variable);

private:
  /** Find the collaborator that matches a specific name */
  const IncoCollaborator* findCollaborator(const NameSet& col) const;

  /** Check all collaborators to see whether a send/calculate/receive
      cycle is completed. */
  bool cycleComplete() const;

  /** Calculate a new cycle. */
  void iterate();

  /** This to indicate that all data have come in again, and it does a
      series of new calculations. */
  void newCalculations();

public:
  /** Initiate a new calculation cycle. */
  void initiate(IncoMode mode);
};
DUECA_NS_END
#endif

