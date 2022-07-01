/* ------------------------------------------------------------------   */
/*      item            : IncoVariableExtra.cxx
        made by         : Rene' van Paassen
        date            : 130104
        category        : additional body code
        description     :
        changes         : 130102 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define __CUSTOM_COMPATLEVEL_110

static const double EPS_INCO = 1e-5;

IncoVariable* IncoVariable::forMode(IncoMode mode, IncoRole r)
{
  // some sanity checks. Only warn however.
  if (vartype == IncoInt && r != Constraint) {
    cerr << "Inco variable \"" << name << "\" is Int, cannot be used as "
         << r << endl;
  }
  else if (r == Target && fabs(tolerance) < 1e-14) {
    cerr << "Tolerance for Inco target \"" << name << '\"' << endl;
  }
  else if (findRole(mode) != Unspecified) {
    cerr << "Mode " << mode << "already has role " << findRole(mode)
         << endl;
  }
  else {
    // add the new role
    roles[mode] = r;
  }
  return this;
}

IncoVariable::IncoVariable(const char* name,
                           int min_val, int max_val) :
  name(name),
  min_value(min_val),
  max_value(max_val),
  tolerance(0),
  vartype(IncoInt),
  roles()
{
  //
}

IncoRole IncoVariable::findRole(IncoMode mode) const
{
  if (roles.find(mode) == roles.end()) {
    return Unspecified;
  }
  return const_cast<IncoVariable*>(this)->roles[mode];
}

bool IncoVariable::queryInsertForThisMode(IncoMode mode) const
{
  return findRole(mode) == Control || findRole(mode) == Constraint;
}

bool IncoVariable::isUserControllable(IncoMode mode) const
{
  return findRole(mode) == Constraint ||
    (findRole(mode) == Target && fabs(max_value - min_value) > 1e-10);
}

