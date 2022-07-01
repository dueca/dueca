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

/** Print the type infromation on a DCO defined type to JSON

    @param writer    Writer object
    @param dcoclass  DCO type class name
 */
void DCOtypeJSON(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                 const char* dcoclass);

/** Print the type infromation on a DCO defined type to JSON

    @param doc       Stringbuffer
    @param dcoclass  DCO type class name
 */
void DCOtypeJSON(rapidjson::StringBuffer &doc,
                 const char* dcoclass);

DUECA_NS_END;

#endif
