/* ------------------------------------------------------------------   */
/*      item            : OutputCalibrator.hxx
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

#ifndef OutputCalibrator_hxx
#define OutputCalibrator_hxx

#include <iostream>

/** This performs a conversion of some (engineering units) value to an
    integer suitable for control of a D/A converter or other output
    device. */
template<class T, class R=int>
class OutputCalibrator
{
  /** Minimum value of the floating point variable. */
  double out_min;

  /** Maximum value of the floating point variable. */
  double out_max;

  /** Minimum value that can be sent to the D/A converter. */
  R int_min;

  /** Maximum value that can be sent to the D/A converter. */
  R int_max;

  /** Function that will convert floating point variable to an int for
      the D/A conversion. */
  T   converter;

  /** Double, unconverted, value. */
  double value;

  /** Resulting converted value */
  R out_going;

  /** Remainder, will be added to the following conversion. */
  double rem;

  /** Weighing factor for the remainder. Must be between zero and
      one. */
  double rem_weight;

  /** Index, as an additional service, e.g. to store the analog IO channel
      index. */
  unsigned int idx;

public:

  /** Constructor. Takes a converter as argument.
      \param out_min  Minimum value for the engineering units side.
      \param out_max  Maximum value for the engineering units side.
      \param int_min  Minimum value for the converted signal.
      \param int_max  Maximum value for the converted signal. Take
                      care that the int_min and int_max ranges do not
                      fall outside the range for your D/whatever
                      converter!
      \param c        Converter.
      \param rem_weight If you have an output value it can almost
                      never be converted exactly, there is a remainder
                      of less than half a bit at each conversion.
                      The rem_weight parameter determines how much
                      this remainder is counted at the next
                      conversion. With 0 it will simply not carry
                      over, with 1 it will count fully.
      \param idx      Optional index to be stored with the calibrator.
  */
  OutputCalibrator(const double out_min, const double out_max,
                   const R int_min, const R int_max, const T& c,
                   const double rem_weight = 0.0,
                   unsigned int idx = 0);

  /** Destructor. */
  ~OutputCalibrator();

  /** Obtain the converted value. */
  inline operator double () const { return value;}

  /** Obtain the raw integer value. */
  inline R raw() const { return out_going; }

  /** Assignment operator. Has the effect of taking the input value,
      and storing it for later output. */
  inline OutputCalibrator& operator = (const double x) {
    value = x;
    if (value < out_min) value = out_min;
    if (value > out_max) value = out_max;
    return *this;
  }

  /** Summation with current value. Has the effect of taking the input
      value, summing it with the present value and storing it for
      later output. */
  inline OutputCalibrator& operator += (const double x) {
    value += x;
    if (value < out_min) value = out_min;
    if (value > out_max) value = out_max;
    return *this;
  }

  /** Get a new converted value (integer) from the calibrator. */
  R newConversion();

  /** The index is a variable for programmer convenience, e.g. to remember
      where in an array of raw data the input value is stored. */
  inline unsigned int index() const { return idx; };

  /** Print to stream, for debugging purposes */
  std::ostream& print (std::ostream& os) const;
};

template <class T, class R>
std::ostream& operator << (std::ostream& os, const OutputCalibrator<T,R>& o)
{ return o.print(os); }

#endif

#if defined(OutputCalibrator_cxx) || defined(DO_INSTANTIATE)
#ifndef OutputCalibrator_ixx
#define OutputCalibrator_ixx
#include <cmath>

template <class T, class R>
OutputCalibrator<T,R>::OutputCalibrator(const double out_min,
                                        const double out_max,
                                        const R int_min, const R int_max,
                                        const T& c, const double rem_w,
                                        unsigned int idx) :
  out_min(out_min),
  out_max(out_max),
  int_min(int_min),
  int_max(int_max),
  converter(c),
  value(0.0),
  out_going(0),
  rem(0.0),
  rem_weight(rem_w),
  idx(idx)
{
  if (rem_weight < 0.0) rem_weight = 0.0;
  if (rem_weight > 1.0) rem_weight = 1.0;
}

template <class T, class R>
OutputCalibrator<T,R>::~OutputCalibrator()
{
  //
}

template <class T, class R>
R OutputCalibrator<T,R>::newConversion()
{
  // output, not truncated
  double out = converter(value) + rem_weight*rem;

  // new remainder, and integer output value
  rem = out - rint(out);
  out_going = R(rint(out));

  // limit the integer output value
  if (out_going < int_min) return int_min;
  if (out_going > int_max) return int_max;

  // return the conversion
  return out_going;
}


template <class T, class R>
std::ostream& OutputCalibrator<T,R>::print(std::ostream& os) const
{
  return os << "OutputCalibrator(out_min=" << out_min
            << ", out_max=" << out_max
            << ", int_min=" << int_min
            << ", int_max=" << int_max
            << ", c=" << converter
            << ", rem_w=" << rem_weight << ") "
            << value << " -> " << out_going;
}

#endif
#endif
