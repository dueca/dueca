/* ------------------------------------------------------------------   */
/*      item            : EasyId.hxx
        made by         : Rene van Paassen
        date            : 061215
        category        : header file
        description     :
        changes         : 061215 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef EasyId_hxx
#define EasyId_hxx

#include <dueca_ns.h>
#include <GlobalId.hxx>
#include <NamedObject.hxx>
#include <NameSet.hxx>

DUECA_NS_START

/** Some objects need an id, but cannot derive from NamedObject,
    because they could be initialized automatically before the
    ObjectManager. A pointer to an EasyId can be used. */
class EasyId: public NamedObject
{
public:
  /** Needed for NamedObject. */
  ObjectType getObjectType() const;

  /** Constructor. */
  EasyId(const char* entity, const char* name, int part);
};

DUECA_NS_END

#endif
