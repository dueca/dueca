/* ------------------------------------------------------------------   */
/*      item            : NamedChannel.hh
        made by         : Rene' van Paassen
        date            : 990824
        category        : header file
        description     :
        changes         : 990824 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NamedChannel_hh
#define NamedChannel_hh

#ifdef NamedChannel_cc
#endif

#include "GlobalId.hxx"
#include "dstypes.h"
//#include <list>

#include <dueca_ns.h>
#include <NameSet.hxx>
DUECA_NS_START


struct NameSet;
class Destination;
class GenericChannel;

/** This class is a common base to all objects with a name. These
    objects will be locatable by the CSE registry.

    The name consists of a tuple (entity, class, part)
    <ul>
    <li> entity is the simulated entity, e.g. "PH-LAB"
    <li> class is the model class of the channel or component,
    e.g. "engine signals"
    <li> part is the part within the entity, e.g. "engine 1"
    </ul>

    The NamedObject class links the name tuple to an object id. The
    object id consists of another tuple (location, object).
    <ul>
    <li> location is the process (with a local RCI) where an object
    is located.
    <li> object is an id of the object itself.
    </ul>

    note that the entity, class, part tuple is stored, upon creation,
    in the registry. The methods that query this tuple use the
    registry, in combination with my_id, to return the answer.

    The registry will provide methods to obtain the object id from the
    (entity, class, part) tuple and vice versa.  */
class NamedChannel
{

private:
  /** Identifying id, composed of location and object id, and unique
      within the simulation. */
  GlobalId my_id;

  /** Local copy of the name, while the id not yet good */
  NameSet local;

  /** Copy constructor, these objects are not supposed to be copied. */
  NamedChannel(const NamedChannel& no);

protected:

  /** Constructor. */
  NamedChannel(const NameSet& nameset);

  /** Destructor. */
  virtual ~NamedChannel();

public:
  /** Return the entity name. */
  const std::string getEntity() const;

  /** Return the class name. */
  const std::string getClass() const;

  /** Return the part name. */
  const std::string getPart() const;

  /** Return the complete name set. */
  const NameSet& getNameSet() const;

  /** Return the id. */
  const GlobalId& getId() const;

protected:
  /** Change the given id. Id's are issued by the node 0, so it may
      cost a little time before it arrives. */
  virtual void setId(const GlobalId& gid);

public:

  /** Tell what kind of object this is. */
  virtual ObjectType getObjectType() const = 0;
};

DUECA_NS_END
#endif
