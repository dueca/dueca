/* ------------------------------------------------------------------   */
/*      item            : EasyId.cxx
        made by         : Rene' van Paassen
        date            : 061215
        category        : body file
        description     :
        changes         : 061215 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define EasyId_cxx
#include "EasyId.hxx"

DUECA_NS_START

ObjectType EasyId::getObjectType() const
{
  return O_Dueca;
}


EasyId::EasyId(const char* entity,
               const char* name, int part) :
  NamedObject(NameSet(entity, name, part))
{
  //
}

DUECA_NS_END
