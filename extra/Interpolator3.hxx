/* ------------------------------------------------------------------   */
/*      item            : Interpolator3.hxx
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

#ifndef Interpolator3_hxx
#define Interpolator3_hxx

#ifdef Interpolator3_cxx
#endif
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
class InterpIndex;

template<class T, class I>
class InterpTable3;

/** Interpolator object. Objects of this class do linear interpolation
    in tables with three dimensions, of which the indices are given
    by three interpolation index objects. */
template<class T>
class Interpolator3
{
  /** First index object. */
  InterpIndex<T> index1;

  /** Second index object. */
  InterpIndex<T> index2;

  /** Third index object. */
  InterpIndex<T> index3;

  /** (remembered) indices for the three axes \{ */
  int idx1, idx2, idx3;  /// \}

  /** (remembered) fractions for the three axes \{ */
  double frac1, frac2, frac3; /// \}

public:
  /** Constructor. */
  Interpolator3(const InterpIndex<T>& index1,
                const InterpIndex<T>& index2,
                const InterpIndex<T>& index3);

  /// Destructor
  ~Interpolator3();

public:

  /** re-calculate the indices and fractions */
  void updateIndices(const T& val1, const T& val2, const T& val3);

  /** use this knowledge to read out a table. */
  const T getValue(const InterpTable3<T, InterpIndex<T> >& table) const;
};
DUECA_NS_END
#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#ifndef INCLUDE_TEMPLATE_SOURCE
#define INCLUDE_TEMPLATE_SOURCE
#endif

#if (defined(Interpolator3_cxx) && !defined(INCLUDE_TEMPLATE_SOURCE)) \
   || (defined(DO_INSTANTIATE) && defined(INCLUDE_TEMPLATE_SOURCE))
#ifndef Interpolator3_ii
#define Interpolator3_ii

#include <InterpTable3.hxx>
#include <InterpIndex.hxx>
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
Interpolator3<T>::Interpolator3(const InterpIndex<T>& index1,
                                const InterpIndex<T>& index2,
                                const InterpIndex<T>& index3) :
  index1(index1),
  index2(index2),
  index3(index3),
  idx1(0),
  idx2(0),
  idx3(0),
  frac1(0.0),
  frac2(0.0),
  frac3(0.0)
{
  //
}

template<class T>
Interpolator3<T>::~Interpolator3()
{
  //
}

template<class T>
void Interpolator3<T>::updateIndices(const T& val1,
                                     const T& val2,
                                     const T& val3)
{
  if (!index1.updateIndex(idx1, frac1, val1)) {
    if (frac1 < 0.0) frac1 = 0.0;
    if (frac1 > 1.0) frac1 = 1.0;
  }
  if (!index2.updateIndex(idx2, frac2, val2)) {
    if (frac2 < 0.0) frac2 = 0.0;
    if (frac2 > 1.0) frac2 = 1.0;
  }
  if (!index3.updateIndex(idx3, frac3, val3)) {
    if (frac3 < 0.0) frac3 = 0.0;
    if (frac3 > 1.0) frac3 = 1.0;
  }
}

template<class T>
const T Interpolator3<T>::
getValue(const InterpTable3<T, InterpIndex<T> >& table) const
{
  return (1.0 - frac1) *
    ( (1.0 - frac2) * ( (1.0 - frac3) * table.getValue(idx1, idx2, idx3) +
                        frac3 * table.getValue(idx1, idx2, idx3+1) ) +

      frac2 * ( (1.0 - frac3) * table.getValue(idx1, idx2+1, idx3) +
                frac3 * table.getValue(idx1, idx2+1, idx3+1) ))

    + frac1 *
    ( (1.0 - frac2) * ( (1.0 - frac3) * table.getValue(idx1+1, idx2, idx3) +
                        frac3 * table.getValue(idx1+1, idx2, idx3+1) ) +

      frac2 * ( (1.0 - frac3) * table.getValue(idx1+1, idx2+1, idx3) +
                frac3 * table.getValue(idx1+1, idx2+1, idx3+1) ));
}
DUECA_NS_END
#endif
#endif
