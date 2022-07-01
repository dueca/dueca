/* ------------------------------------------------------------------   */
/*      item            : MultiStreamReader.hxx
        made by         : Rene van Paassen
        date            : 041105
        category        : header file
        description     :
        changes         : 041105 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef MultiStreamReader_hxx
#define MultiStreamReader_hxx

#ifdef MultiStreamReader_cxx
#endif


#include <MultiStreamReadToken.hxx>
#include <DataReader.hxx>
#include <dueca_ns.h>
DUECA_NS_START

#ifndef NoExtrapolation_defined
#define NoExtrapolation_defined
/** Example extrapolation (actually no-extrapolation) class. Default
    class for the MultiStreamReader. You can use this as a template to
    create actual extrapolation classes. */
template<class T>
class NoExtrapolation
{
public:
  /** extrapolate the data object data, which became valid for a time
      data_time, to a time in the future or past wish_time. This one
      actually does nothing but return the unchanged data object.
      \param data      Object of extrapolation.
      \param data_time Time associated with the object, in integer ticks.
      \param wish_time Time that an extrapolated object should be
                       valid for.
      \returns         A reference to the extrapolated data. */
  inline const T& warp(const T& data, const TimeTickType data_time,
                       const TimeTickType wish_time)
  { return data; }
};
#endif

/** This is a facilitator for reading stream data. By creating a
    "MultiStreamReader", the access token is used to gain access to the
    data in the channel. When at the end of scope the MultiStreamReader is
    destroyed, the access is released again.
    \param T        Type of data that is used in the channel.
    \param E        Extrapolator, may perform internal integration to
                    obtain an estimate of the data at a different
                    time. The default performs no extrapolation. */
template<class T, template<class> class E = MatchIntervalStartOrEarlier>
class MultiStreamReader:
  public DataReader<T, E >
{
public:

  /** Constructor. Gains access to the channel for which the token was
      made. The data returned is valid at the start of the time
      specification, or, if no data is available yet, the latest data
      in the channel.
      \param token  Read access token.
      \param ts     Time specification. */
  MultiStreamReader(MultiStreamReadToken<T>& token, const TimeSpec& ts) :
    DataReader<T, E >(token, ts.getValidityStart())
  { }

  /** Destructor. Releases the access again with a token. */
  ~MultiStreamReader() { }

  /** Check that the stream reading is OK. Only applicable in code
      without exceptions. */
  inline bool ok()
  {
    try {
      DataReader<T, E>::data();
    }
    catch (const NoDataAvailable& e) {
      return false;
    }
    return true;
  }

private:
  /** copying is not possible. */
  MultiStreamReader(const MultiStreamReader<T>& );

  /** nor is assignment. */
  MultiStreamReader<T>& operator = (const MultiStreamReader<T>& );

  /** and new is certainly forbidden! */
  static void* operator new(size_t s);
};

/* \example MultiStreamReaderExample.cxx
    Here is an example of how to use stream readers. */

DUECA_NS_END
#endif

