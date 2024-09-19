/* ------------------------------------------------------------------   */
/*      item            : DCOtoJSON.hxx
        made by         : Rene van Paassen
        date            : 180518
        category        : header file
        description     :
        changes         : 180518 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DCOtoJSON_hxx
#define DCOtoJSON_hxx

#include <dueca_ns.h>
#include <rapidjson/ostreamwrapper.h>

/// Define relaxed acceptance of NaN and Inf
#define RAPIDJSON_WRITE_DEFAULT_FLAGS kWriteNanAndInfFlag
#include "rapidjson/writer.h"
#include <rapidjson/stringbuffer.h>
#include <dueca/CommObjectReader.hxx>

/** @file DCOtoJSON.hxx

    Conversion routines for DCO to JSON */

DUECA_NS_START;
class CommObjectReader;

/** Convert the data from a DCO object to a JSON stringbuffer

    Compact variant, does not code DCO type information, so you need
    to be sure of the DCO type to interpret this data. Also "extended"
    JSON, codes NaN, Infinite and -Infinite. Used internally in DUECA,
    careful about use with external clients.

    @param writer   RapidJSON writer object
    @param reader   Channel access object.
 */
void DCOtoJSONcompact(rapidjson::Writer<rapidjson::OStreamWrapper> &writer,
                      const CommObjectReader& reader);

/** Convert the data from a DCO object to a JSON stringbuffer

    Compact variant, does not code DCO type information, so you need
    to be sure of the DCO type to interpret this data. Also "extended"
    JSON, codes NaN, Infinite and -Infinite. Used internally in DUECA,
    careful about use with external clients.

    @param writer   RapidJSON writer object
    @param reader   Channel access object.
 */
void DCOtoJSONcompact(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                      const CommObjectReader& reader);

/** Convert the data from a DCO object to a JSON stringbuffer

    Compact variant, does not code DCO type information, so you need to be
    sure of the DCO type to interpret this data. Also "extended"
    JSON, codes NaN, Infinite and -Infinite. Used internally in DUECA,
    careful about use with external clients.

    @param doc      Stringbuffer
    @param dcoclass Type name of the DCO object
    @param object   Void pointer to the object with the data.
 */
void DCOtoJSONcompact(rapidjson::StringBuffer &doc,
                      const char* dcoclass, const void* object);

/** Convert the data from a DCO object to a JSON stringbuffer

    Compact variant, does not code DCO type information, so you need
    to be sure of the DCO type to interpret this data. Stricter
    version, +Infinity and -Infinity are converted to large floats,
    NaN is converted to NULL.

    @param writer   RapidJSON writer object
    @param reader   Channel access object.
 */
void DCOtoJSONstrict(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                     const CommObjectReader& reader);

/** Convert the data from a DCO object to a JSON stringbuffer

    Compact variant, does not code DCO type information, so you need
    to be sure of the DCO type to interpret this data. Stricter
    version, +Infinity and -Infinity are converted to large floats,
    NaN is converted to NULL.

    @param writer   RapidJSON writer object
    @param reader   Channel access object.
 */
void DCOtoJSONstrict(rapidjson::Writer<rapidjson::OStreamWrapper> &writer,
                     const CommObjectReader& reader);

/** Convert the data from a DCO object to a JSON stringbuffer

    Compact variant, does not code DCO type information, so you need to be
    sure of the DCO type to interpret this data. Stricter
    version, +Infinity and -Infinity are converted to large floats,
    NaN is converted to NULL.

    @param doc      Stringbuffer
    @param dcoclass Type name of the DCO object
    @param object   Void pointer to the object with the data.
 */
void DCOtoJSONstrict(rapidjson::StringBuffer &doc,
                     const char* dcoclass, const void* object);

/** classname function, should exist for DCO objects */
template <typename T> const char* getclassname();

/** Convert the data from a DCO object into a JSON writer
    Templated version, directly access the (known) object.

    @param writer   JSON writer object.
    @param object   Object to be read.
    @tparam WR      Compatible type for JSON writing
    @tparam DCO     Class of the DCO object
*/
template<class WR, class DCO>
void dco_to_json(WR &writer, const DCO& object)
{
  CommObjectReader reader(getclassname<DCO>(),
			  reinterpret_cast<const void*>(&object));
  DCOtoJSONstrict(writer, reader);
}

DUECA_NS_END;

#endif
