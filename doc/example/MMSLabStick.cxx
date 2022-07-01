/* ------------------------------------------------------------------   */
/*      item            : MMSLabStick.cxx
        made by         : Rene' van Paassen
        date            : 001027
        category        : body file
        description     :
        changes         : 001027 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2

        WARNING         : This example has not been updated to current
                                 DUECA interfacing specifications
*/


#define MMSLabStick_cc
#include "MMSLabStick.hxx"
#include "PassiveManipulator.hxx"
#include <Ticker.hxx>
#include <IncoTable.hxx>
#include <ParameterTable.hxx>
extern "C" {
#include "natmio.h"
#include "dda-06.h"
}
#include "config.h"

#define DO_INSTANTIATE
#include <TypeCreator.hxx>
#include <VarProbe.hxx>
#include <MemberCall.hxx>
#include <Callback.hxx>
#include <StreamAccessToken.hxx>
#include <EventAccessToken.hxx>
#include <Event.hxx>
//#define SKIP_STICK_IO

const char* const MMSLabStick::classname = "mmslab-stick";

// input scale factors
// for roll and pitch, assuming that these were scaled to give 100N/10V
// for roll and 81.33N/10V for pitch
// These gains give the moment. (100N * 0.09)
// A/D scale has been set to 5V, so gains must be multiplied by 0.5
const double MMSLabStick::i_scale[6] = {-0.00458 * 0.5,   // roll  [Nm/-]
                                        -8.784e-4,  // roll  [rad/-]
                                        -0.00366 * 0.5,   // pitch [Nm/-]
                                        6.608e-4,   // pitch [rad/-]
                                        0.0, 0.0};
const double MMSLabStick::i_offset[6] =
{41.0,
    -2.958e-3, //roll  [rad]
    48.0,
    4.503e-4,  //pitch [rad]
    0.0,
    0.0};
//const double MMSLabStick::o_scale[3] = {-4228.0*3.3,    // roll  [-/rad]
//                                         5402.0*3.3,    // pitch [-/rad]
//                                            0.0};   // yaw

const double DEG_RAD = 3.1415927/180.0;

// this gain has been guessed. Adri adjusted input gain for stick with
// factor 1/3.3 Above (commented out) calculation gives unrealistic values,
// with outputs way over 2000 for max roll/pitch position (30, 20 deg resp,
// converted to radians. A quick test revealed a digital output value of
// 1900 is close to max excursion for both roll and pitch. this has been
// implemented below.
const double MMSLabStick::o_scale[3] = {-1900.0/(30.0*DEG_RAD),    // roll  [-/rad]
                                          1900.0/(20.0*DEG_RAD),    // pitch [-/rad]
                                            0.0};   // yaw
const double MMSLabStick::o_offset[3] = {0.0, 0.0, 0.0};

const IncoTable* MMSLabStick::getMyIncoTable()
{
  static const IncoTable my_inco_table[] =
    {
        {(new IncoVariable("stick pitch trim", -1.0, 1.0))
        ->forMode(FlightPath, Control),
          new VarProbe<MMSLabStick,double>
            (&MMSLabStick::pitch_output_offset) },

        { NULL, NULL}
    };
  return my_inco_table;
}

MMSLabStick::MMSLabStick(Entity* e,
                         const char* part,
                         const PrioritySpec& ps) :
  SimulationModule(e, classname, part, getMyIncoTable()),

  // default stick roll properties
  roll_mass(2.0),
  roll_damping(30.0),
  roll_spring_left(400.0),
  roll_spring_middle(400.0),
  roll_spring_right(400.0),
  roll_breakout(0.0),
  roll_friction(0.0),
  roll_stiction(0.0),
  roll_x_min(-25.0),
  roll_x_trans_lower(-25.0),
  roll_x_neutral(0.0),
  roll_x_trans_upper(25.0),
  roll_x_max(25.0),
  roll_output_k_force(0.0),
  roll_output_k_pos(1.0),
  roll_output_offset(0.0),

  // default stick pitch properties
  pitch_mass(2.0),
  pitch_damping(30.0),
  pitch_spring_aft(400.0),
  pitch_spring_middle(400.0),
  pitch_spring_forward(400.0),
  pitch_breakout(0.0),
  pitch_friction(0.0),
  pitch_stiction(0.0),
  pitch_x_min(-19.0),
  pitch_x_trans_lower(-19.0),
  pitch_x_neutral(0.0),
  pitch_x_trans_upper(19.0),
  pitch_x_max(19.0),
  pitch_output_k_force(0.0),
  pitch_output_k_pos(1.0),
  pitch_output_offset(0.0),

  // generic simulation properties
  dt(Ticker::single()->getDT()),
  stick_arm(0.09),
  kickstart_io(false),

  // the actual systems
  roll(NULL), pitch(NULL),

  // output channels
  controls(getId(), NameSet(getEntity(), "PrimaryControls", part)),

  // NOTE: inco output with part = "" is a hack, depending on a rate
  // reductor that reduces/interfaces the controls channel, and direct
  // communication of the inco channel.
  i_controls(getId(), NameSet(getEntity(), "inco_PrimaryControls", "")),

  // callback functions and activities
  cb1(this, &MMSLabStick::doStep),
  cb2(this, &MMSLabStick::doIncoCalculation),
  measure_control(getId(), "control input", &cb1, ps),
  calculate_inco(getId(), "inco", &cb2, PrioritySpec(0, 0))
{
  measure_control.setTrigger(*Ticker::single());
  calculate_inco.setTrigger(t_inco_input);
  calculate_inco.switchOn(TimeSpec(0,0));

  // initialise board
  int status = Init_AT_MIO();
  if (status != 0) {
    cerr << "Cannot init AT_MIO" << endl;
  }

  // so set-up with my preferences
  Setup_AT_MIO();

  // zero the dda outputs
  for (int ii = 6; ii--; )
    outputVolt(ii, 0.0);
}

