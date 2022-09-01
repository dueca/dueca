/* ------------------------------------------------------------------   */
/*      item            : EasyId.hxx
        made by         : Rene van Paassen
        date            : 220700
        category        : header file
        description     :
        changes         : ?? first version
        language        : C++
        api             : DUECA_API
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

/** Internally in DUECA, some objects need an id, but cannot derive
    from NamedObject, because they could be initialized automatically
    before the ObjectManager. A pointer to an EasyId can then be used,
    and the EsyId is instantiated later.

    In some cases, having a class that derives from EasyId can be
    useful in application code. Only do this when you have an object
    that is not a Module, and also not associated with a (single)
    Module, but that still needs some Module-like capabilities, such
    as accessing a channel.

    When having classes that are clearly associated with a
    dueca::Module (helper classes to handle a specific channel entry
    or the like), instead use dueca::AssociateObject, which can
    "borrow" access to the Module's name and identity.
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
