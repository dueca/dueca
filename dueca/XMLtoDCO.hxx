/* ------------------------------------------------------------------   */
/*      item            : XMLtoDCO.hxx
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

#ifndef XMLtoDCO_hxx
#define XMLtoDCO_hxx

#include <dueca_ns.h>
#include <pugixml.hpp>

/** @file XMLtoDCO.hxx

    Conversion routines for XML to DCO. */

DUECA_NS_START;

/** Exception type thrown when XML data cannot be fitted in a given
    DCO object. */
struct xmldecodeexception: public std::exception
{
  /** Message */
  const char* reason;

  /** Constructor

      @param re   reason for the exception
  */
  xmldecodeexception(const char* re);

  /** Return the message */
  const char* what() const noexcept;
};

class CommObjectWriter;

/** Convert the data from an XML stringbuffer to a DCO object.

    @param reader    XML node value object.
    @param writer    DCO channel access object.
    @throw dueca::xmlcodeexception Data cannot be interpreted as part of
                     the dco object.
*/
void XMLtoDCO(const pugi::xml_node& reader,
              CommObjectWriter& writer);

DUECA_NS_END;
#endif
