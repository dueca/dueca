/* ------------------------------------------------------------------   */
/*      item            : MMSLabStick.hxx
        made by         : Rene' van Paassen
        date            : 001027
        category        : header file
        description     :
        changes         : 001027 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2

        WARNING         : This example has not been updated to current
                                 DUECA interfacing specifications
*/

#ifndef MMSLabStick_hh
#define MMSLabStick_hh

#ifdef MMSLabStick_cc
#endif

#include <SimulationModule.hxx>
#include <StreamAccessToken.hxx>
#include <PrimaryControls.hxx>
class PassiveManipulator;
class ParameterTable;

class MMSLabStick: public SimulationModule
{
  // definition of the stick roll properties
  double roll_mass, roll_damping,
    roll_spring_left, roll_spring_middle, roll_spring_right,
    roll_breakout, roll_friction, roll_stiction,
    roll_x_min, roll_x_trans_lower, roll_x_neutral,
    roll_x_trans_upper, roll_x_max,
    roll_output_k_force, roll_output_k_pos, roll_output_offset;

  // definition of the stick pitch properties
  double pitch_mass, pitch_damping,
    pitch_spring_aft, pitch_spring_middle, pitch_spring_forward,
    pitch_breakout, pitch_friction, pitch_stiction,
    pitch_x_min, pitch_x_trans_lower, pitch_x_neutral,
    pitch_x_trans_upper, pitch_x_max,
    pitch_output_k_force, pitch_output_k_pos, pitch_output_offset;

  // definition of generic properties
  double dt, stick_arm;
  bool kickstart_io;

  // two systems, roll and pitch channel
  PassiveManipulator *roll, *pitch;

  // scale factors for the input
  static const double      i_scale[6], i_offset[6];
  static const double      o_scale[3], o_offset[3];

  // input force check
  int                      force_check_count;
  double                   fcheck[2];

  // output channels for the controls, and inco output channel
  StreamChannelWriteToken<PrimaryControls>         controls;
  EventChannelWriteToken<PrimaryControls>          i_controls;

  // activity
  Callback<MMSLabStick>                            cb1, cb2;
  ActivityCallback                                 measure_control;
  ActivityCallback                                 calculate_inco;

public:
  const static ParameterTable*                     getMyParameterTable();
  static const char* const                         classname;
  static const IncoTable*                          getMyIncoTable();

  /// constructor, should be called from scheme, via the ModuleCreator
  MMSLabStick(Entity* e, const char* part, const PrioritySpec& ts);

  /// destructor
  ~MMSLabStick();

  /// further specification, update rate
  bool setTiming(const TimeSpec& t);

public:
  /// start responsiveness to input data
  void startModule(const TimeSpec &time);

  /// stop responsiveness to input data
  void stopModule(const TimeSpec &time);

  /// check sanity of all data and report ok or not
  bool isPrepared();

private:

  /// the method that implements the main calculation
  void doStep(const TimeSpec& ts);

  /** performs an inco calculation. Should NOT use current state
      uses event channels parallel to the stream data channels,
      calculates based on the event channel input, the steady state
      output. */
  void doIncoCalculation(const TimeSpec& ts);

  /// snapshot capability, sending
  Snapshot* sendSnapshot(const TimeSpec& t, bool inco);

  /// restoring state from snapshot
  void loadSnapshot(const Snapshot* );
};




#endif
