/* ------------------------------------------------------------------   */
/*      item            : StreamWriter.hxx
        made by         : Rene van Paassen
        date            : 010411
        category        : header file
        description     :
        changes         : 010411 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef StreamWriter_hxx
#define StreamWriter_hxx

#ifdef StreamWriter_cxx
#endif

#include <StreamAccessToken.hxx>
#include <DataWriter.hxx>
#include <dueca_ns.h>
DUECA_NS_START

/** This is a light weight (on the stack) object to facilitate for
    writing stream data. By creating a "StreamWriter", the access
    token is used to gain access to the data in the channel. The
    data() member of the StreamWriter can be used to write the proper
    data.  When at the end of scope the StreamWriter is destroyed, the
    actual data is sent.

    \tparam T    Data type of the channel
    \tparam init If false, the newly written variable is not
                 initialised using the default constructor. Default is
                 true. Note that, to have any positive effect from
                 this, the default values for the data type need to be
                 specified in the dco file.

    An example, suppose you have created a StreamChannelWriteToken for
    a channel with MyData objects:

    \code
    // it is good practice to start with an opening bracket. This
    // starts a new variable scope
    {

      // create the writer
      StreamWriter<MyData> w(my_stream_write_token, ts);

      // put the proper data in
      w.data().a = some_value;
      w.data().b = some_other_value;
      // etc. Note that you get cryptic error messages if you forget
      // that data() is a function, and type "w.data.a"
      // If you forget to assign a value to a member of w.data(), the
      // value there will be undefined, unless you specified the
      // default values for the constructor of the MyData type

    } // <-- This closing bracket ends the "scope", in which
      // StreamWriter<MyData> w was created. That means that at this
      // point the destructor is called. The destructor actually sends
      // the data over the channel.
    \endcode
*/
template<class T,bool init=true>
class StreamWriter: public DataWriter<T>
{
public:
  /** Constructor. Gains access to the channel for which the token was
      made.
      \param token  Read access token.
      \param ts     Time specification. */
  StreamWriter(StreamChannelWriteToken<T>& token, const TimeSpec& ts) :
    DataWriter<T>(token, ts)
  { }

  /** Destructor. Releases the access again with a token. */
  ~StreamWriter() { }

  /** Check that the stream writing is OK. Only applicable in code
      without exceptions. */
  inline bool ok() const {return true;}

private:
  /** Copying is not possible. */
  StreamWriter(const StreamWriter<T>& );

  /** Nor is assignment. */
  StreamWriter<T>& operator = (const StreamWriter<T>& );

  /** And new is certainly forbidden! */
  static void* operator new(size_t s);
};

DUECA_NS_END
#endif
