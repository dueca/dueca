/* ------------------------------------------------------------------   */
/*      item            : InputCalibratorRanging.hxx
        made by         : Rene van Paassen
        date            : 260709
        category        : header file
        description     :
        changes         : 260907 New, from standard calibrator
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        copyright       : (c) 2026 René van Paassen
        license         : EUPL-1.2
*/

#pragma once

#include <iostream>

namespace dueca {

template <class T, class R>
class InputCalibratorRanging;

template <class T, class R = int>
struct RangingLockReason {
  typedef InputCalibratorRanging<T,R> caltype;

  const caltype &r;

  template <typename OS> OS &print(OS &os) const {
     double _r = r.out_max - r.out_min;
  if ((r.found_max - r.found_min - _r) <= _r * r.range_tolerance) {
    os << "range matching ";
  }
  else {
    os << "range error ";
  }
  return os << r.found_min << "(" << r.out_min << ") .. " << r.found_max << "("
            << r.out_max << ") c=" << r.converter(0);
  }

  RangingLockReason(const InputCalibratorRanging<T,R> &r) : r(r) {}
};

template <class T, class R>
std::ostream &operator<<(std::ostream &os,
                         const RangingLockReason<T, R> &o)
{
  return o.print(os);
}

/** Class that performs calibration and scaling of an incoming integer
    value from an A/D converter or other input device. It is a
    templated class, and the template parameter must be a class
    capable of scaling the incoming values. */
  template <class T, class R = int>
  class InputCalibratorRanging
{
  friend class RangingLockReason<T,R>;

  /** Minimum value to be received from A/D conversion. */
  const R in_min;

  /** Maximum value to be received from A/D conversion. */
  const R in_max;

  /** Value that came in. */
  R in_coming;

  /** Function that will convert the received A/D conversion to a
      double value. */
  T converter;

public:
  /** Converted datatype */
  typedef typename T::data_type data_type;

private:
  /** Expected minimum */
  const data_type out_min;

  /** Expected maximum */
  const data_type out_max;

  /** Currently found minimum */
  data_type found_min;

  /** Currently found maximum */
  data_type found_max;

  /** Converted value. */
  data_type value;

  /** Tolerance in range matching */
  const double range_tolerance;

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
      \param out_min  Minimum output value
      \param out_max  Maximum output value
      \param idx      Optional index to be stored with the calibrator
      \param range_tolerance Relative tolerance for the error between the
                      out_min..out_max range and the range detected in
                      homing.
  */
  InputCalibratorRanging(const R in_min, const R in_max, const T &c,
                         const data_type out_min, const data_type out_max,
                         unsigned int idx = 0,
                         const double range_tolerance = 0.01);

  /** Destructor. */
  ~InputCalibratorRanging();

  /** Obtain the converted value. */
  inline operator double() const { return value; }

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

  /** Determine the calibration offset. Checks found maximum and minimum
      values, verifies that their range is within the tolerance of expected
      max and min, and if so locks the offset

      @returns true if the range within tolerance
  */
  bool lockOffset();

  /** Print calibration offset reasoning */
  RangingLockReason<T,R> lockReason() const { return RangingLockReason<T,R>(*this); }

  /** Reset for a new homing movement */
  void resetHoming()
  {
    found_min = out_max;
    found_max = out_min;
  }

  /** Print to stream, for debugging purposes */
  std::ostream &print(std::ostream &os) const;
};

template <class T, class R>
std::ostream &operator<<(std::ostream &os,
                         const InputCalibratorRanging<T, R> &o)
{
  return o.print(os);
}

template <class T, class R>
InputCalibratorRanging<T, R>::InputCalibratorRanging(
  const R in_min, const R in_max, const T &c, data_type out_min,
  data_type out_max, unsigned int idx, const double range_tolerance) :
  in_min(in_min),
  in_max(in_max),
  in_coming(0),
  converter(c),
  out_min(out_min),
  out_max(out_max),
  found_min(out_max),
  found_max(out_min),
  value((out_min + out_max) * 0.5),
  range_tolerance(range_tolerance),
  idx(idx)
{
  //
}

template <class T, class R>
InputCalibratorRanging<T, R>::~InputCalibratorRanging()
{
  //
}

template <class T, class R>
void InputCalibratorRanging<T, R>::newConversion(const R i)
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

    if (value > found_max)
      found_max = value;
    if (value < found_min)
      found_min = value;
  }
}

template <class T, class R>
R InputCalibratorRanging<T, R>::bitDifference(const double r) const
{
  double upper1 = converter(double(in_coming + 1));
  double lower1 = converter(double(in_coming - 1));
  if (upper1 > lower1) {
    if (upper1 > r && lower1 < r) {
      return 0;
    }
    if (r > upper1 && r < converter(double(in_coming + 2))) {
      return 1;
    }
    else if (r < lower1 && r > converter(double(in_coming - 2))) {
      return -1;
    }
    else if (r > upper1)
      return 2;
    return -2;
  }
  else {
    if (lower1 > r && upper1 < r) {
      return 0;
    }
    if (r > lower1 && r < converter(double(in_coming - 2))) {
      return 1;
    }
    else if (r < upper1 && r > converter(double(in_coming + 2))) {
      return -1;
    }
    else if (r > lower1)
      return 2;
    return -2;
  }
}

template <class T, class R> bool InputCalibratorRanging<T, R>::lockOffset()
{
  double r = out_max - out_min;
  if ((found_max - found_min - r) <= r * range_tolerance) {
    double new_offset = 0.5 * (out_max - found_max + out_min - found_min);
    converter.adjustOffset(new_offset);
    return true;
  }
  return false;
}

template <class T, class R>
std::ostream &InputCalibratorRanging<T, R>::print(std::ostream &os) const
{
  return os << "InputCalibratorRanging(in_min=" << in_min
            << ", in_max=" << in_max << " c=" << converter << ") " << in_coming
            << " -> " << value;
}

} // namespace dueca