/* ------------------------------------------------------------------   */
/*      item            : Interpolator2.hxx
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

#ifndef Interpolator2_hxx
#define Interpolator2_hxx

#ifdef Interpolator2_cxx
#endif
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
class InterpIndex;

template<class T, class I>
class InterpTable2;

/** Interpolator object. Objects of this class do linear interpolation
    in tables with three dimensions, of which the indices are given
    by three interpolation index objects. */
template<class T>
class Interpolator2
{
  /** First index object. */
  InterpIndex<T> index1;

  /** Second index object. */
  InterpIndex<T> index2;

  /** (remembered) indices for the three axes \{ */
  int idx1, idx2; /// \}

  /** (remembered) fractions for the three axes \{ */
  double frac1, frac2;  /// \}

public:
  /** Constructor. */
  Interpolator2(const InterpIndex<T>& index1,
                const InterpIndex<T>& index2);

  /// Destructor
  ~Interpolator2();

public:

  /** re-calculate the indices and fractions */
  void updateIndices(const T& val1, const T& val2);

  /** use this knowledge to read out a table. */
  const T getValue(const InterpTable2<T, InterpIndex<T> >& table) const;
};
DUECA_NS_END
#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#ifndef INCLUDE_TEMPLATE_SOURCE
#define INCLUDE_TEMPLATE_SOURCE
#endif

#if (defined(Interpolator2_cxx) && !defined(INCLUDE_TEMPLATE_SOURCE)) \
   || (defined(DO_INSTANTIATE) && defined(INCLUDE_TEMPLATE_SOURCE))
#ifndef Interpolator2_ii
#define Interpolator2_ii

#include <InterpTable2.hxx>
#include <InterpIndex.hxx>
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
Interpolator2<T>::Interpolator2(const InterpIndex<T>& index1,
                                const InterpIndex<T>& index2) :
  index1(index1),
  index2(index2),
  idx1(0),
  idx2(0),
  frac1(0.0),
  frac2(0.0)
{
  //
}

template<class T>
Interpolator2<T>::~Interpolator2()
{
  //
}

template<class T>
void Interpolator2<T>::updateIndices(const T& val1,
                                     const T& val2)
{
  if (!index1.updateIndex(idx1, frac1, val1)) {
    if (frac1 < 0.0) frac1 = 0.0;
    if (frac1 > 1.0) frac1 = 1.0;
  }
  if (!index2.updateIndex(idx2, frac2, val2)) {
    if (frac2 < 0.0) frac2 = 0.0;
    if (frac2 > 1.0) frac2 = 1.0;
  }
}

template<class T>
const T Interpolator2<T>::
getValue(const InterpTable2<T, InterpIndex<T> >& table) const
{
  return
    (1.0 - frac1) * ( (1.0 - frac2) * table.getValue(idx1, idx2) +
                      frac2 * table.getValue(idx1, idx2+1) ) +

    (      frac1) * ( (1.0 - frac2) * table.getValue(idx1+1, idx2) +
                      frac2 * table.getValue(idx1+1, idx2+1) );

}
DUECA_NS_END
#endif
#endif