MMSLabStick::~MMSLabStick()
{
  //
}

bool MMSLabStick::isPrepared()
{
  // checks
  if (roll_mass < 0.5 || roll_damping < 0.0 ||
      roll_spring_left < 0.0 || roll_spring_middle < 0.0 ||
      roll_spring_right < 0.0 || roll_x_min > roll_x_neutral ||
      roll_x_neutral > roll_x_max ||
      roll_x_trans_upper < roll_x_trans_lower) {
    cerr << getId() << "Error in roll parameters" << endl;
    return false;
  }
  if (pitch_mass < 0.5 || pitch_damping < 0.0 ||
      pitch_spring_aft < 0.0 || pitch_spring_middle < 0.0 ||
      pitch_spring_forward < 0.0 || pitch_x_min > pitch_x_neutral ||
      pitch_x_neutral > pitch_x_max ||
      pitch_x_trans_upper < pitch_x_trans_lower) {
    cerr << getId() << "Error in pitch parameters" << endl;
    return false;
  }
  if (!controls.isValid()) {
    return false;
  }

  // just to make sure
  if (pitch != NULL) return true;

  // make a pitch stick system
  if (pitch != NULL) delete(pitch);
  // note: forward is positive
  pitch = new PassiveManipulator
    (stick_arm, pitch_mass, pitch_damping,
     pitch_spring_aft, pitch_spring_middle, pitch_spring_forward,
     pitch_breakout, pitch_friction, pitch_stiction,
     pitch_x_min, pitch_x_trans_lower, pitch_x_neutral,
     pitch_x_trans_upper, pitch_x_max,
     pitch_output_k_force, pitch_output_k_pos, pitch_output_offset, dt);

  // note: left is positive, bigger
  if (roll != NULL) delete(roll);
  roll = new PassiveManipulator
    (stick_arm, roll_mass, roll_damping,
     roll_spring_right, roll_spring_middle, roll_spring_left,
     roll_breakout, roll_friction, roll_stiction,
     roll_x_min, roll_x_trans_lower, roll_x_neutral,
     roll_x_trans_upper, roll_x_max,
     roll_output_k_force, roll_output_k_pos, roll_output_offset, dt);

  // prepared
  return true;
}

bool MMSLabStick::setTiming(const TimeSpec& ts)
{
  measure_control.setTimeSpec(ts);
  dt = Ticker::single()->getDT()*ts.getValiditySpan();
  return true;
}

void MMSLabStick::startModule(const TimeSpec &time)
{
  measure_control.switchOn(time);

#ifdef STICK_ASYNC_IO
  kickstart_io = true;
#endif
}

void MMSLabStick::stopModule(const TimeSpec &time)
{
  measure_control.switchOff(time);
}

void MMSLabStick::doIncoCalculation(const TimeSpec& ts)
{
  // there is only one mode
  assert(getIncoMode() == FlightPath);

  // only thing to do is to send the inco output
  i_controls.sendEvent
    (new PrimaryControls(0.0, pitch_output_offset, 0.0, 0.0, 0.0, 0.0), ts);
}

