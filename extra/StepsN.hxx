/* ------------------------------------------------------------------   */
/*      item            : StepsN.hxx
        made by         : Rene van Paassen
        date            : 030228
        category        : header file
        description     :
        changes         : 030228 first version
                          170412 extended documentation
                          170412 added copy constructor and assignment
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef StepsN_hxx
#define StepsN_hxx

#ifdef StepsN_cxx
#endif

#include <dueca_ns.h>
#include "SimpleFunction.hxx"
#include <iostream>

DUECA_NS_START

/** Implementation of a scaling/converting device, that produces a
    discrete number of different output values depending on a
    continous input. This can be used to e.g. implement IO signals for
    a flap handle.

    To create the object, call the constructor with a list of desired
    output values and a list of corresponding raw input values. When
    called to convert an input value, the closest input value from the
    list is found, and the corresponding output is returned.
*/
class StepsN: public SimpleFunction
{
  /** Dimension of steps */
  unsigned int n;

  /** Array of output values. */
  double* y;

  /** Array of input values. */
  double* ui;

  /** Array of input/measured value transition points. */
  double* tp;

public:
  /** Constructor.
      \param n     Number of input-output value pairs.
      \param yi    Set of possible output values for this device.
      \param ui    Input values corresponding to the output
                   values. */
  StepsN(unsigned int n, double yi[], double ui[]);

  /** Copy constructor */
  StepsN(const StepsN& o);

  /** Assignment */
  StepsN& operator = (const StepsN& o);

  /** Destructor. */
  ~StepsN();

  /** The operator.
      @param x     Raw value
      @returns     Converted output. */
  double operator () (const double x) const;

  /** Print the object */
  std::ostream& print (std::ostream& os) const;
};
DUECA_NS_END

inline std::ostream& operator << (std::ostream& os, const DUECA_NS::StepsN& o)
{ return o.print(os); }

#endif
