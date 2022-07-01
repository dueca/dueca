/* ------------------------------------------------------------------   */
/*      item            : ChannelEndUpdateExtra.cxx
        made by         : Rene' van Paassen
        date            : 010823
        category        : body file
        description     :
        changes         : 170908 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110
#include <ChannelDef.hxx>

#define __CUSTOM_FUNCTION_PRINT
std::ostream & ChannelEndUpdate::print(std::ostream& s) const
{
  s << "ChannelEndUpdate("
    << "update=" << this->update << ','
    << "name_set=" << this->name_set << ','
    << "end_id=" << this->end_id << ','
    << "destination_id=" << this->destination_id << ','
    << "transportclass=" << Channel::TransportClass(this->transportclass)
    << ')';
  return s;
}
