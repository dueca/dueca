/* ------------------------------------------------------------------   */
/*      item            : RateReduction.cxx
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


#define RateReduction_cc
#include "RateReduction.hxx"

#include <Ticker.hxx>
extern "C" {
#include "natmio.h"
}
#include "config.h"

#define DO_INSTANTIATE
#include <dueca.h>

const char* const RateReduction::classname = "rate-reduction";

RateReduction::RateReduction(Entity* e,
                             const char* part,
                             const PrioritySpec& ps) :
  Module(e, classname, part),

  // output channels
  controls_out(getId(), NameSet(getEntity(), "PrimaryControls", part)),
  controls_in(getId(), NameSet(getEntity(), "PrimaryControls", "stick")),

  // callback functions and activities
  cb1(this, &RateReduction::doStep),
  reduce_rate(getId(), "control input", &cb1, ps)
{
  reduce_rate.setTrigger(controls_in);
}

RateReduction::~RateReduction()
{
  //
}

bool RateReduction::isPrepared()
{
  return controls_in.isValid() && controls_out.isValid();
}

bool RateReduction::setTiming(const TimeSpec& ts)
{
  reduce_rate.setTimeSpec(ts);
  return true;
}

void RateReduction::startModule(const TimeSpec &time)
{
  reduce_rate.switchOn(time);
}

void RateReduction::stopModule(const TimeSpec &time)
{
  reduce_rate.switchOff(time);
}


void RateReduction::doStep(const TimeSpec& ts)
{
  // measure the input, copy to the output
  const PrimaryControls *in; controls_in.getAccess(in, ts);
  PrimaryControls *out; controls_out.getAccess(out, ts);

  out->stick_roll  = in->stick_roll;
  out->stick_pitch = in->stick_pitch;
  out->roll_moment = in->roll_moment;
  out->pitch_moment= in->pitch_moment;
  out->roll_rate   = in->roll_rate;
  out->pitch_rate  = in->pitch_rate;


  controls_in.releaseAccess(in);
  controls_out.releaseAccess(out);
}

const ParameterTable* RateReduction::getMyParameterTable()
{
  static const ParameterTable table[] = {

    { "set-timing", new MemberCall<RateReduction,TimeSpec>
      (&RateReduction::setTiming)},

    {NULL, NULL}};
  return table;
}

static TypeCreator<RateReduction> a(RateReduction::getMyParameterTable());



