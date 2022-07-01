/* ------------------------------------------------------------------   */
/*      item            : MultiStreamWriter.hxx
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

#ifndef MultiStreamWriter_hxx
#define MultiStreamWriter_hxx

#ifdef MultiStreamWriter_cxx
#endif

#include <MultiStreamWriteToken.hxx>
#include <DataWriter.hxx>
#include <dueca_ns.h>
DUECA_NS_START

/** This is a facilitator for writing multi-stream data. By creating a
    "MultiStreamWriter", the access token is used to gain access to the
    data in the channel. When at the end of scope the MultiStreamWriter is
    destroyed, the access is released again, and the actual data is
    sent.

    An example, suppose you have created a MultiStreamChannelWriteToken for
    a channel with MyData objects:

    \code
    // it is good practice to start with an opening braket. This
    // starts a new variable scope
    {

      // create the writer
      MultiStreamWriter<MyData> w(my_multi_stream_write_token, ts);

      // put the proper data in
      w.data().a = some_value;
      w.data().b = some_other_value;
      // etc. Note that you get cryptic error messages if you forget
      // that data() is a function, and type "w.data.a"

    } // <-- This closing bracket ends the "scope", in which
      // MultiStreamWriter<MyData> w was created. That means that at this
      // point the destructor is called. The destructor actually sends
      // the data over the channel.
    \endcode

    MultiStream channels work differently from Event and Stream
    channels, in that the data written there is persistent. The next
    time you create a MultiStreamWriter on a channel, the data you had
    written before is present (and readable) in the Writer. If you
    want, you can limit yourself to only writing the changes.
*/
template<class T>
class MultiStreamWriter: public DataWriter<T>
{

public:
  /** Constructor. Gains access to the channel for which the token was
      made.
      \param token  Read access token.
      \param ts     Time specification. */
  MultiStreamWriter(MultiStreamWriteToken<T>& token, const TimeSpec& ts) :
    DataWriter<T>(token, ts)
  {
    // any exceptions should be caught by client
    // token.getWriteAccess(data_ptr, ts);
  }

  /** Destructor. Releases the access again with a token. */
  ~MultiStreamWriter() { }


  /** Check that the stream writing is OK. Only applicable in code
      without exceptions. */
  inline bool ok() {
    return true;
  }

private:
  /** Copying is not possible. */
  MultiStreamWriter(const MultiStreamWriter<T>& );

  /** Nor is assignment. */
  MultiStreamWriter<T>& operator = (const MultiStreamWriter<T>& );

  /** And new is certainly forbidden! */
  static void* operator new(size_t s);
};

DUECA_NS_END

#endif
