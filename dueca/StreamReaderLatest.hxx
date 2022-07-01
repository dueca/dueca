/* ------------------------------------------------------------------   */
/*      item            : StreamReaderLatest.hxx
        made by         : Rene van Paassen
        date            : 010411
        category        : header file
        description     :
        changes         : 010411 first version
                          040407 Moved access with the token to the
                          data() method, since throwing exceptions
                          from constructors can lead to undefined
                          behaviour (is the item constructed or not?)
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef StreamReaderLatest_hxx
#define StreamReaderLatest_hxx

#include <dueca_ns.h>
DUECA_NS_START

#include <StreamAccessToken.hxx>
#include <DataReader.hxx>

/** This is a "light weight object" that enables you to read the
    latest set of data on a StreamChannelEnd.

    The idea is that this object is created "on the stack" (so new
    StreamReaderLatest<YourType>(token); will not work!). The access
    token, which you must have made and validated earlier, is used to
    gain access to the data. If successfull, you can call the data()
    method to get access to the data, and the timeSpec() method to
    find out about the time stamp of this data.

    Note that it is seldom necessary to use StreamReaderLatest,
    normally you should use StreamReader.
*/
template<class T>
class StreamReaderLatest: public DataReader<T, MatchIntervalStartOrEarlier>
{

public:

  /** Constructor. Gains access to the channel for which the token was
      made.
      \param token  Read access token. */
  StreamReaderLatest(StreamChannelReadToken<T>& token) :
    DataReader<T, MatchIntervalStartOrEarlier>(token)
  {
    //
  }

  /** Second Constructor. Gains access to the channel for which the
      token was made. The TimeSpec argument is ignored. This
      constructor is only added for compatibility.
      \param token  Read access token.
      \param dum    TimeSpec, ignored. */
  StreamReaderLatest(StreamChannelReadToken<T>& token, const TimeSpec& dum) :
    DataReader<T, MatchIntervalStartOrEarlier>(token)
  {
    //
  }

  /** Destructor. Releases the access again with a token. */
  ~StreamReaderLatest() {}

  /** Check whether access can be successful. */
  inline bool ok()
  {
    try {
      DataReader<T, MatchIntervalStartOrEarlier>::data();
    }
    catch (const NoDataAvailable& e) {
      return false;
    }
    return true;
  }

private:
  /** copying is not possible. */
  StreamReaderLatest(const StreamReaderLatest<T>& );

  /** nor is assignment. */
  StreamReaderLatest<T>& operator = (const StreamReaderLatest<T>& );

  /** and new is certainly forbidden! */
  static void* operator new(size_t s);
};

DUECA_NS_END
#endif
