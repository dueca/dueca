/* ------------------------------------------------------------------   */
/*      item            : GenericCallback.hh
        made by         : Rene' van Paassen
        date            : 980709
        category        : header file
        description     : Function object, to be derived for various
                          callback types.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef GenericCallback_hh
#define GenericCallback_hh

#include<iostream>
using namespace std;

#include <dueca_ns.h>
DUECA_NS_START
class TimeSpec;
/** Implements callback objects.

    Not usable as such, one needs to derive a class that actually does
    something. GenericCallback objects are required by
    ActivityCallback objects. */
class GenericCallback
{
public:
  /** The callback action. */
  virtual void operator() (const TimeSpec &t) = 0;

  /** print to stream, for debugging. */
  virtual void print(ostream& os) const = 0;

  /// Constructor.
  GenericCallback();

  /// Destructor.
  virtual ~GenericCallback();

private:
  /// Copying is not possible.
  GenericCallback(const GenericCallback&);

  /// Nor is assignment.
  GenericCallback& operator = (const GenericCallback&);
};

extern ostream& operator << (ostream& os, const GenericCallback& callback);

DUECA_NS_END

#endif
