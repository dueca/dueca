/* ------------------------------------------------------------------   */
/*      item            : DusimeModule.hxx
        made by         : Rene' van Paassen
        date            : 20001010
        category        : header file
        description     :
        changes         : 20001010 Rvp first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef DusimeModule_hxx
#define DusimeModule_hxx

#ifdef DusimeModule_cxx
#endif
#include <Module.hxx>
#include <SimulationState.hxx>
#include <SnapshotState.hxx>
#include "IncoMode.hxx"
#include <Callback.hxx>
#include <SimTime.hxx>
#include <list>
#include <boost/scoped_ptr.hpp>

#include <dueca_ns.h>
DUECA_NS_START
class TriggerPuller;
struct Snapshot;
struct IncoSpec;
struct IncoNotice;
struct IncoTable;
class ActivityCallback;
class ChannelWriteToken;
class ChannelReadToken;

/** A class "in-between" the dueca Module class, and
    Simulation/HardwareModule.

    There are many common tasks in the SimulationModule and
    HardwareModule classes. These common tasks, basic to the DUSimE
    capabilities are implemented in the DusimeModule class.

    Application developers should derive their modules from either a
    SimulationModule, for modules that implement a piece of simulation
    model, without doing IO, and from HardwareModule, for modules that
    may (but preferrably do not) implement a piece of simulation, and
    do IO and control of hardware.

    So: do not derive from DusimeModule directly.
*/
class DusimeModule: public Module
{
  /** \name Snapshot
      Snapshot capabilities implementation */
  //@{

private:
  /** Size of a snapshot. */
  const int                                state_size;

protected:
  /** State of the snapshot taking. */
  SnapshotState                            snap_state;

private:
  /** Time at which a snapshot should be taken. This is "sent around"
      in advance, so that coordinated, time-consistent snapshots can
      be made. */
  TimeTickType                            future_snap_time;

  /** Channel over which the snapshot is sent. (Snapshot) */
  boost::scoped_ptr<ChannelWriteToken>    t_snapshot_send;

  /** Channel over which snapshots -- with state data -- can be
      received. (Snapshot) */
  boost::scoped_ptr<ChannelReadToken>     t_snapshot_read;

  /** 1st callback object. */
  Callback<DusimeModule>                  cb1;

  /** Activity for receiving incoming state data. */
  boost::scoped_ptr<ActivityCallback>     load_snapshot;

  //@}


  /** \name Trim
      Trim condition calculation capabilities */
  //@{

private:
  /** Write access token for sending the trim capabilities
      specification. Writes IncoSpec */
  boost::scoped_ptr<ChannelWriteToken>    t_inco_spec;

  /** Read access token to the channel from which trim calculation
      commands come in. Reads IncoNotice */
  boost::scoped_ptr<ChannelReadToken>     t_inco_input;

  /** Write access token to the channel over which the results of the
      trim calculations are sent. Writes IncoNotice */
  boost::scoped_ptr<ChannelWriteToken>    t_inco_feedback;

  /** Pointer to the table with trim calculation specification. */
  const IncoTable*                        inco_table;

  /** Size of the above table. */
  int                                     inco_table_size;

  /** 2nd callback object. */
  Callback<DusimeModule>                  cb2;

  /** 3rd callback object. */
  Callback<DusimeModule>                  cb3;

  /** Activity for receiving trim commands and doing trim
      calculations. */
  boost::scoped_ptr<ActivityCallback>     do_inco_calculation;

  //@}

private:
  /** Method that is activated when a snapshot comes in. Will try to
      pass this on to the client with a virtual function call. */
  void localLoadSnapshot(const TimeSpec& t);

  /** Method that is activated when trim calculation work comes
      in. Will try to pass this on to the client with a virtual
      function call. */
  void localIncoCalculation(const TimeSpec& ts);

  /** Method that send the inco specification as soon as the channel
      is ready. */
  void sendIncoSpecification(const TimeSpec& ts);

  /** Method that allocates the room for a snapshot, has the
      descendant class fill it in, and sends it off. */
  void localSendSnapshot(const TimeSpec& ts, bool from_trim);

  friend class SimulationModule;
  friend class HardwareModule;

protected:
  /** Constructor.
      \param e       Pointer to my entity
      \param m_class String with name of the module class
      \param part    String with part name
      \param inco_table  Pointer to the table with initial condition
                     calculation definitions. If this class does not
                     take part in calculation of initial conditions
                     (other than possibly sendin on data), this
                     pointer may be NULL.
      \param state_size Size of the state, as sent in a snapshot. */
  DusimeModule(Entity* e, const char* m_class, const char* part,
               const IncoTable* inco_table, int state_size);

  /** Destructor. */
  virtual ~DusimeModule();

protected:

  /** Returns true if a snapshot has to be taken in this cycle.  If
      this returns true, you should keep a copy of the state, at a
      location of your discretion. This copy will later be sent with
      the sendSnapshot call. */
  bool snapshotNow();

  /** If snapshots are generated, this has to be implemented by a
      descendant.

      It should return a Snapshot event. It is advised to use an
      AmorphStore object to pack the state data into the event. If the
      inco flag is used, the initial condition state, instead of the
      normal model state, should be sent. If you need to make
      preparations for a following snapshot you should also do that
      here.
      \param ts    For your reference, the time specification of the
                   snapshot command
      \param snap  The Snapshot that has to be filled.
      \param from_trim If true, indicates that the snapshot has to be
                   filled from the trim calculation, instead of from
                   the normal calculation. */
  virtual void fillSnapshot(const TimeSpec& ts,
                            Snapshot& snap, bool from_trim);

  /** For restoring the state from an old snapshot.

      This should take the snapshot data, unpack it (so keep aligned
      with the sendSnapshot routine), and use this to replace the
      current state. Note that the model will not be running at this
      time, so -- if you don't touch your state in HoldCurrent, as you
      should not -- this can run in parallel to the simulation without
      locking. */
  virtual void loadSnapshot(const TimeSpec &ts, const Snapshot &snap);

  /** This can be used to specify the condition under which the trim
      calculation may take place, for example, data has to be received
      on trim calculation input channels. */
  void trimCalculationCondition(TriggerPuller& cond);

  /** This must be overridden if the module takes part in trim
      condition calculations. */
  virtual void trimCalculation(const TimeSpec& ts, const TrimMode& mode);
};
DUECA_NS_END
#endif
