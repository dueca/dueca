/* ------------------------------------------------------------------   */
/*      item            : DusimeController.hxx
        made by         : Rene' van Paassen
        date            : 001010
        category        : header file
        description     :
        changes         : 001010 first version
                          120905 re-factoring, split into gui/controller
                                 part
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DusimeController_hh
#define DusimeController_hh

#include "Module.hxx"
#include "SimulationState.hxx"
#include "SimTime.hxx"
#include "EntityCommand.hxx"
#include "EntityConfirm.hxx"
#include "EventAccessToken.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include <SimStateRequest.hxx>
#include <dueca_ns.h>
#include <AperiodicAlarm.hxx>
#include <dueca.h>

DUECA_NS_START

/** This is a definition of a singleton object that controls the
    Dusime modules (HardwareModule, SimulationModule) in a
    dueca/dusime system. This controller accepts signals from the
    interface (button presses etc.) and communicates with the
    different modules. */
class DusimeController : public Module
{
protected:
  /** State of the simulation, as commanded to the model. */
  SimulationState                        commanded_state;

  /** Confirmed state from the simulation. */
  SimulationState                        confirmed_state;

  /** Confirmed state from the simulation. */
  SimulationState                        previous_confirmed_state;

  /** Transition state commanded after pressing a button. */
  SimulationState                        new_state;

  /** Flag that indicates that a state change has been commanded. */
  bool                                   state_has_changed;

  /** Flag that is set when replay is possible. */
  bool                                   replay_ready;

  /** Flag to remember the calibration pass. */
  bool                                   calibrated;

  /** Flag to remember start. */
  bool                                   waiting_for_emanager;

  /** Flag to remember dirt in the state. */
  bool                                   state_dirty;

  /** Earliest time at which a new mode change request can be made. */
  TimeTickType                           earliest_change;

  /** Minimum interval of mode change requests. */
  TimeTickType                           min_interval;

  /** Minimum advance notification required for a change request. */
  TimeTickType                           min_notification;

  /** Block programmatic transition to advance */
  bool                                   block_advance;

  /** Flag / hack to run without gui, for gui-derived classes */
  bool                                   use_gui;

private:
  /** Counter to keep down the no of confirm messages, when all states
      are aligned. */
  int                                    confirm_divisor;

  /** Channel to write the entity commands. */
  ChannelWriteToken                      t_entity_commands;

  /** Channel over which confirmation is received. */
  ChannelReadToken                       t_entity_confirm;

  /** Channel over which -- possibly -- the application can send state
      change requests. */
  ChannelReadToken                       t_state_request;

  /** Channel with confirmation of mode changes. */
  ChannelWriteToken                      t_confirmed_state;

  /** Callback object 1. */
  Callback<DusimeController>             cb1;

  /** Callback object 2. */
  Callback<DusimeController>             cb2;

  /** Callback object 3. */
  Callback<DusimeController>             cb3;

  /** Callback object 4. */
  Callback<DusimeController>             cb4;


  /** Activity, reading the confirmations. */
  ActivityCallback                       read_confirms;

  /** Activity, sending queries. */
  ActivityCallback                       send_queries;

  /** Activity, responding to application state change requests */
  ActivityCallback                       respond_app;

  /** Activity, send a snapshot send request. */
  ActivityCallback                       collect_snap;

  /** Clock for a-periodic stuff. */
  AperiodicAlarm                         waker;

  /** This class is a singleton. */
  static DusimeController*               singleton;

public:
  /** Constructor, called from scheme as a standard module. */
  DusimeController(Entity* e, const char* part,
                   const PrioritySpec& ps);

  /** Destructor. */
  ~DusimeController();

  /** Table with tuneable parameters. */
  static const ParameterTable* getParameterTable();

  /** Adjust minimum interval */
  bool setMinInterval(const int& i);

  /** name of the class, as known to scheme. */
  static const char* const               classname;

  /** Get a pointer to the singleton. */
  static inline DusimeController* single() { return singleton; }

  /** Standard startmodule command; this is after all a normal
      module. */
  void startModule(const TimeSpec& time);

  /** Standard stopmodule command; this is after all a normal
      module. */
  void stopModule(const TimeSpec& time);

  /** Tell that I am prepared to run. */
  bool isPrepared();

  /** Control the model, used to pass on commands from the interface.
      \param req_state    The target state for the simulation.
      \returns            True if the state change can be initialised,
                          false if not possible. */
  bool controlModel(const SimulationState& req_state,
                    TimeTickType req_time = SimTime::getTimeTick());

  /** Determine whether the model is frozen, and initial condition etc.
      adjustments may be made */
  bool modelHolding() const;

  /** call from Replay supporting modules that replay is enabled */
  virtual void setReplayPrepared(bool replay_prepared);

  /** Can the controller be sent to advance */
  inline bool allowCommandAdvance() const { return !block_advance; }

protected:
  /** Initiate a snapshot */
  void takeSnapshot();

private:
  /** Handle confirmation messages. */
  void processConfirm(const TimeSpec& time);

  /** Send out queries. */
  void sendQuery(const TimeSpec& time);

  /** Change the state on request of the application. */
  void applicationStateChange(const TimeSpec& time);

  /** Follow up on a snapshot take request. */
  void snapCollect(const TimeSpec& time);

protected:
  /** Have any GUIs refresh their view of the entities in DUECA */
  virtual void refreshEntitiesView();

  /** Have any GUI, refresh button state */
  virtual void refreshButtonState(const SimulationState& btn_state);
};

DUECA_NS_END
#endif
