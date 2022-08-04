/* ------------------------------------------------------------------   */
/*      item            : EasyId.hxx
        made by         : Rene van Paassen
        date            : 061215
        category        : header file
        description     :
        changes         : 061215 first version
        language        : C++
	api             : DUECA_API
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
    ObjectManager. A pointer to an EasyId can be used.

    In rare cases, this can also be used in application code, however,
    for your own sake, avoid mis-using this; to give NamedObject
    capabilities to helper classes (e.g., for opening a channel,
    activities, callback), better use dueca::AssociateObject.
*/
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
