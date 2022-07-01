/* ------------------------------------------------------------------   */
/*      item            : Interpolator1.hxx
        made by         : Rene van Paassen
        date            : 010614
        category        : header file
        description     :
        changes         : 010614 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef Interpolator1_hxx
#define Interpolator1_hxx

#ifdef Interpolator1_cxx
#endif
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
class InterpIndex;

template<class T, class I>
class InterpTable1;

/** Interpolator object. Objects of this class do linear interpolation
    in tables with one dimension, of which the index is given by an
    interpolation index object. */
template<class T>
class Interpolator1
{
  /** First index object. */
  InterpIndex<T> index1;

  /** (remembered) indices for the three axes */
  int idx1;

  /** (remembered) fractions for the three axes */
  double frac1;

public:
  /** Constructor. */
  Interpolator1(const InterpIndex<T>& index1);

  /// Destructor
  ~Interpolator1();

public:

  /** re-calculate the indices and fractions */
  void updateIndices(const T& val1);

  /** use this knowledge to read out a table. */
  const T getValue(const InterpTable1<T, InterpIndex<T> >& table) const;
};
DUECA_NS_END
#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#ifndef INCLUDE_TEMPLATE_SOURCE
#define INCLUDE_TEMPLATE_SOURCE
#endif

#if (defined(Interpolator1_cxx) && !defined(INCLUDE_TEMPLATE_SOURCE)) \
   || (defined(DO_INSTANTIATE) && defined(INCLUDE_TEMPLATE_SOURCE))
#ifndef Interpolator1_ii
#define Interpolator1_ii

#include <InterpTable1.hxx>
#include <InterpIndex.hxx>
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
Interpolator1<T>::Interpolator1(const InterpIndex<T>& index1) :
  index1(index1),
  idx1(0),
  frac1(0.0)
{
  //
}

template<class T>
Interpolator1<T>::~Interpolator1()
{
  //
}

template<class T>
void Interpolator1<T>::updateIndices(const T& val1)
{
  if (!index1.updateIndex(idx1, frac1, val1)) {
    if (frac1 < 0.0) frac1 = 0.0;
    if (frac1 > 1.0) frac1 = 1.0;
  }
}

template<class T>
const T Interpolator1<T>::
getValue(const InterpTable1<T, InterpIndex<T> >& table) const
{
  return (1.0 - frac1) * table.getValue(idx1) +
    frac1 * table.getValue(idx1+1);
}
DUECA_NS_END
#endif
#endif
