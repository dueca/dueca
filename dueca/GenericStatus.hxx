/* ------------------------------------------------------------------   */
/*      item            : GenericStatus.hxx
        made by         : Rene van Paassen
        date            : 010822
        category        : header file
        description     :
        changes         : 010822 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GenericStatus_hxx
#define GenericStatus_hxx

#include <iostream>
using namespace std;
#include <dueca_ns.h>
DUECA_NS_START

/** A base class for status objects. These can be queried, printed,
    and combined. */
class GenericStatus
{
  /** Virtual function for combining the status of several status
      objects. */
  virtual void operatorAndEq(const GenericStatus& o) = 0;

 public:
  /** Constructor. */
  GenericStatus();

  /** Destructor. */
  virtual ~GenericStatus();

  /** Cloning. */
  virtual GenericStatus* clone() const = 0;

  /* Combining. */
  /* virtual GenericStatus operator && (GenericStatus& o) const; */

  /** No longer virtual. */
  virtual GenericStatus& operator &= (GenericStatus& o);

  /** Clear status, neutral or something. */
  virtual void clear() = 0;

  /** Printing. */
  virtual ostream& print(ostream& os) const = 0;

  /** Equality test. */
  virtual bool operator == (const GenericStatus& o) const = 0;

  /** Return a null pointer. */
  static inline GenericStatus* null() {return NULL;}
};

DUECA_NS_END

PRINT_NS_START
inline ostream& operator << (ostream& o, const DUECA_NS::GenericStatus& cl)
{ return cl.print(o); }
PRINT_NS_END

#endif
