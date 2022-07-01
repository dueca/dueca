/* ------------------------------------------------------------------   */
/*      item            : TrimLink.hxx
        made by         : Rene van Paassen
        date            : 010926
        category        : header file
        description     :
        changes         : 010926 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TrimLink_hxx
#define TrimLink_hxx

#ifdef TrimLink_cxx
#endif

#include <iostream>
#include <cmath>
using namespace std;
#include <dueca_ns.h>
DUECA_NS_START

/** This forms a link between a trim calculation variable, and the
    visual display -- and possibly manipulation -- of this variable. */
class TrimLink
{
  /** Current value of the displayed variable -- as displayed. */
  float value;

  /** If true, the user should be able to change the value. */
  bool locked;

  /** Minimum allowed value if changeable. */
  float min_accept;

  /** Maximum allowed value if changeable. */
  float max_accept;
public:

  /** Define the type of the visualization object. */
  typedef class TrimView viewer;

public:

  /** Constructor. */
  TrimLink(float value, float min_accept, float max_accept);

  /** Locking and unlocking */
  inline void lock(bool l = true) {locked = l;}

  /** Changing and getting the value. */
  void setValue(float v);

  /** Return the actual value of the variable. */
  inline float getValue() const {return value;}

  /** Returns a pointer to the object that provides the graphical
      representation. */
  static void* getTree();

  /** Print the link to stream. */
  friend ostream& operator << (ostream& os, const TrimLink& l);

  /** Equality test. */
  inline bool operator == (const TrimLink & o) const
  {return fabs(value - o.value) < 1.0E-6;}

  /** Combine two links, useless in this case. */
  inline TrimLink& operator &= (const TrimLink& o)
  {return *this;}

  /** Clear the link, also useless. */
  inline void clear()
  { }

};
DUECA_NS_END

#endif
