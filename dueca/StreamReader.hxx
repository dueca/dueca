/* ------------------------------------------------------------------   */
/*      item            : StreamReader.hxx
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

#ifndef StreamReader_hxx
#define StreamReader_hxx

#ifdef StreamReader_cxx
#endif

#include <StreamAccessToken.hxx>
#include <DataReader.hxx>
#include <dueca_ns.h>

DUECA_NS_START

/** This is a facilitator for reading stream data. By creating a
    "StreamReader", the access token is used to gain access to the
    data in the channel. When at the end of scope the StreamReader is
    destroyed, the access is released again. */
template<class T>
class StreamReader: public DataReader<T, MatchIntervalStart>
{

public:

  /** Constructor. Gains access to the channel for which the token was
      made.
      \param token  Read access token.
      \param ts     Time specification. */
  StreamReader(StreamChannelReadToken<T>& token, const TimeSpec& ts) :
    DataReader<T, MatchIntervalStart>(token, ts)
  {
    //
  }

  /** Destructor. Releases the access again with a token. */
  ~StreamReader() {}

  /** Check that the stream reading is OK. Only applicable in code
      without exceptions. */
  inline bool ok()
  {
    try {
      DataReader<T, MatchIntervalStart>::data();
    }
    catch (const NoDataAvailable& e) {
      return false;
    }
    return true;
  }

private:
  /** copying is not possible. */
  StreamReader(const StreamReader<T>& );

  /** nor is assignment. */
  StreamReader<T>& operator = (const StreamReader<T>& );

  /** and new is certainly forbidden! */
  static void* operator new(size_t s);
};

/** \example StreamReaderExample.cxx
    Here is an example of how to use stream readers. */

DUECA_NS_END
#endif
