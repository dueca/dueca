/* ------------------------------------------------------------------   */
/*      item            : EntityState.hh
        made by         : Rene' van Paassen
        date            : 990727
        category        : header file
        description     :
        changes         : 990727 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef EntityState_hh
#define EntityState_hh

#include <iostream>
#include "AmorphStore.hxx"

#include <dueca_ns.h>
DUECA_NS_START
/** The logic of entity states. These describe the entity states from
    a dueca viewpoint, so purely the running state of a system. States
    from a dusime viewpoint, so from the functionality within a
    simulation, are defined in the SimulationState class. */
enum EntityState {
  EntityUnknown,  /**< The state of the entity is not known. */
  EntityCreated,  /**< The entity has been freshly created. */
  EntityExists,   /**< The entity has been created. */
  EntityInitialPrepared, /**< The entity is prepared for initial
                            starting, which means connection with the
                            hardware in a safe mode. */
  EntityPrepared, /**< The entity is prepared for further starting,
                       meaning connection with the model /
                       application. */
  EntityRunning,  /**< The entity is running. */
  EntityNotHere,  /**< not at a certain location. */
  EntityStart,    /**< command. */
  EntityStop,      /**< command. */
  EntityInitialStart,/**< command. */
  EntityFinalStop,/**< command. */
  EntityQuery     /**< command. */
};

inline void packData(AmorphStore &s, const EntityState& o)
{
  ::packData(s, int8_t(o));
}

inline void unPackData(AmorphReStore &s, EntityState& o)
{
  int8_t tmp;
  ::unPackData(s, tmp);
  o = EntityState(tmp);
}


const char* const getString(const EntityState& o);
ostream& operator << (ostream& os, const EntityState& o);

DUECA_NS_END
#endif

