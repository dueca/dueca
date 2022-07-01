/* ------------------------------------------------------------------   */
/*      item            : Interpolator4.hxx
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

#ifndef Interpolator4_hxx
#define Interpolator4_hxx

#ifdef Interpolator4_cxx
#endif
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
class InterpIndex;

template<class T, class I>
class InterpTable4;

/** Interpolator object. Objects of this class do linear interpolation
    in tables with three dimensions, of which the indices are given
    by three interpolation index objects. */
template<class T>
class Interpolator4
{
  /** First index object. */
  InterpIndex<T> index1;

  /** Second index object. */
  InterpIndex<T> index2;

  /** Third index object. */
  InterpIndex<T> index3;

  /** Fourth index object. */
  InterpIndex<T> index4;

  /** (remembered) indices for the three axes \{ */
  int idx1, idx2, idx3, idx4;  /// \}

  /** (remembered) fractions for the three axes  \{ */
  double frac1, frac2, frac3, frac4;  /// \}

public:
  /** Constructor. */
  Interpolator4(const InterpIndex<T>& index1,
                const InterpIndex<T>& index2,
                const InterpIndex<T>& index3,
                const InterpIndex<T>& index4);

  /// Destructor
  ~Interpolator4();

public:

  /** re-calculate the indices and fractions */
  void updateIndices(const T& val1, const T& val2,
                     const T& val3, const T& val4);

  /** use this knowledge to read out a table. */
  const T getValue(const InterpTable4<T, InterpIndex<T> >& table) const;
};
DUECA_NS_END
#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#ifndef INCLUDE_TEMPLATE_SOURCE
#define INCLUDE_TEMPLATE_SOURCE
#endif

#if (defined(Interpolator4_cxx) && !defined(INCLUDE_TEMPLATE_SOURCE)) \
   || (defined(DO_INSTANTIATE) && defined(INCLUDE_TEMPLATE_SOURCE))
#ifndef Interpolator4_ii
#define Interpolator4_ii

#include <InterpTable4.hxx>
#include <InterpIndex.hxx>
#include <dueca_ns.h>
DUECA_NS_START

template<class T>
Interpolator4<T>::Interpolator4(const InterpIndex<T>& index1,
                                const InterpIndex<T>& index2,
                                const InterpIndex<T>& index3,
                                const InterpIndex<T>& index4) :
  index1(index1),
  index2(index2),
  index3(index3),
  index4(index4),
  idx1(0),
  idx2(0),
  idx3(0),
  idx4(0),
  frac1(0.0),
  frac2(0.0),
  frac3(0.0),
  frac4(0.0)
{
  //
}

template<class T>
Interpolator4<T>::~Interpolator4()
{
  //
}

template<class T>
void Interpolator4<T>::updateIndices(const T& val1,
                                     const T& val2,
                                     const T& val3,
                                     const T& val4)
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
  if (!index4.updateIndex(idx4, frac4, val4)) {
    if (frac4 < 0.0) frac4 = 0.0;
    if (frac4 > 1.0) frac4 = 1.0;
  }
}

template<class T>
const T Interpolator4<T>::
getValue(const InterpTable4<T, InterpIndex<T> >& table) const
{
  return
    (1.0 - frac1) *
    ( (1.0 - frac2) *
      ( (1.0 - frac3) *
        ( (1.0 - frac4) * table.getValue(idx1, idx2, idx3, idx4) +
          (      frac4) * table.getValue(idx1, idx2, idx3, idx4+1) ) +

        (      frac3) *
        ( (1.0 - frac4) * table.getValue(idx1, idx2, idx3+1, idx4) +
          (      frac4) * table.getValue(idx1, idx2, idx3+1, idx4+1) )) +
      (      frac2) *
      ( (1.0 - frac3) *
        ( (1.0 - frac4) * table.getValue(idx1, idx2+1, idx3, idx4) +
          (      frac4) * table.getValue(idx1, idx2+1, idx3, idx4+1) ) +

        (      frac3) *
        ( (1.0 - frac4) * table.getValue(idx1, idx2+1, idx3+1, idx4) +
          (      frac4) * table.getValue(idx1, idx2+1, idx3+1, idx4+1) ))) +
    (      frac1) *
    ( (1.0 - frac2) *
      ( (1.0 - frac3) *
        ( (1.0 - frac4) * table.getValue(idx1+1, idx2, idx3, idx4) +
          (      frac4) * table.getValue(idx1+1, idx2, idx3, idx4+1) ) +

        (      frac3) *
        ( (1.0 - frac4) * table.getValue(idx1+1, idx2, idx3+1, idx4) +
          (      frac4) * table.getValue(idx1+1, idx2, idx3+1, idx4+1) )) +
      (      frac2) *
      ( (1.0 - frac3) *
        ( (1.0 - frac4) * table.getValue(idx1+1, idx2+1, idx3, idx4) +
          (      frac4) * table.getValue(idx1+1, idx2+1, idx3, idx4+1) ) +

        (      frac3) *
        ( (1.0 - frac4) * table.getValue(idx1+1, idx2+1, idx3+1, idx4) +
          (      frac4) * table.getValue(idx1+1, idx2+1, idx3+1, idx4+1) )));
}
DUECA_NS_END
#endif
#endif
