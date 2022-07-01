/* ------------------------------------------------------------------   */
/*      item            : Module.hh
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

#ifndef Module_hh
#define Module_hh

#include <NamedObject.hxx>
#include <ModuleState.hxx>
#include <dueca_ns.h>
#include <boost/scoped_ptr.hpp>

DUECA_NS_START
class Entity;
class TimeSpec;
class CriticalActivity;
class ModuleCreator;

/** Base class for user-created self contained objects in DUECA.

    A Module is a class that basically obeys to some simple DUECA
    rules (it listens to its Entity), and which can be created from
    Scheme or Python, one of the scripting languages used in
    DUECA. Note that for simulations, often the SimulationModule class
    -- derived from Module -- is used as a base class for application
    code.

    A Module can register one or more activities, and supply the
    conditions on which the activities must be invoked. In the
    activities the module's work is done; simulation, control, data
    processing, etc.

    For certain situations, it is good to know from which thread a
    module's method is called.

    <ul>

    <li> Modules are created from the main thread of DUECA. This
    thread handles script (Scheme or Python) interaction, thus the
    constructor and any methods listed in the ParameterTable are
    called from this thread. There is access to your data, but not
    much more at this phase.

    <li> After creation of all initial modules in DUECA, applicable
    graphics code is initialized. Typically a windowing toolkit like
    GTK2, gtk3, glut may be started. After that, the
    Module::complete() method is called. At this stage, windows using
    the graphics code may be opened.

    <li> Module control is done from the "admin" or priority 0
    thread. This thread commonly also does the gtk interface code, if
    a gtk interface is present. So the startModule, stopModule,
    initialStartModule and finalStopModule methods are called from
    this thread.

    <li> It is common for a module to have one or sometimes more
    activities; the priority for these activities can be freely
    chosen, and is often configurable. So you should take care when
    sharing data between these activities and the module control
    callbacks.

    </ul>

    However, module control callbacks are called with a time
    specification that gives you some lead. In general it is safe to
    access the module's data, and then set the activity active.
*/
class Module : public NamedObject
{
private:

  /// Pointer to my entity (at least its local representation).
  const Entity* my_entity;

protected:
  /** Flag to remember whether we are stopped due to some error with
      hardware device manipulation. */
  ModuleState state;

private:

  /// Copying is not a good thing here.
  Module(const Module& m);

  /// Neither is assignment
  Module& operator = (const Module& o);

protected:
  /// Constructor.
  Module(const Entity *e, const char* m_class, const char* part);

public:
  /// Destructor.
  virtual ~Module();

  /** Inform the module that all parameters have now been passed.

      Override this complete method if you have non-trivial
      work to do before starting to run. Examples are reading large
      databases, opening graphic (especially accelerated GL) windows,
      etc. Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!)

      @returns    True if all initialisation successful */
  virtual bool complete();

  /** To check whether the module is ready for work.

      In this phase, typically check that channel tokens needed for
      communication are valid. May be called repeatedly.

      @returns    True if ready for running */
  virtual bool isPrepared() = 0;

  /** To check whether the module is prepared to be prepared.

      This method may be overloaded (e.g. by HardwareModule derived
      classes) to indicate that the hardware may be started. Typically
      isInitialPrepared checks all DUECA-independent conditions for
      running (hardware, datafiles, etc.) and isPrepared checks DUECA
      communication.

      @returns    True if ready for next phase */
  virtual bool isInitialPrepared();

  /// The object type within DUECA.
  ObjectType getObjectType() const { return O_Module; }

  /** Start the module's activity (activities)

      This is called only after isPrepared has returned true. This is
      called with some lead time, typically start activities at the
      time passed in. Associated with the "work" phase of the control
      panel.

      @param time    Start time for any activities
  */
  virtual void startModule(const TimeSpec &time) = 0;

  /** Stop the module's activity (activities)

      This is called with some lead time, back to "safe" phase on the
      control panel.

      @param time    Stop time for any activities
  */
  virtual void stopModule(const TimeSpec &time) = 0;

  /** Initial start opportunity

      This is called after isInitialPrepared returned true, associated
      with the "safe" phase on the control panel. This is typically
      used by hardware modules that need to actively control a
      hardware device; use a CriticalActivity to control the phase
      transitions between safe and work.

      @param time    Start time for any activities
  */
  virtual void initialStartModule(const TimeSpec &time);

  /** Final stop command.

      Transition from "safe" to "off"

      @param time    Stop time for any activities */
  virtual void finalStopModule(const TimeSpec &time);

  /** Return a pointer to the entity to which this module belongs. */
  inline const Entity* getMyEntity() {return my_entity;}

  /** Return the module state. */
  const ModuleState& getState();

protected:
  friend class CriticalActivity;
  /// Put a brake on this module's activities
  virtual void setSafetyStop();

private:
  friend class Entity;
  /** Command the module state. */
  void setState(const ModuleState& state, const TimeSpec &ts);
};


DUECA_NS_END
#endif
