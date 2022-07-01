/* ------------------------------------------------------------------   */
/*      item            : DCOtoXML.hxx
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

#ifndef DCOtoXML_hxx
#define DCOtoXML_hxx

#include <dueca_ns.h>
#include <pugixml.hpp>

/** @file DCOtoXML.hxx

    Conversion routines for writing DCO objects to XML. */

DUECA_NS_START;

class CommObjectReader;

/** Convert the data from a DCO object to an XML stringbuffer.

    Codes DCO type information, using a dueca::CommObjectReader
    object. Note that the dueca::DCOReader that can be created with an
    access token is also a dueca::CommObjectReader.

    @param writer   XML node writer object
    @param reader   Channel access object.
    @param meta     If true, the type information is added for member values.
 */
void DCOtoXML(pugi::xml_node &writer,
              CommObjectReader& reader,
              bool meta=false);

/** Convert the data from a DCO object to a JSON stringbuffer

    Codes DCO type information, using the name of the coded class, and
    a pointer to the data.

    @param writer   XML node writer object
    @param dcoclass Name of the object class.
    @param object   Pointer to the object to be read.
    @param meta     If true, the type information is added for member values.
 */
void DCOtoXML(pugi::xml_node &writer,
              const char* dcoclass, const void* object,
              bool meta=false);

DUECA_NS_END;

#endif
