/* ------------------------------------------------------------------   */
/*      item            : SimpleFunction.hxx
        made by         : Rene van Paassen
        date            : 031212
        category        : header file
        description     :
        changes         : 031212 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SimpleFunction_hxx
#define SimpleFunction_hxx

#ifdef SimpleFunction_cxx
#endif

#include <dueca_ns.h>
DUECA_NS_START

/** Base class for simple (one parameter) double precision
    functions. */
class SimpleFunction
{
public:
  /** Constructor. */
  SimpleFunction();

  /** Destructor */
  virtual ~SimpleFunction();

  /** Main function */
  virtual double operator () (const double x) const = 0;
};

DUECA_NS_END

#endif
