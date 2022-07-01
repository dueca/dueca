/* ------------------------------------------------------------------   */
/*      item            : Evaluator.cxx
        made by         : Rene' van Paassen
        date            : 001214
        category        : body file
        description     :
        changes         : 001214 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Evaluator_cc
#include "Evaluator.hxx"
#include <strstream>
#define RT
#define RTW_GENERATED_S_FUNCTION
#include "StatesOutputs.hxx"
#include <Ticker.hxx>
extern "C" {
#include "interface.h"
#include <support.h>
}

#define DO_INSTANTIATE
#include <dusime.h>

const char* const Evaluator::classname = "evaluator";
const unsigned int Evaluator::NTUNNELS = 3;

Evaluator::Evaluator(Entity* e, const char* part, const
                       PrioritySpec& ps) :
  SimulationModule(e, classname, part, NULL),
  input(getId(), NameSet(getEntity(), "SpacePlaneY", part)),
  display_select(getId(), NameSet(getEntity(), "DisplaySelect", part)),
  is_reset(true),
  tunnel(0),
  z_stop(0.0),
  cb(this, &Evaluator::doCalculation),
  cb2(this, &Evaluator::feedBack),
  keep_up(getId(), "evaluation calculation", &cb, ps),
  feedback(getId(), "evaluation feedback", &cb2, PrioritySpec(0,0)),
  feedback_timing_set(false)
{
  // create the gtk window. This version assumes singleton
  window = create_evaluator();
  gtk_widget_show(window);

#define LOOGUP_ENTRY(A) \
  h_ ## A = GTK_ENTRY(lookup_widget(window, #A ))
  LOOGUP_ENTRY(sinkrate);
  LOOGUP_ENTRY(altitude);
  LOOGUP_ENTRY(sigma_z_1);
  LOOGUP_ENTRY(sigma_y_1);
  LOOGUP_ENTRY(sigma_z_2);
  LOOGUP_ENTRY(sigma_y_2);
  LOOGUP_ENTRY(sigma_z_3);
  LOOGUP_ENTRY(sigma_y_3);
  LOOGUP_ENTRY(delta_y);
  LOOGUP_ENTRY(delta_z);
#undef LOOGUP_ENTRY

  // attach my activity to incoming data from the pane
  keep_up.setTrigger(input);
  feedback.setTrigger(*Ticker::single());
}

Evaluator::~Evaluator()
{
  //
}

bool Evaluator::isPrepared()
{
  return display_select.isValid()
    && input.isValid()                 // channels valid
    && path.size() == NTUNNELS         // (3) paths defined
    && x_flare.size() == NTUNNELS      // (3) start of flare
    && x_final.size() == NTUNNELS      // (3) start of final
    && feedback_timing_set;            // feedback timing set
}

void Evaluator::startModule(const TimeSpec &time)
{
  keep_up.switchOn(time);
  feedback.switchOn(time);
}

void Evaluator::stopModule(const TimeSpec &time)
{
  keep_up.switchOff(time);
  feedback.switchOff(time);
}


void Evaluator::loadSnapshot(const TimeSpec& ts, const Snapshot& snap)
{
  // this just means that we start anew, reset the counters
  // and zero the deviations
  v_initialy.zero();  v_initialz.zero();
  v_flarey.zero();  v_flarez.zero();
  v_finaly.zero();  v_finalz.zero();
  altitude = 0.0; sinkrate = 0.0;
  is_reset = true;
}

void Evaluator::doCalculation(const TimeSpec& ts)
{
  // check for changes in path
  if (display_select.getNumWaitingEvents(ts)) {
    const Event<DisplaySelect>* e;
    display_select.getNextEvent(e, ts);
    tunnel = e->getEventData()->type % NTUNNELS;
  }

  // do normal simulation/feedback stuff
  switch (getAndCheckState(ts)) {

  case SimulationState::Advance:
    {
      const SpacePlaneY* y;
      input.getAccess(y, ts);

      // read out values from y
      altitude = y->Y[Y_z];
      sinkrate = y->Y[Y_sink_rate];

      // get the normative value for the path
      path[tunnel].charge(y->Y[Y_x]);

      // add the deviations to the variance trackers
      present_x = y->Y[Y_x];

      delta_z = y->Y[Y_z] - path[tunnel].getZ();
      delta_y = y->Y[Y_y];

      if (altitude > z_stop) {
        if (present_x > x_flare[tunnel]) {
          v_initialy.add(y->Y[Y_y]);
          v_initialz.add(y->Y[Y_z] - path[tunnel].getZ());
        }
        else if (present_x > x_final[tunnel]) {
          v_flarey.add(y->Y[Y_y]);
          v_flarez.add(y->Y[Y_z] - path[tunnel].getZ());
        }
        else if (present_x > 0.0) {
          v_finaly.add(y->Y[Y_y]);
          v_finalz.add(y->Y[Y_z] - path[tunnel].getZ());
        }
      }
      input.releaseAccess(y);

      break;
    }

  case SimulationState::HoldCurrent:

    // nothing
    break;
  }
}

void Evaluator::feedBack(const TimeSpec& ts)
{
  // zero the display after a reset/inco load
  if (is_reset) {
    char buf[] = "0.0";
    gtk_entry_set_text(h_altitude, buf);
    gtk_entry_set_text(h_sinkrate, buf);
    gtk_entry_set_text(h_sigma_y_1, buf);
    gtk_entry_set_text(h_sigma_z_1, buf);
    gtk_entry_set_text(h_sigma_y_2, buf);
    gtk_entry_set_text(h_sigma_z_2, buf);
    gtk_entry_set_text(h_sigma_y_3, buf);
    gtk_entry_set_text(h_sigma_z_3, buf);
    is_reset = false;
  }
  else {

    // feed back results to the screen
    char buf[16];
    {
      strstream s1(buf, 16);s1 << delta_y << '\000';
      gtk_entry_set_text(h_delta_y, buf);
      strstream s2(buf, 16);s2 << delta_z << '\000';
      gtk_entry_set_text(h_delta_z, buf);
    }
    {
      strstream s1(buf, 16);s1 << altitude << '\000';
      gtk_entry_set_text(h_altitude, buf);
      strstream s2(buf, 16);s2 << sinkrate << '\000';
      gtk_entry_set_text(h_sinkrate, buf);
    }
    {
      strstream s1(buf, 16); s1 << v_initialy.sigma() << '\000';
      gtk_entry_set_text(h_sigma_y_1, buf);

      strstream s2(buf, 16); s2 << v_initialz.sigma() << '\000';
      gtk_entry_set_text(h_sigma_z_1, buf);
    }
    {
      strstream s1(buf, 16); s1 << v_flarey.sigma() << '\000';
      gtk_entry_set_text(h_sigma_y_2, buf);

      strstream s2(buf, 16); s2 << v_flarez.sigma() << '\000';
      gtk_entry_set_text(h_sigma_z_2, buf);
    }
    {
      strstream s1(buf, 16); s1 << v_finaly.sigma() << '\000';
      gtk_entry_set_text(h_sigma_y_3, buf);

      strstream s2(buf, 16); s2 << v_finalz.sigma() << '\000';
      gtk_entry_set_text(h_sigma_z_3, buf);
    }
  }
}


bool Evaluator::setFile(const vstring& f)
{
  // make a new interpolator
  path.push_back(Interpolator());

  // tell it to read the file
  bool result = path.back().load(f.c_str());

  // undo changes in case of no success
  if (!result) {
    path.pop_back();
    return result;
  }

  return true;
}

#define FT_TO_METER 0.3048

bool Evaluator::setFlare(const double& f)
{
  x_flare.push_back(f * FT_TO_METER);
  return true;
}

bool Evaluator::setFinal(const double& f)
{
  x_final.push_back(f * FT_TO_METER);
  return true;
}

bool Evaluator::setFeedbackRate(const TimeSpec& ts)
{
  feedback.setTimeSpec(ts);
  feedback_timing_set = true;
  return feedback_timing_set;
}

const ParameterTable* Evaluator::getMyParameterTable()
{
  static const ParameterTable table[] = {
    { "set-path",
      new MemberCall<Evaluator,vstring>(&Evaluator::setFile)},

    { "set-flare",
      new MemberCall<Evaluator,double>(&Evaluator::setFlare)},

    { "set-final",
      new MemberCall<Evaluator,double>(&Evaluator::setFinal)},

    { "set-feedback-timing",
      new MemberCall<Evaluator,TimeSpec>(&Evaluator::setFeedbackRate)},

    { "set-stop-height",
      new VarProbe<Evaluator,double>(REF_MEMBER(&Evaluator::z_stop))},

    {NULL, NULL}
  };
  return table;
}

static TypeCreator<Evaluator> a(Evaluator::getMyParameterTable());
