/* ------------------------------------------------------------------   */
/*      item            : InterpTable4.hxx
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

#ifndef InterpTable4_hxx
#define InterpTable4_hxx

#ifdef InterpTable4_cxx
#endif
#include <dueca_ns.h>
DUECA_NS_START

/** Interpolation data holder, for 4d interpolation tables. Either
    reads data from file, or uses a static array initialisation. */
template<class T, class I>
class InterpTable4
{
  /** Node values of this first index. */
  const I index1;

  /** Node values of this second index. */
  const I index2;

  /** Node values of this third index. */
  const I index3;

  /** Node values of this fourth index. */
  const I index4;

  /** Table data, as one flat array. Stored in a row-major order, i.e.
      iterating fastest over the last dimension. */
  const T* data;

public:
  /** Constructor with an array pointer as input, by far the fastest
      constructor. */
  InterpTable4(const I& i1, const I& i2, const I& i3, const I& i4,
               const T* data);

#if 0
  /** Constructor with a file as input, flexible. */
  InterpTable4(const char* filename);
#endif

public:
  /** Access the first index, normally for getting fraction/index
      values. */
  inline const I& getIndex1() {return index1;}

  /** Access the second index, normally for getting fraction/index
      values. */
  inline const I& getIndex2() {return index2;}

  /** Access the third index, normally for getting fraction/index
      values. */
  inline const I& getIndex3() {return index3;};

  /** Access the fourth index, normally for getting fraction/index
      values. */
  inline const I& getIndex4() {return index4;};

  /** Get node values */
  inline const T& getValue(int i1, int i2, int i3, int i4) const
  {
    if (i1 < 0) i1 = 0; if (i1 > index1.maxDim()) i1 = index1.maxDim();
    if (i2 < 0) i2 = 0; if (i2 > index2.maxDim()) i2 = index2.maxDim();
    if (i3 < 0) i3 = 0; if (i3 > index3.maxDim()) i3 = index3.maxDim();
    if (i4 < 0) i4 = 0; if (i4 > index4.maxDim()) i4 = index4.maxDim();
    return data[((i1 * index2.nDim() + i2) * index3.nDim() + i3) *
               index4.nDim() + i4];
  }
};
DUECA_NS_END

#endif

#ifndef INCLUDE_TEMPLATE_SOURCE
#define INCLUDE_TEMPLATE_SOURCE
#endif

#if (defined(InterpTable4_cxx) && !defined(INCLUDE_TEMPLATE_SOURCE)) \
   || (defined(DO_INSTANTIATE) && defined(INCLUDE_TEMPLATE_SOURCE))
#ifndef InterpTable4_ii
#define InterpTable4_ii
#include <dueca_ns.h>
DUECA_NS_START

template<class T, class I>
InterpTable4<T,I>::InterpTable4(const I& i1, const I& i2,
                                const I& i3, const I& i4,
                                const T* data) :
  index1(i1),
  index2(i2),
  index3(i3),
  index4(i4),
  data(data)
{
  //
}

#if 0
template<class T, class I>
InterpTable4<T,I>::InterpTable4(const char* filename)
{
  assert(0); // not implemented
}
#endif
DUECA_NS_END

#endif
#endif
