/* ------------------------------------------------------------------   */
/*      item            : DCOtypeJSON.hxx
        made by         : Rene van Paassen
        date            : 220214
        category        : header file
        description     :
        changes         : 220214 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DCOtypeJSON_hxx
#define DCOtypeJSON_hxx

#include <dueca_ns.h>
/// \cond DO_NOT_DOCUMENT
#define RAPIDJSON_WRITE_DEFAULT_FLAGS kWriteNanAndInfFlag
/// \endcond
#include "rapidjson/writer.h"
#include <rapidjson/stringbuffer.h>

/** @file DCOtypeJSON.hxx

    Conversion routines to code DCO type information into JSON */

DUECA_NS_START;

/** Print the type information on a DCO defined type to JSON

    @param writer    Writer object
    @param dcoclass  DCO type class name

    The JSON struct has the following format:

    - "class", with a string giving the class name of the object.
    - "type", here this is always "object".
    - "members", for a non-primitive type, array of data/structure members.

    The data members array has the following contents for each entry:

    - "name", a string giving the member name
    - "class", as above for the object
    - "type", may be "object" as above, to indicate a nested object, or
      alternatively "primitive" (c/c++ type, or common std types like
      std::string) or "enum", in which case data are string values
      representing enumeration items.
    - "enumvalues" (only with enum), a nested object with keys representing
      enum writing mapping to the integer represenation of the enums.
    - "enumclass" (only with enum), integer class representing enum
    - "size" (optional), an integer indicating array size if the size of
      the arrays is fixed.
    - "container" (optional),
      * "array" if the member is a list, array, vector or the like,
      * "map" if the member is a mapped type. In this case also the
        variable "key_class" is present.

 */
void DCOtypeJSON(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                 const char* dcoclass);

/** Print the type information on a DCO defined type to JSON

    @param doc       Stringbuffer
    @param dcoclass  DCO type class name
 */
void DCOtypeJSON(rapidjson::StringBuffer &doc,
                 const char* dcoclass);

DUECA_NS_END;

#endif
