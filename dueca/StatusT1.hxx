/* ------------------------------------------------------------------   */
/*      item            : StatusT1.hxx
        made by         : Rene van Paassen
        date            : 010824
        category        : header file
        description     :
        changes         : 010824 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef StatusT1_hxx
#define StatusT1_hxx

#ifdef StatusT1_cxx
#endif

#include <ModuleState.hxx>
#include <SimulationState.hxx>
#include <SimTime.hxx>
#include <dueca_ns.h>


DUECA_NS_START

class DuecaView;

/** This object contains a status summary of DUECA modules. These
    modules may be pure DUECA modules, so they have no DUSIME status,
    or they may be DUSIME modules and have a DUSIME status, in that
    case the flag "have_sstate" will be true. */
class StatusT1
{
  /** State of a dueca module. */
  ModuleState     mstate;

  /** State of a DUSIME module, either hardware or simulation. */
  SimulationState sstate;

  /** Time for the actualisation of the module state. */
  TimeTickType    m_actualised_at;

  /** Time for the actualisation of the simulation state. */
  TimeTickType    s_actualised_at;

public:

  /** Define the type of the visualization object. */
  typedef DuecaView viewer;

public:
  /** Constructor. */
  StatusT1();

  /** Destructor. */
  ~StatusT1();

  /** Copy constructor. */
  StatusT1(const StatusT1& o);

  /** If true, this object has a simulation state. */
  inline bool haveSState() const
  { return sstate != SimulationState::Neutral;}

  /** Insert the module state. */
  inline void setModuleState(const ModuleState& mstate,
                             const TimeTickType& tick)
  {this->mstate = mstate; m_actualised_at = tick; }

  /** Insert a new simulation (DUSIME) state. */
  inline void setSimulationState(const SimulationState& sstate,
                                 const TimeTickType &tick)
  {this->sstate = sstate; s_actualised_at = tick; }

  /** Return the module state. */
  inline ModuleState& getModuleState() {return mstate;}

  /** Return the current DUSIME state. */
  inline SimulationState& getSimulationState() {return sstate;}

  /** Return the module state, const version. */
  inline const ModuleState& getModuleState() const {return mstate;}

  /** Return the time of the module state. */
  inline const TimeTickType& getModuleStateTime() const
  {return m_actualised_at;}

  /** Return the current DUSIME state, const version. */
  inline const SimulationState& getSimulationState() const {return sstate;}

  /** Return the time of the simulation state. */
  inline const TimeTickType& getSimulationStateTime() const
  {return s_actualised_at;}

  /** Test for equality, means states must be equal and at same time. */
  bool operator == (const StatusT1 & o) const;

  /** Second test for equality, only states need to be the same. */
  bool equiv(const StatusT1 &o) const;

  /** Combine state with another status object. */
  StatusT1& operator &= (const StatusT1& o);

  /** Re-initialise to a neutral state. */
  void clear();

  /** Print to stream, debugging. */
  ostream& print (ostream& os) const;
};

DUECA_NS_END

PRINT_NS_START
inline ostream& operator << (ostream& os, const DUECA_NS::StatusT1& o)
{ return o.print(os); }
PRINT_NS_END

#endif
