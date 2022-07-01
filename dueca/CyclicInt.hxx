/* ------------------------------------------------------------------   */
/*      item            : CyclicInt.hxx
        made by         : Rene' van Paassen
        date            : 990817
        category        : header file
        description     :
        changes         : 990817 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CyclicInt_hh
#define CyclicInt_hh

#include <iostream>
using namespace std;

#include <dueca_ns.h>
DUECA_NS_START

/** An integer value with a maximum limit, that, when increased to or
    beyond this limit, wraps back. */
class CyclicInt
{
  /** Current value. */
  int value;

  /** Maximum limit. */
  int limit;
public:

  /** Constructor.
      \param value  Initial value.
      \param limit  Limit for the value. */
  CyclicInt(int value, int limit);

  /** Destructor. */
  ~CyclicInt();

  /** Copy constructor. */
  CyclicInt(const CyclicInt& o);

  /** Change the limit. */
  void setLimit(int newlimit);

  /** Assign an integer value. */
  CyclicInt& operator=(int d);

  /** Increase the value. */
  CyclicInt& operator ++ ();

  /** Decrease the value. */
  CyclicInt& operator -- ();

  /** Increase with an integer. */
  CyclicInt operator+(int d);

  /** Decrease with an integer. */
  CyclicInt operator-(int d);

  /** Increase with an integer. */
  CyclicInt& operator += (int d);

  /** Test for equality. */
  bool operator == (int d) const;

  /** Test for equality, with integer as first argument. */
  friend bool operator == (int d, const CyclicInt& a);

  /** Test for inequality. */
  bool operator != (int d) const;

  /** Test for inequality, with integer as first argument. */
  friend bool operator != (int d, const CyclicInt& a);

  /** Print to stream. */
  std::ostream& print(std::ostream& o) const;

  /** Convert to integer. */
  inline operator int() const {return value;}

  /** Obtain the current limit. */
  inline int getLimit() const {return limit;}
};

DUECA_NS_END

PRINT_NS_START
inline ostream& operator << (ostream& o, const DUECA_NS::CyclicInt& cl)
{ return cl.print(o); }
PRINT_NS_END

#endif
