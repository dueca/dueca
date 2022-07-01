/* ------------------------------------------------------------------   */
/*      item            : InterpTable2.hxx
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

#ifndef InterpTable2_hxx
#define InterpTable2_hxx

#ifdef InterpTable2_cxx
#endif
#include <dueca_ns.h>
DUECA_NS_START

/** Interpolation data holder, for 2d interpolation tables. Either
    reads data from file, or uses a static array initialisation. */
template<class T, class I>
class InterpTable2
{
  /** Node values of this first index. */
  const I index1;

  /** Node values of this second index. */
  const I index2;

  /** Table data, as one flat array. Stored in a row-major order, i.e.
      iterating fastest over the last dimension. */
  const T* data;

public:
  /** Constructor with an array pointer as input, by far the fastest
      constructor.
      \param i1  Index object for the first index of the table.
      \param i2  Index object for the second index of the table.
      \param data  Pointer to (permanent!) data array with the data
                 for the table. This data is laid out in row-mayor
                 order, in other words, the second index is the fast-
                 running one (e.g. d0_1, d0_2, d0_3, d1_1, d1_2, d1_3 etc.*/
  InterpTable2(const I& i1, const I& i2,
               const T* data);

#if 0
  /** Constructor with a file as input, flexible. */
  InterpTable2(const char* filename);
#endif

public:
  /** Access the first index, normally for getting fraction/index
      values. */
  inline const I& getIndex1() {return index1;}

  /** Access the second index, normally for getting fraction/index
      values. */
  inline const I& getIndex2() {return index2;}

  /** Get node values */
  inline const T& getValue(int i1, int i2) const
  {
    if (i1 < 0) i1 = 0; if (i1 > index1.maxDim()) i1 = index1.maxDim();
    if (i2 < 0) i2 = 0; if (i2 > index2.maxDim()) i2 = index2.maxDim();
    return data[i1 * index2.nDim() + i2];
  }
};
DUECA_NS_END

#endif

#ifndef INCLUDE_TEMPLATE_SOURCE
#define INCLUDE_TEMPLATE_SOURCE
#endif

#if (defined(InterpTable2_cxx) && !defined(INCLUDE_TEMPLATE_SOURCE)) \
   || (defined(DO_INSTANTIATE) && defined(INCLUDE_TEMPLATE_SOURCE))
#ifndef InterpTable2_ii
#define InterpTable2_ii
#include <dueca_ns.h>
DUECA_NS_START

template<class T, class I>
InterpTable2<T,I>::InterpTable2(const I& i1, const I& i2,
             const T* data) :
  index1(i1),
  index2(i2),
  data(data)
{
  //
}

#if 0
template<class T, class I>
InterpTable2<T,I>::InterpTable2(const char* filename)
{
  assert(0); // not implemented
}
#endif
DUECA_NS_END

#endif
#endif
