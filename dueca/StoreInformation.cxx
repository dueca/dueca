/* ------------------------------------------------------------------   */
/*      item            : StoreInformation.cxx
        made by         : Rene' van Paassen
        date            : 010412
        category        : body file
        description     :
        changes         : 010412 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define StoreInformation_cxx
#include "StoreInformation.hxx"
DUECA_NS_START

ReflectiveStoreInformation::
ReflectiveStoreInformation(ReflectiveAccessor* accessor,
                           volatile uint32_t** area_cb,
                           volatile uint32_t** area,
                           int area_size,
                           int no_parties,
                           int node_id) :
  accessor(accessor),
  area_cb(area_cb),
  area(area),
  area_size(area_size),
  no_parties(no_parties),
  node_id(node_id)
{
  //
}

ostream& operator << (ostream& os, const
                      ReflectiveStoreInformation& o)
{
  os << "ReflectiveStoreInformation(accessor="
     << reinterpret_cast<void*>(o.accessor)
     << ", area_cb=(";
  for (int ii = 0; ii < o.no_parties; ii++) {
    os << reinterpret_cast<volatile void*>(o.area_cb[ii]);
    if (ii < o.no_parties - 1) os << ',';
  }
  os << "), area=(";
  for (int ii = 0; ii < o.no_parties; ii++) {
    os << reinterpret_cast<volatile void*>(o.area[ii]);
    if (ii < o.no_parties - 1) os << ',';
  }
  return os << "), area_size=" << o.area_size << ", no_parties="
            << o.no_parties << ", node_id=" << o.node_id << ")";
}

DUECA_NS_END

