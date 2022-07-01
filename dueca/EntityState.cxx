/* ------------------------------------------------------------------   */
/*      item            : EntityState.cxx
        made by         : Rene' van Paassen
        date            : 990727
        category        : body file
        description     :
        changes         : 990727 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define EntityState_cc
#include "EntityState.hxx"
DUECA_NS_START

static const char* names[] = {
  "EntityUnkown",
  "EntityCreated",
  "EntityExists",
  "EntityInitialPrepared",
  "EntityPrepared",
  "EntityRunning",
  "EntityNotHere",
  "EntityStart",
  "EntityStop",
  "EntityInitialStart",
  "EntityFinalStop",
  "EntityQuery"};

ostream& operator << (ostream& os, const EntityState& o)
{
  return os << names[int(o)];
}

const char* const getString(const EntityState& o)
{
  return names[int(o)];
}
DUECA_NS_END
