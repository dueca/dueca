/* ------------------------------------------------------------------   */
/*      item            : RTWModule.hxx
        made by         : Joost Ellerbroek
        date            : 080208
        category        : header file
        description     :
        changes         : 080208 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef RTWModule_hxx
#define RTWModule_hxx

#include <SimulationModule.hxx>
DUECA_NS_START

//forward declarations
struct XmlSnapshot;

/** A base class from which users can derive Real-Time Workshop modules.

    The RTWModule base class implements the basic communication
    for a DUSIME module. By deriving from this class, using its
    methods to dermine the simulation state, and re-implementing
    applicable virtual methods, a fully DUSIME-aware class can be
    made, and objects of this class have coordinated start-stop
    abilities, the ability to calculate initial conditions and the
    ability for saving and restoring simulation state. A RTWModule
    can also store and restore RTW model state and parameters,
    from and to an xml description. This enables translation of
    states and parametersets between your DUECA simulation and
    the MATLAB workspace. */
class RTWModule : public SimulationModule
{
  private:
    /** State of the snapshot taking. */
    SnapshotState                            xml_snap_state;

    /** Time at which a snapshot should be taken. This is "sent around"
    in advance, so that coordinated, time-consistent snapshots can
    be made. */
    TimeTickType                            future_xml_snap_time;

    /** callback objects. */
    Callback<RTWModule>                     cb1, cb2;

    /** An access token for reading the incoming xml snapshots. */
    ChannelReadToken                        r_xml_snap;

    /** An access token for sending xml snapshots for the current state. */
    ChannelWriteToken                       w_xml_snap;

    /** An actvity to react to entity commands. */
    ActivityCallback                        xml_snap_recv;

  private:
    /** copy constructor, assure no-one uses this inadvertently. */
    RTWModule(const RTWModule&);

    /** Callback for processing incoming xml snapshots and calls for new snapshots */
    void receiveXmlSnapshot(const TimeSpec& ts);

    /** Callback for activating xml snapshot activity upon EventChannel valid. */
    void initXmlChannels(const TimeSpec& ts);

  protected:
    /** Constructor.
    \param e       Pointer to my entity
    \param m_class String with name of the module class
    \param part    String with part name
    \param table   Pointer to the table with initial condition
    calculation definitions. If this class does not
    take part in calculation of initial conditions
    (other than possibly sendin on data), this
    pointer may be NULL.
    \param state_size Size of the state, as sent in a snapshot. */
    RTWModule(Entity* e, const char* m_class, const char* part,
                     const IncoTable* table = NULL, int state_size = 0);

    /** Destructor. */
    virtual ~RTWModule();

  public:
    /** Returns true if an xml snapshot has to be taken in this cycle.  If
    this returns true, you should keep a copy of the continuous and/or
    discrete states of the RTW model, at a location of your discretion.
    This copy will later be sent with the sendXmlSnapshot call. */
    bool XmlSnapshotNow(const TimeSpec& ts);

    /** If xml snapshots are generated, this has to be implemented by a
    descendant.

    The default implementation for this function is included when generating
    a new RTW module with the 'new-module' command, and subsequently xml
    functionality is selected. Currently this is functional for RTW version
    5.0 and up.
    \param ts    For your reference, the time specification of the
    snapshot command
    \param snap  The XmlSnapshot that has to be filled. */
    virtual void fillXmlSnapshot(const TimeSpec& ts, XmlSnapshot& snap);

    /** For restoring state, model parameters, and initial inputs from
    a previously generated xml file.

    This should take the snapshot data, unpack it (so keep aligned
    with the sendXmlSnapshot routine), and use this to replace the
    current state, parameters, and inputs. Note that the model will
    not be running at this time, so -- if you don't touch your state
    in HoldCurrent, as you should not -- this can run in parallel to
    the simulation without locking. Default implementation for this
    function is included in the rtw model template. */
    virtual void loadXmlSnapshot(const TimeSpec &ts, const XmlSnapshot &snap);
};
DUECA_NS_END
#endif
