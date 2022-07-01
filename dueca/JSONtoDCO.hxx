/* ------------------------------------------------------------------   */
/*      item            : JSONtoDCO.hxx
        made by         : Rene van Paassen
        date            : 181027
        category        : header file
        description     :
        changes         : 181027 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef JSONtoDCO_hxx
#define JSONtoDCO_hxx

#include <dueca_ns.h>
#include <dueca/CommObjectWriter.hxx>

/** Macro definition allowing NaN and Inf parsing */
#define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseNanAndInfFlag
#include <rapidjson/document.h>

/** @file JSONtoDCO.hxx

    Conversion routines for JSON to DCO */

DUECA_NS_START;
class CommObjectWriter;

/** Shorthand for the used JSON type */
typedef rapidjson::GenericValue<rapidjson::UTF8<> > JValue;

/** Convert the data from a JSON stringbuffer to a DCO object

    @param reader    RapidJSON value object.
    @param writer    DCO channel access object.
*/
void JSONtoDCO(const JValue &reader,
               CommObjectWriter& writer);

/** classname function, should exist for DCO objects */
template <typename T> const char* getclassname();

/** Convert the data from a JSON representation into
    a DCO object.
    Templated version, directly access the (known) object.

    @param reader   JSON representation.
    @param object   Object to be written.
    @tparam WR      Compatible type for JSON writing
    @tparam DCO     Class of the DCO object
*/
template<class RD, class DCO>
void json_to_dco(RD &reader, DCO& object)
{
  CommObjectWriter cow(getclassname<DCO>(), reinterpret_cast<void*>(&object));
  JSONtoDCO(reader, cow);
}


DUECA_NS_END;
#endif
