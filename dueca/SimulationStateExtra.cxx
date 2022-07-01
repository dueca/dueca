/* ------------------------------------------------------------------   */
/*      item            : SimulationStateExtra.cxx
        made by         : Rene' van Paassen
        date            : 990713
        category        : body file
        description     :
        changes         : 990713 first version
                          130929 adapted to .dco file inclusion
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

#define __CUSTOM_GETSTRING_Type
// alternative names
const char* const getString(const DUECA_NS::SimulationState::Type &o)
{
  static const char* Type_names[] = {
    "HoldCurrent",
    "Advance",
    "Replay",
    "Inactive",
    "Ina->Hold",
    "Cal->Hold",
    "Adv->Hold",
    "Rep->Hold",
    "Hold->Ina",
    "Neutral",
    "Undefined"
  };
  return Type_names[int(o)];
}

SimulationState::SimulationState(const std::string& s) :
  t(Type(0))
{
  readFromString(this->t, s);
}

const char* const SimulationState::getString() const
{
  return DUECA_NS::getString(t);
}

SimulationState::Type SimulationState::transitionFinal() const
{
  static SimulationState::Type transitions[] = {
    HoldCurrent,
    Advance,
    Replay,
    Inactive,
    HoldCurrent,
    HoldCurrent,
    HoldCurrent,
    HoldCurrent,
    Inactive,
    Neutral,
    Undefined};
  return transitions[int(t)];
}

SimulationState& SimulationState::operator &= (const SimulationState& o)
{
  // rows:
  // HoldCurrent, Advance, Replay, Inactive,
  //             Ina->Hold, Cal->Hold, Adv->Hold, Hold->Ina,
  //             Neutral, Undefined

  static Type ttable[][int(Undefined)+1] =
  {
    {},                        // <- HoldCurrent with ...
    {Undefined},               // <- Advance with HoldCurrent
    {Undefined, Undefined},    // <- Replay with HoldCurrent and Advance
    {Undefined, Undefined, Undefined},  // <- Inactive with HoldCurrent
                                        // .. Replay
    {Inactive_HoldCurrent, Undefined, Undefined, Undefined},
                               // <- Inactive_Holdcurrent HoldCurrent .. Replay
    {Calibrate_HoldCurrent, Undefined, Undefined, Undefined,
     Undefined},               // <- Calibrate_HoldCurrent with
                               // HoldCurrent .. Inactive_HoldCurrent
    {Advance_HoldCurrent, Undefined, Undefined, Undefined,
     Undefined, Undefined},    // <- Advance_HoldCurrent with ...
    {Replay_HoldCurrent, Undefined, Undefined, Undefined, Undefined,
     Undefined, Undefined},    // <- Replay_HoldCurrent with HoldCurrent
                               // .. Advance_HoldCurrent
    {Undefined, Undefined, Undefined, HoldCurrent_Inactive,
     Undefined, Undefined, Undefined, Undefined},
                               // <- HoldCurrent_Inactive with HoldCurrent ..
                               // Replay_Holdcurrent
    {HoldCurrent, Advance, Replay, Inactive,
     Inactive_HoldCurrent, Calibrate_HoldCurrent,
     Advance_HoldCurrent, Replay_HoldCurrent, HoldCurrent_Inactive},
                               // <- Neutral with HoldCurrent ..
                               // HoldCurrent_Inactive
    {Undefined, Undefined, Undefined, Undefined, Undefined,
     Undefined, Undefined, Undefined, Undefined, Undefined}
                               // <- Undefined with anything else
  };

  if (o.t == t) {
    // t stays the same
  }
  else if (o.t < t) {
    t = ttable[int(t)][int(o.t)];
  }
  else {
    t = ttable[int(o.t)][int(t)];
  }
  return *this;
}

SimulationState SimulationState::operator && (const SimulationState&
                                               o) const
{
  SimulationState r(*this);
  r &= o;
  return r;
}

bool SimulationState::betterOrSame(const SimulationState& o) const
{
  if (t == o.t) return true;
  switch(t) {
    case Inactive:
      return (o.t == HoldCurrent);
  default:
    return false;
  }
}

