/* ------------------------------------------------------------------   */
/*      item            : InputCalibrator.hxx
        made by         : Rene van Paassen
        date            : 020429
        category        : header file
        description     :
        changes         : 020429 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef InputCalibrator_hxx
#define InputCalibrator_hxx

#ifdef InputCalibrator_cxx
#endif

#include <iostream>

/** Class that performs calibration and scaling of an incoming integer
    value from an A/D converter or other input device. It is a
    templated class, and the template parameter must be a class
    capable of scaling the incoming values. */
template<class T, class R=int>
class InputCalibrator
{
  /** Minimum value to be received from A/D conversion. */
  R in_min;

  /** Maximum value to be received from A/D conversion. */
  R in_max;

  /** Value that came in. */
  R in_coming;

  /** Function that will convert the received A/D conversion to a
      double value. */
  T   converter;

  /** Double, converted, value. */
  double value;

  /** Index, as an additional service, e.g. to store the analog IO channel
      index. */
  unsigned int idx;

public:

  /** Constructor. Takes a converter as argument.
      \param in_min   Minimum integer value
      \param in_max   Maximum integer value
      \param c        Converter of (template) type T, the operation
                      operator() (const double x) should exist for
                      values between in_min and in_max, and
                      should produce the required converted/scaled
                      value.
      \param idx      Optional index to be stored with the calibrator. */
  InputCalibrator(const R in_min, const R in_max, const T& c,
                  unsigned int idx = 0);

  /** Destructor. */
  ~InputCalibrator();

  /** Obtain the converted value. */
  inline operator double () const { return value;}

  /** Obtain the raw integer value. */
  inline R raw() const { return in_coming; }

  /** Insert a new converted value into the calibrator. */
  void newConversion(const R i);

  /** Return the difference between the given value and the measured
      value in bit domain.
      \param r      Value for comparison
      \returns      <ul> <li> 0 if the difference is 1 bit or less
                    <li> 1 if the value is two bits larger
                    <li> -1 if the value is two bits smaller
                    <li> 2 respectively -2 if the value is three bits
                    or more larger or smaller.
                    </ul> */
  R bitDifference(const double r) const;

  /** The index is a variable for user convenience, e.g. to remember
      where in an array of raw data the input value is stored. */
  inline unsigned int index() const { return idx; };

  /** Print to stream, for debugging purposes */
  std::ostream& print (std::ostream& os) const;
};

template <class T, class R>
std::ostream& operator << (std::ostream& os, const InputCalibrator<T,R>& o)
{ return o.print(os); }


#endif

#if defined(InputCalibrator_cxx) || defined(DO_INSTANTIATE)
#ifndef InputCalibrator_ixx
#define InputCalibrator_ixx

template <class T, class R>
InputCalibrator<T,R>::InputCalibrator(const R in_min, const R in_max,
                                      const T& c, unsigned int idx) :
  in_min(in_min),
  in_max(in_max),
  in_coming(0),
  converter(c),
  value(converter(double(in_min+in_max)*0.5)),
  idx(idx)
{
  //
}

template <class T, class R>
InputCalibrator<T,R>::~InputCalibrator()
{
  //
}

template <class T, class R>
void InputCalibrator<T,R>::newConversion(const R i)
{
  // store the int that came in
  in_coming = i;

  if (i > in_max) {
    value = converter(double(in_max));
  }
  else if (i < in_min) {
    value = converter(double(in_min));
  }
  else {
    value = converter(double(i));
  }
}

template <class T, class R>
R InputCalibrator<T,R>::bitDifference(const double r) const
{
  double upper1 = converter(double(in_coming+1));
  double lower1 = converter(double(in_coming-1));
  if (upper1 > lower1) {
    if (upper1 > r && lower1 < r){
      return 0;
    }
    if (r > upper1 && r < converter(double(in_coming+2))) {
      return 1;
    }
    else if (r < lower1 && r > converter(double(in_coming-2))) {
      return -1;
    }
    else if (r > upper1)
      return 2;
    return -2;
  }
  else {
    if (lower1 > r && upper1 < r){
      return 0;
    }
    if (r > lower1 && r < converter(double(in_coming-2))) {
      return 1;
    }
    else if (r < upper1 && r > converter(double(in_coming+2))) {
      return -1;
    }
    else if (r > lower1)
      return 2;
    return -2;
  }
}

template <class T, class R>
std::ostream& InputCalibrator<T,R>::print(std::ostream& os) const
{
  return os << "InputCalibrator(in_min=" << in_min << ", in_max=" << in_max
            << " c=" << converter << ") " << in_coming << " -> " << value;
}

#endif
#endif
