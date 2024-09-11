/* ------------------------------------------------------------------   */
/*      item            : @Module@.hxx
        generated by    : @author@
        from template   : Simulink88Template.hxx (2022.06)
        date            : @date@
        category        : header file
        description     : Encapsulation of the Simulink/rtw model
                          @rtwmodel@
        changes         : @date@ first version
        language        : C++
        copyright       : (c)
*/

#ifndef @Module@_hxx
#define @Module@_hxx

// include the dusime header
#include <dusime.h>
USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include headers for functions/classes you need in the module
extern "C" {
#include "@rtwmodel@.h"
}
// the @rtwmodel@.h file defines the assert macro to be null. This is
// not wat we want, we need assert to check the dimensions of the
// model against those of the DUECA simulation. Therefore:
#undef assert

// define the number of states here.
#define NCSTATES (sizeof(X_@rtwmodel@_T))
#define NDSTATES (sizeof(DW_@rtwmodel@_T))

// calculate total number of states
#if defined(NCSTATES) && defined(NDSTATES)
# define NSTATES (NCSTATES + NDSTATES)
#elif defined(NCSTATES)
# define NSTATES NCSTATES
#elif defined(NDSTATES)
# define NSTATES NDSTATES
#else
# error "Neither NCSTATES nor NDSTATES defined"
#endif



/** A simulation module.

    The instructions to create an module of this class from the start
    script are:

    \verbinclude @smodule@.scm
 */
class @Module@:
  public SimulationModule
{
  /** self-define the module type, to ease writing the parameter table */
  typedef @Module@ _ThisModule_;

private: // simulation data
  /** structure to contain all relevant information for an RTW model */
  struct modelSet {
    /** RTW model */
    RT_MODEL_@rtwmodel@_T *M;
    /** Inputs to the model */
    ExtU_@rtwmodel@_T      U;
    /** Outputs of the model */
    ExtY_@rtwmodel@_T      Y;
  };

  /** pointer to the real-time workshop model. */
  modelSet* S;

  /** pointer to a new, re-initialized  model, waiting */
  modelSet *Snew;

  /** pointer to a used, old model. */
  modelSet *Sold;

  /** The time step we are going to use. */
  double    dt;

private: // trim calculation data
  /** pointer to a second copy of this real-time workshop model, for
      trim condition calculation. */
  modelSet *i_S;

private: // snapshot data
  /** Copy of the model state for snapshot sending. */
  //struct tag_RTM_@rtwmodel@_T;

private: // channel access
  // declare access tokens for all the channels you read and write
  // examples:
  // ChannelReadToken    r_mytoken;
  // ChannelWriteToken   w_mytoken;

private: // activity allocation
  /** Callback object for simulation calculation. */
  Callback<@Module@>  cb1;

  /** Activity for simulation calculation. */
  ActivityCallback      do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the initial condition table. */
  static const IncoTable*            getMyIncoTable();

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from the creation script. */
  @Module@(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengthy initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~@Module@();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int>& i);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec& ts);

public: // member functions for cooperation with DUSIME
  /** For the Snapshot capability, fill the snapshot "snap" with the
      data saved at a point in your simulation (if from_trim is false)
      or with the state data calculated in the trim calculation (if
      from_trim is true). */
  void fillSnapshot(const TimeSpec& ts,
                    Snapshot& snap, bool from_trim);

  /** Restoring the state of the simulation from a snapshot. */
  void loadSnapshot(const TimeSpec& t, const Snapshot& snap);

  /** Perform a trim calculation. Should NOT use current state
      uses event channels parallel to the stream data channels,
      calculates, based on the event channel input, the steady state
      output. */
  void trimCalculation(const TimeSpec& ts, const IncoMode& mode);

private:
  /** Create and initialise a model. */
  modelSet *createAndInitialiseModel();

  /** Destroy it again. */
  void destroyModel(modelSet *S);
};

#endif
