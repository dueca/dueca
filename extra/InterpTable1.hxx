/* ------------------------------------------------------------------   */
/*      item            : InterpTable1.hxx
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

#ifndef InterpTable1_hxx
#define InterpTable1_hxx

#ifdef InterpTable1_cxx
#endif
#include <dueca_ns.h>
DUECA_NS_START

/** Interpolation data holder, for 1d interpolation tables. Either
    reads data from file, or uses a static array initialisation. */
template<class T, class I>
class InterpTable1
{
  /** Node values of this first index. */
  const I index1;

  /** Table data, as one flat array. Stored in a row-major order, i.e.
      iterating fastest over the last dimension. Of course, for a 1 d
      array this does not really matter! */
  const T* data;

public:
  /** Constructor with an array pointer as input, by far the fastest
      constructor. */
  InterpTable1(const I& i1,
               const T* data);

#if 0
  /** Constructor with a file as input, flexible. */
  InterpTable1(const char* filename);
#endif

public:
  /** Access the first index, normally for getting fraction/index
      values. */
  inline const I& getIndex1() {return index1;}

  /** Get node values */
  inline const T& getValue(int i1) const
  {
    if (i1 < 0) i1 = 0; if (i1 > index1.maxDim()) i1 = index1.maxDim();
    return data[i1];
  }
};
DUECA_NS_END

#endif

#ifndef INCLUDE_TEMPLATE_SOURCE
#define INCLUDE_TEMPLATE_SOURCE
#endif

#if (defined(InterpTable1_cxx) && !defined(INCLUDE_TEMPLATE_SOURCE)) \
   || (defined(DO_INSTANTIATE) && defined(INCLUDE_TEMPLATE_SOURCE))
#ifndef InterpTable1_ii
#define InterpTable1_ii
#include <dueca_ns.h>
DUECA_NS_START

template<class T, class I>
InterpTable1<T,I>::InterpTable1(const I& i1,
                                const T* data) :
  index1(i1),
  data(data)
{
  //
}

#if 0
template<class T, class I>
InterpTable1<T,I>::InterpTable1(const char* filename)
{
  assert(0); // not implemented
}
#endif
DUECA_NS_END

#endif
#endif
