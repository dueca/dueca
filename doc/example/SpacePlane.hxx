/* ------------------------------------------------------------------   */
/*      item            : SpacePlane.hxx
        made by         : Rene' van Paassen
        date            : 001003
        category        : header file
        description     :
        changes         : 001003 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SpacePlane_hh
#define SpacePlane_hh

#ifdef SpacePlane_cc
#endif

// include the dusime header
#include <dusime.h>

// include headers for the objects that are sent over the channels
#include "PrimaryControls.hxx"
#include "SpacePlaneY.hxx"
#include "SpacePlaneState.hxx"

// include headers for functions/classes you need in the module

// This module is based on code generated with real-time workshop from
// a SimuLink model. Therefore we include the simstruc and the
// simulink model, called "complete"
extern "C" {
#define RT
#define RTW_GENERATED_S_FUNCTION
#include "simstruc.h"
#include "complete.h"
}
/** e01 */

/*  Provides a class that implements Crew Rescue Vehicle dynamics.

    Implements the dynamics of a Crew Rescue Vehicle with low L/D
    characteristics. The dynamic model is created in SimuLink and
    exported with Real-Time Workshop. The module encapsulates the C
    source with the Simulink Model. Inputs are the control vector,
    outputs contain most of the state and instrument related data. */
class SpacePlane: public SimulationModule
{
  /** e02 */
private: // Simulation data

  /* Simulink system variable pointers, normal, one for inco. These
     also contain the state data. */
  SimStruct *S; /* *i_S */

  // prediction time
  int                                       t_predict;

  // stop height
  double                                    z_stop;

private: // trim calculation data
  // trim calculation is not implemented yet.

private: // snapshot data
  // snapshot state
  real_T                                    s_x[NSTATES];

private: // channel access
  // input and output channels for the normal running
  StreamChannelReadToken<PrimaryControls>           controls;
  StreamChannelWriteToken<SpacePlaneY>              output;
  StreamChannelWriteToken<SpacePlaneState>          state;

private: // activity allocation
  //  Callback object for simulation calculation
  Callback<SpacePlane>  cb1;

  //  Activity for simulation calculation
  ActivityCallback      do_calc;

public:  // class name and trim/parameter tables
  //  Name of the module.
  static const char* const           classname;

  //  Return the initial condition table
  static const IncoTable*            getMyIncoTable();

  //  Return the parameter table
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /*  Constructor. Is normally called from scheme/the creation script. */
  SpacePlane(Entity* e, const char* part, const PrioritySpec& ts);

  /*  Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /*  Destructor. */
  ~SpacePlane();
  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** s01 */
  /*  Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /*  Request check on the timing. */
  bool checkTiming(const vector<int>& i);

public: // member functions for cooperation with DUECA
  //  indicate that everything is ready
  bool isPrepared();

  //  start responsiveness to input data
  void startModule(const TimeSpec &time);

  //  stop responsiveness to input data
  void stopModule(const TimeSpec &time);

  /** s03 */
public: // the member functions that are called for activities
  //  the method that implements the main calculation
  void doCalculation(const TimeSpec& ts);

public: // member functions for cooperation with DUSIME
  //  For the Snapshot capability, fill the snapshot "snap" with the
  //  data saved at a point in your simulation (if from_trim is false)
  //  or with the state data calculated in the trim calculation (if
  //  from_trim is true)
  void fillSnapshot(const TimeSpec& ts,
                    Snapshot& snap, bool from_trim);

  //  Restoring the state of the simulation from a snapshot
  void loadSnapshot(const TimeSpec& t, const Snapshot& snap);

  /*  Perform a trim calculation. Should NOT use current state
      uses event channels parallel to the stream data channels,
      calculates, based on the event channel input, the steady state
      output. */
  // this has not been implemented in DUECA, so we comment it out
  // void trimCalculation(const TimeSpec& ts, const IncoMode& mode);
};

#endif




