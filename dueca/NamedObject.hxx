/* ------------------------------------------------------------------   */
/*      item            : NamedObject.hh
        made by         : Rene' van Paassen
        date            : 980211
        category        : header file
        description     : Basic named object, these objects can be found
                          through the CSE registry
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef NamedObject_hh
#define NamedObject_hh

#include "GlobalId.hxx"
#include "dstypes.h"
//#include <list>

#include <dueca_ns.h>
DUECA_NS_START
struct NameSet;
class Destination;
class GenericChannel;

/** This class is a common base to all objects with a name. These
    objects will be locatable by the CSE registry.

    The name consists of a tuple (entity, cclass, part)

    <ul>
    <li> entity is the simulated entity, e.g. "PH-LAB"
    <li> cclass is the model class of the channel or component,
    e.g. "engine signals"
    <li> part is the part within the entity, e.g. "engine 1"
    </ul>

    The NamedObject class links the name tuple to an object id. The
    object id consists of another tuple (location, object).

    <ul>
    <li> location is the DUECA node where an object is located.
    <li> object is an id of the object itself.
    </ul>

    Note that the entity, class, part tuple is stored, upon creation,
    in the registry. The methods that query this tuple use the
    registry, in combination with my_id, to return the answer.

    The registry will provide methods to obtain the object id from the
    (entity, class, part) tuple and vice versa.  */
class NamedObject
{
  /** identifying id, composed of location and object id, and unique
      within the simulation */
  GlobalId my_id;

  /** copy constructor, these objects, nor descendants, are not supposed
      to be copied */
  NamedObject(const NamedObject& no);
  /// Cannot assign, ...
  NamedObject& operator = (const NamedObject&);
protected:

  /** Normal constructor, protected, because it has no use to create a
      NamedObject by itself */
  NamedObject(const NameSet& ns);
  /// Destructor
  virtual ~NamedObject();

private:
  /// An ObjectManager may initialise its name later
  friend class ObjectManager;
  /// And also the ScriptInterpret object may do this
  friend class ScriptInterpret;
  /// This is used to give a delayed name
  void delayedInit(const NameSet& name_set);
  /// And the corresponding constructor, without name
  NamedObject();

public:
  /// Returns the "entity" part of the name
  const std::string getEntity() const;
  /// Returns the "class" part of the name
  const std::string getClass() const;
  /// Returns the sub-entity or "part" part of the name
  const std::string getPart() const;
  /// This returns the complete name set
  const NameSet& getNameSet() const;
  /** This returns the id. Normally this is used by a derived class,
      because the id has to be supplied to obtain various services,
      such as channel access and activation */
  const GlobalId& getId() const;

  /** This has to be re-implemented in a derived class, used to get an
      idea of what the derived class does */
  virtual ObjectType getObjectType() const = 0;
};

DUECA_NS_END
#endif
