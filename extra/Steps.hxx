/* ------------------------------------------------------------------   */
/*      item            : Steps.hxx
        made by         : Rene van Paassen
        date            : 030228
        category        : header file
        description     :
        documentation   : DUECA_API
        changes         : 030228 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Steps_hxx
#define Steps_hxx

#ifdef Steps_cxx
#endif

#include <dueca_ns.h>
#include <iostream>

DUECA_NS_START

/** Implementation of a scaling/converting device, that produces a
    discrete number of different output values depending on a
    continous input. This can be used to e.g. implement IO signals for
    a flap handle. */
template<int n>
class Steps
{
  /** Array of output values. */
  double y[n];

  /** Array of input/measured value transition points. */
  double tp[n-1];

public:
  /** Constructor.
      \param yi    Set of possible output values for this device.
      \param ui    Input values corresponding to the output
                   values. */
  Steps(double yi[n], double ui[n]);

  /** Destructor. */
  ~Steps();

  /** The operator. */
  double operator () (const double x) const;

  /** Print the object */
  std::ostream& print (std::ostream& os) const;
};

DUECA_NS_END

template<int n>
inline std::ostream& operator << (std::ostream& os, const DUECA_NS::Steps<n>& o)
{ return o.print(os); }

#if defined(Steps_cxx) || defined(DO_INSTANTIATE)
#ifndef Steps_ixx
#define Steps_ixx

#include <float.h>

DUECA_NS_START

/** An auxiliary function for Steps. */
void Steps_sort_and_copy(double *ui, double *yi,
                         double *tp, double *y, int n);

template<int n>
Steps<n>::Steps(double yi[n], double ui[n])
{
  Steps_sort_and_copy(ui, yi, tp, y, n);
}


template<int n>
Steps<n>::~Steps()
{
  //
}

template<int n>
double Steps<n>::operator () (const double x) const
{
  for (int ii = n - 1; ii--; ) {
    if (x > tp[ii]) return y[ii+1];
  }
  return y[0];
}


template<int n>
std::ostream& Steps<n>::print(std::ostream& os) const
{
  os << "Steps<" << n << ">(y:" << y[0];
  for (int ii = 1; ii < n; ii++) {
      os << " <" << tp[ii-1] << "> " << y[ii];
  }
  return os << ")";
}



DUECA_NS_END



#endif
#endif

#endif