void MMSLabStick::doStep(const TimeSpec& ts)
{
  double roll_moment, roll_pos, pitch_moment, pitch_pos;

  // measure the input, start a single aquisition and wait
#ifdef STICK_ASYNC_IO
  if (kickstart_io) {
    kickstart_io = false;
    Clear_FIFO();
    AI_Start_The_Acquisition();
  }
#elif !defined(SKIP_STICK_IO)
  AI_Start_The_Acquisition();
#endif

#ifdef SKIP_STICK_IO
  roll_moment = 0.0;
  roll_pos = 0.0;
  pitch_moment =0.0;
  pitch_pos = 0.0;
#else

  Wait_AT_MIO();
  int16_t tmp = Board_Read(ADC_FIFO_Data_Register) & 0xffff;
  roll_moment = (tmp +i_offset[0]) * i_scale[0];

  Wait_AT_MIO();
  tmp = Board_Read(ADC_FIFO_Data_Register) & 0xffff;
  roll_pos = tmp * i_scale[1] + i_offset[1];

  Wait_AT_MIO();
  tmp = Board_Read(ADC_FIFO_Data_Register) & 0xffff;
  pitch_moment = (tmp +i_offset[2]) * i_scale[2];

  Wait_AT_MIO();
  tmp = Board_Read(ADC_FIFO_Data_Register) & 0xffff;
  pitch_pos = tmp * i_scale[3] + i_offset[3];
#endif

#ifdef STICK_ASYNC_IO
  // starts the acquisition for the NEXT cycle. In this way we don't wait
  // for the conversion
  AI_Start_The_Acquisition();
#endif

  switch (getAndCheckState(ts)) {

  case HoldCurrent:

    // simple trick. use 0 input on the system. Without much friction,
    // it will more or less return to the middle position
    pitch->doStep(0.0);
    roll->doStep(0.0);
    break;

  case Advance:

    // do the simulation
    pitch->doStep(pitch_moment); //*stick_arm);
    roll->doStep(roll_moment); //*stick_arm);
  }

  // output back to the stick
  int16_t out0 =
    int16_t(rint(pitch->getState()[0] * o_scale[0] + o_offset[0]));
  if (out0 > 2047) out0 = 2047;
  if (out0 < -2048) out0 = -2048;
  int16_t out1 =
    int16_t(rint(roll->getState()[0] * o_scale[1] + o_offset[1]));
  if (out1 > 2047) out1 = 2047;
  if (out1 < -2048) out1 = -2048;
  Board_Write(AO_DAC_0_Data_Register, out0);
  Board_Write(AO_DAC_1_Data_Register, out1);

  // send to the rest of the simulation
  PrimaryControls *c;
  controls.getAccess(c, ts);
  c->stick_roll  = roll->getOutput()[0];
  c->stick_pitch = pitch->getOutput()[0];
  c->roll_moment = roll_moment;
  c->pitch_moment= pitch_moment;
  c->roll_rate   = roll_pos;
  c->pitch_rate  = pitch_pos;
  controls.releaseAccess(c);
}

Snapshot* MMSLabStick::sendSnapshot(const TimeSpec& ts, bool inco)
{
  // the snapshot contains the time, and the initial pitch input

  AmorphStore s(new char[16], 16);

  if (inco) {
    packData(s, double(0.0));   // roll pos
    packData(s, double(0.0));   // roll vel
    packData(s, double(0.0));   // pitch pos
    packData(s, double(0.0));   // pitch vel
    //  packData(s, i_pitch_output_offset);  // trim
  }
  else {
    packData(s, double(0.0));   // roll pos
    packData(s, double(0.0));   // roll vel
    packData(s, double(0.0));   // pitch pos
    packData(s, double(0.0));   // pitch vel
    // packData(s, pitch_output_offset);  // trim
  }

  return new Snapshot(s.getToData(), s.getSize(), getNameSet());
}

void MMSLabStick::loadSnapshot(const Snapshot* d)
{
  AmorphReStore s(d->data, d->data_size);
  //  unPackData(s, t);
  //  unPackData(s, u0_y);
}

const ParameterTable* MMSLabStick::getMyParameterTable()
{
  static const ParameterTable table[] = {

    { "set-timing", new MemberCall<MMSLabStick,TimeSpec>
      (&MMSLabStick::setTiming)},

    { "roll-mass", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_mass) },

    { "roll-damping", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_damping) },

    { "roll-spring-left", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_spring_left) },

    { "roll-spring-middle", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_spring_middle) },

    { "roll-spring-right", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_spring_right) },

    { "roll-breakout", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_breakout) },

    { "roll-friction", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_friction) },

    { "roll-stiction", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_stiction) },

    { "roll-x-min", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_x_min) },

    { "roll-x-trans-lower", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_x_trans_lower) },

    { "roll-x-neutral", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_x_neutral) },

    { "roll-x-trans-upper", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_x_trans_upper) },

    { "roll-x-max", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_x_max) },

    { "roll-output-k-force", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_output_k_force) },

    { "roll-output-k-pos", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_output_k_pos) },

    { "roll-output-offset", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::roll_output_offset) },

    { "pitch-mass", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_mass) },

    { "pitch-damping", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_damping) },

    { "pitch-spring-aft", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_spring_aft) },

    { "pitch-spring-middle", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_spring_middle) },

    { "pitch-spring-forward", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_spring_forward) },

    { "pitch-breakout", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_breakout) },

    { "pitch-friction", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_friction) },

    { "pitch-stiction", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_stiction) },

    { "pitch-x-min", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_x_min) },

    { "pitch-x-trans-lower", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_x_trans_lower) },

    { "pitch-x-neutral", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_x_neutral) },

    { "pitch-x-trans-upper", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_x_trans_upper) },

    { "pitch-x-max", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_x_max) },

    { "pitch-output-k-force", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_output_k_force) },

    { "pitch-output-k-pos", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_output_k_pos) },

    { "pitch-output-offset", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::pitch_output_offset) },

    { "stick-arm", new VarProbe<MMSLabStick,double>
      (&MMSLabStick::stick_arm) },

    {NULL, NULL}};
  return table;
}

static TypeCreator<MMSLabStick> a(MMSLabStick::getMyParameterTable());



