/* ------------------------------------------------------------------   */
/*      item            : SimulationModule.hxx
        made by         : Rene' van Paassen
        date            : 990713
        category        : header file
        description     :
        changes         : 990713 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef SimulationModule_hh
#define SimulationModule_hh

#ifdef SimulationModule_cc
#endif

#include "DusimeModule.hxx"
#include "SimulationState.hxx"
#include <dueca/ChannelReadToken.hxx>
#include <dueca/ChannelWriteToken.hxx>
#include "StateChange.hxx"
#include "SimTime.hxx"
#include "EntityCommand.hxx"
#include "EntityConfirm.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include <AsyncList.hxx>
#include <dueca_ns.h>
DUECA_NS_START

class TimeSpec;
struct IncoVariable;

/** A base class from which users can derive DUSIME modules.

    The SimulationModule base class implements the basic communication
    for a DUSIME module. By deriving from this class, using its
    methods to dermine the simulation state, and re-implementing
    applicable virtual methods, a fully DUSIME-aware class can be
    made, and objects of this class have coordinated start-stop
    abilities, the ability to calculate initial conditions and the
    ability for saving and restoring simulation state. */
class SimulationModule : public DusimeModule
{
private:
  /** Current mode of the simulation, advance, hold. */
  SimulationState                  current_state;

  /** Current mode of the simulation, advance or hold. */
  SimulationState                  current_report_state;

  /** Last time the simulation state was checked/updated. */
  TimeTickType                     last_check;

  /** A single-linked list, with all future simulation states. These
      can be read out by one thread, and pushed in by another, without
      locking. */
  AsyncList<StateChange<SimulationState> > future_states;

  /** An access token for reading the commands from the entity
      manager. */
  ChannelReadToken                         t_entity_commands;

  /** An access token for readback to the entity manager. */
  ChannelWriteToken                        t_entity_confirm;

  /** 1st callback object. */
  Callback<SimulationModule>              cb1;

  /** An actvity to react to entity commands. */
  ActivityCallback                        respond_to_entity;

private:
  /** copy constructor, assure no-one uses this inadvertently. */
  SimulationModule(const SimulationModule&);

  /** Callback for entity command input.
      \param t       Current simulation time span */
  void processEntityCommands(const TimeSpec& t);

protected:
  /** Constructor.
      \param e       Pointer to my entity
      \param m_class String with name of the module class
      \param part    String with part name
      \param table   Pointer to the table with initial condition
                     calculation definitions. If this class does not
                     take part in calculation of initial conditions
                     this pointer may be NULL.
      \param state_size Size of the state, as sent in a snapshot. */
  SimulationModule(Entity* e, const char* m_class, const char* part,
                   const IncoTable* table = NULL, int state_size = 0);

  /** Destructor. */
  virtual ~SimulationModule();

protected:
  /** Returns the state of the simulation at this time.
      To be used by the child, every time a model update is
      calculated,  for determining desired action. Note that this call
      MUST be used by the derived class, otherwise the state of the
      SimulationModule does not change, and with that the DUSIME state
      machine gets stuck.

      This call has a side effect; it checks for commanded state changes,
      and implements/confirms those when their time maches the passed
      TimeSpec.

      \param ts  Current time specification.
      \param confirm_transition  If true, immediately confirm a
                     transition to a new state.
      \returns   A SimulationState, restricted to only HoldCurrent,
                 Advance or Replay. */
  SimulationState::Type getAndCheckState(const TimeSpec& ts,
                                         bool confirm_transition = true);

  /** If a transition was not (implicitly) confirmed in the
      getAndCheckState call (second argument false), confirm with this
      call that it is now complete. */
  void transitionComplete();

  /** If you "forgot" what the state from a previous
      getAndCheckState() call was, you can remember it here.
      \returns   A SimulationState, restricted to only HoldCurrent,
                 Advance or Replay. */
  SimulationState::Type getCurrentState();
};
DUECA_NS_END
#endif

