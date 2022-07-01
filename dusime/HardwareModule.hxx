/* ------------------------------------------------------------------   */
/*      item            : HardwareModule.hh
        made by         : Rene' van Paassen
        date            : 20001010
        category        : header file
        description     :
        changes         : 20001010 Rvp first version
                          20160517 RvP, return SimulationState::Type for
                                   getCurrentState
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef HardwareModule_hh
#define HardwareModule_hh

#ifdef HardwareModule_cc
#endif

#include "DusimeModule.hxx"
#include "SimulationState.hxx"
#include "StateChange.hxx"
#include "SimTime.hxx"
#include "EventAccessToken.hxx"
#include "EntityCommand.hxx"
#include "EntityConfirm.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include <AsyncList.hxx>
#include <dueca_ns.h>
DUECA_NS_START

/** HardwareModule, a base class for modules that interact with hardware.

    A base class from which users can derive Dusime hardware modules,
    i.e. modules that do the IO with physical hardware, and that thus, in
    addition, require extra facilities, such as:

    * calibration of the hardware
    * a safety mode for driving the hardware to a safe state
    * transitional states, e.g. for moving hardware
    */
class HardwareModule: public DusimeModule
{
private:
  /** Current mode of the simulation, advance, holdcurrent, calibrate
      or a transition between one of these. */
  SimulationState                          current_state;

  /** Last tick for which the state was checked. */
  TimeTickType                             last_check;

  /** A single-linked list, with all future simulation states. These
      can be read out by one thread, and pushed in by another, without
      locking. */
  AsyncList<StateChange<SimulationState> > future_states;

  /** An access token for reading the commands from the entity
      manager. */
  EventChannelReadToken<EntityCommand>     t_entity_commands;

  /** An access token for readback to the entity manager. */
  EventChannelWriteToken<EntityConfirm>    t_entity_confirm;

  /** 1st callback object. */
  Callback<HardwareModule>                 cb1;

  /** An actvity to react to entity commands. */
  ActivityCallback                         respond_to_entity;

  /** Copy protection. */
  HardwareModule(const HardwareModule&);

  /** Assignment protection. */
  HardwareModule& operator = (const HardwareModule& o);

private:
  /** Callback for entity command input. */
  void processEntityCommands(const TimeSpec& t);

protected:
  /** Constructor.
      \param e        Pointer to the entity.
      \param m_class  Module class name.
      \param part     Part name for the module.
      \param table    Table with initial condition calculation
                      specification.
      \param state_size Size of a snapshot of the state. */
  HardwareModule(Entity* e, const char* m_class, const char* part,
                 const IncoTable* table = NULL, int state_size = 0);

  /** Destructor. */
  virtual ~HardwareModule();

protected:

  /** Returns the state of the simulation at this time.
      To be used by the child, every time a model update is
      calculated,  for determining desired action. */
  SimulationState::Type getAndCheckState(const TimeSpec& t);

  /** Find the current state, previously calculated with
      getAndCheckState. */
  SimulationState::Type getCurrentState();

  /** Indicate that a transition is complete.

      In transitional states (e.g. Inactive_HoldCurrent,
      Calibrate_HoldCurrent), a HardwareModule must confirm when the
      transition is completed through this call. */
  void transitionComplete();

private:
  /** A method that is called by CriticalActivity whenever a safety
      stop is requested. Here we reset the simulationstate, so that we
      don't get any nasty surprises at re-start.

      Re-implementation of Module::setSafetyStop() */
  void setSafetyStop();
};
DUECA_NS_END
#endif
