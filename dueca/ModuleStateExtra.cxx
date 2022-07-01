/* ------------------------------------------------------------------   */
/*      item            : ModuleStateExtra.cxx
        made by         : Rene' van Paassen
        date            : 010823
        category        : body file
        description     :
        changes         : 010823 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

ModuleState::ModuleState(const  std::string& s) :
  t(Undefined)
{
  readFromString(this->t, s);
}

ModuleState ModuleState::operator && (const ModuleState& o) const
{
  ModuleState r(*this);
  r &= o;
  return r;
}

ModuleState& ModuleState::operator &= (const ModuleState& o)
{
  static Type ttable[int(Undefined)+1][int(Undefined)+1] =
  {
    { UnPrepared, UnPrepared, UnPrepared, UnPrepared,
      Undefined, UnPrepared, Undefined },   // me UnPrepared
    { UnPrepared, InitialPrep, InitialPrep, InitialPrep,
      Undefined, InitialPrep, Undefined },  // me InitialPrep
    { Undefined, Undefined, Safe, Safe, Undefined,
      Safe, Undefined },         // me Safe
    { UnPrepared, InitialPrep, Safe, Prepared, Undefined,
      Prepared, Undefined },     // me Prepared
    { Undefined, Undefined, Undefined, Undefined, On,
      On, Undefined },           // me On
    { UnPrepared, InitialPrep, Safe, Prepared, On,
      Neutral, Undefined },      // me Neutral
    { Undefined, Undefined, Undefined, Undefined, Undefined,
      Undefined, Undefined }     // me Undefined
  };
  t = ttable[int(t)][int(o.t)];
  return *this;
}

const char* ModuleState::getString() const
{
  return DUECA_NS::getString(t);
}
