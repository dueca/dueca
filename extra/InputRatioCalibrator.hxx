/* ------------------------------------------------------------------   */
/*      item            : InputRatioCalibrator.hxx
        made by         : Rene van Paassen
        date            : 020429
        category        : header file
        description     :
        changes         : 150725 Derived from InputCalibrator, uses
                          ratio between to A/D converted signals, rather
                          than a single value.
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef InputRatioCalibrator_hxx
#define InputRatioCalibrator_hxx

#include <iostream>

/** Class that performs calibration and scaling of an incoming integer
    value from an A/D converter or other input device. It is a
    templated class, and the template parameter must be a class
    capable of scaling the incoming values. */
template<class T, class R=int>
class InputRatioCalibrator
{
  /** Minimum value to be received from A/D conversion. */
  R in_min;

  /** Maximum value to be received from A/D conversion. */
  R in_max;

  /** Zero value for the reference A/D conversion */
  R in_zero;

  /** Value that came in. */
  R in_coming;

  /** Value of the reference */
  R in_reference;

  /** Function that will convert the received A/D conversion to a
      double value. */
  T   converter;

  /** Double, converted, value. */
  double value;

  /** Index, as an additional service, e.g. to store the analog IO channel
      index. */
  unsigned int idx;

  /** Index of the reference signal. */
  unsigned int idx_ref;

public:

  /** Constructor. Takes a converter as argument.
      \param in_min   Minimum integer value
      \param in_max   Maximum integer value
      \param in_zero  Zero value for the input and reference
      \param c        Converter of (template) type T, the operation
                      operator() (const double x) should exist for
                      values between in_min and in_max, and
                      should produce the required converted/scaled
                      value.
      \param idx      Optional index to be stored with the calibrator.
      \param idx_ref  Index of the reference signal.
  */
  InputRatioCalibrator(const R in_min, const R in_max, const R in_zero,
                       const T& c, unsigned int idx = 0, unsigned idx_ref = 0);

  /** Destructor. */
  ~InputRatioCalibrator();

  /** Obtain the converted value. */
  inline operator double () const { return value;}

  /** Obtain the raw integer value. */
  inline R raw() const { return in_coming; }

  /** Obtain the raw integer value for the reference. */
  inline R rawReference() const { return in_coming; }

  /** Insert a new converted value set into the calibrator. */
  void newConversion(const R i, const R ref);

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
      where in an array of raw data the input value is stored.
      @returns index value */
  inline unsigned int index() const { return idx; }

  /** The index of the reference signal, for user convenience.
      @returns index value */
  inline unsigned int indexReference() const { return idx_ref; }

  /** Print to stream, for debugging purposes */
  std::ostream& print (std::ostream& os) const;
};

template <class T, class R>
std::ostream& operator << (std::ostream& os, const InputRatioCalibrator<T,R>& o)
{ return o.print(os); }


#endif

#if defined(InputRatioCalibrator_cxx) || defined(DO_INSTANTIATE)
#ifndef InputRatioCalibrator_ixx
#define InputRatioCalibrator_ixx
#include <cmath>

template <class T, class R> InputRatioCalibrator<T,R>::
InputRatioCalibrator(const R in_min, const R in_max, const R in_zero,
                     const T& c, unsigned int idx, unsigned idx_ref) :
  in_min(in_min),
  in_max(in_max),
  in_zero(in_zero),
  in_coming(0),
  in_reference(1),
  converter(c),
  value(converter(double(in_min+in_max)*0.5)),
  idx(idx)
{
  //
}

template <class T, class R>
InputRatioCalibrator<T,R>::~InputRatioCalibrator()
{
  //
}

template <class T, class R>
void InputRatioCalibrator<T,R>::newConversion(const R i, const R ref)
{
  // store the ints that came in
  in_coming = i;
  in_reference = ref;
  R in_clip = i < in_min ? in_min : (i > in_max ? in_max : i);
  R ref_clip = ref < in_min ? in_min : (ref > in_max ? in_max : ref);

  if (ref_clip == 0) {
    value = INFINITY;
  }
  else {
    value = converter(double(in_clip - in_zero)/double(ref_clip - in_zero));
  }
}

template <class T, class R>
R InputRatioCalibrator<T,R>::bitDifference(const double r) const
{
  R in_clip = in_coming < in_min ? in_min :
    (in_coming > in_max ? in_max : in_coming);
  R ref_clip = in_reference < in_min ? in_min :
    (in_reference > in_max ? in_max : in_reference);

  if (ref_clip == 0) {
    return 0;
  }

  double upper1 = converter(double(in_clip+1-in_zero)/double(ref_clip-in_zero));
  double lower1 = converter(double(in_clip-1-in_zero)/double(ref_clip-in_zero));
  if (upper1 > lower1) {
    if (upper1 > r && lower1 < r){
      return 0;
    }
    if (r > upper1 &&
        r < converter(double(in_clip+2-in_zero)/double(ref_clip-in_zero))) {
      return 1;
    }
    if (r < lower1 &&
        r > converter(double(in_clip-2-in_zero)/double(ref_clip-in_zero))) {
      return -1;
    }
    if (r > upper1) {
      return 2;
    }
    return -2;
  }
  else {
    if (lower1 > r && upper1 < r){
      return 0;
    }
    if (r > lower1 &&
        r < converter(double(in_clip-2-in_zero)/double(ref_clip-in_zero))) {
      return 1;
    }
    if (r < upper1 &&
        r > converter(double(in_clip+2-in_zero)/double(ref_clip-in_zero))) {
      return -1;
    }
    if (r > lower1) {
      return 2;
    }
    return -2;
  }
}

template <class T, class R>
std::ostream& InputRatioCalibrator<T,R>::print(std::ostream& os) const
{
  return os << "InputRatioCalibrator(in_min=" << in_min << ", in_max=" << in_max
            << ", c=" << converter << ") "
            << in_coming << '/' << in_reference << " -> " << value;
}

#endif
#endif
