/* ------------------------------------------------------------------   */
/*      item            : ArgElement.hxx
        made by         : Rene van Paassen
        date            : 180322
        category        : header file
        description     :
        changes         : 180322 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ArgElement_hxx
#define ArgElement_hxx

#include <boost/any.hpp>
#include "dueca_ns.h"
#include <list>

DUECA_NS_START

/** Result for a parsed element */
struct ArgElement {
  /** Index referring to the parameter table element */
  unsigned idx;
  /** Object containing a copy of the data */
  boost::any value;
  /** Constructor */
  ArgElement(const unsigned idx, const boost::any value);
  /** Copy constructor */
  ArgElement(const ArgElement& o);
  /** Assignment */
  ArgElement& operator= (const ArgElement& o);
  /** List of these */
  typedef std::list<ArgElement> arglist_t;
};

DUECA_NS_END
#endif
