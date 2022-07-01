/* ------------------------------------------------------------------   */
/*      item            : GlobalIdExtra.cxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional body code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

// include a boost header, and since we are in dueca namespace,
// temporarily exit
DUECA_NS_END;
#include <boost/numeric/conversion/cast.hpp>
#include <boost/lexical_cast.hpp>
DUECA_NS_START;

// provide a custom function over the standard print function for an ID
#define __CUSTOM_FUNCTION_PRINT
std::ostream& GlobalId::print(std::ostream& o) const
{
  o << "id(";
  if (location == invalid_location_id)
    o << '-'; else o << boost::numeric_cast<int>(location);
  if (object == invalid_object_id)
    o << ",-"; else o << ',' << object;
  return o << ')';
}

std::string GlobalId::printid() const
{
  return
    (location == invalid_location_id ? std::string("-") :
     boost::lexical_cast<std::string>(int(location))) +
    std::string(",") +
    (object == invalid_object_id ? std::string("-") :
     boost::lexical_cast<std::string>(object));
}
