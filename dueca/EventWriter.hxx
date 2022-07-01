/* ------------------------------------------------------------------   */
/*      item            : EventWriter.hxx
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

#ifndef EventWriter_hxx
#define EventWriter_hxx

#include <EventAccessToken.hxx>
#include <DataWriter.hxx>
#include <SimTime.hxx>
#include <dueca_ns.h>
DUECA_NS_START

/** This is a facilitator for writing event data. By creating a
    "EventWriter", an event data object is created.
    When at the end of scope the EventWriter is
    destroyed, the data object is sent over the channel.

    \tparam Data type of the event channel, normally a dco (DUECA
    Comunication Object), generated from a .dco file.

    An example, suppose you have created an EventChannelWriteToken for
    a channel with MyData objects:

    \code
    if (have_to_write_event) {

      // create the writer
      EventWriter<MyData> w(my_event_write_token, ts);

      // put the proper data in
      w.data().a = some_value;
      w.data().b = some_other_value;
      // etc. Note that you get cryptic error messages if you forget
      // that data() is a function, and type "w.data.a"

      // Another note: unless you specify defaults for the data in
      // your MyData.dco file, the stuff that is in the MyData event
      // is initially undefined. So do not forget to assign values to
      // all members of your event.

    } // <-- This closing bracket ends the "scope", in which
      // EventWriter<MyData> w was created. That means that at this
      // point the EventWriter<MyData> destructor is called. The
      // destructor actually sends the data over the channel.
    \endcode
 */
template<class T>
class EventWriter: public DataWriter<T>
{
public:
  /** Constructor. Gains access to the channel for which the token was
      made.
      \param token  Read access token.
      \param ts     Time specification.
                    If not supplied, the current time will be taken. */
  EventWriter(EventChannelWriteToken<T>& token,
              const DataTimeSpec& ts = DataTimeSpec::now());

  /** Constructor. Gains access to the channel for which the token was
      made, one variable-size array.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array. */
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1);

  /** Constructor. Gains access to the channel for which the token was
      made, two variable-size arrays.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array.
      \param n2     Size of the second variable-size array.*/
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1, unsigned int n2);

  /** Constructor. Gains access to the channel for which the token was
      made, three variable-size arrays.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array.
      \param n2     Size of the second variable-size array.
      \param n3     Size of the third variable-size array.*/
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1, unsigned int n2, unsigned int n3);

  /** Constructor. Gains access to the channel for which the token was
      made, four variable-size arrays.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array.
      \param n2     Size of the second variable-size array.
      \param n3     Size of the third variable-size array
      \param n4     Size of the fourth variable-size array.*/
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1, unsigned int n2, unsigned int n3,
              unsigned int n4);

  /** Constructor. Gains access to the channel for which the token was
      made, five variable-size arrays.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array.
      \param n2     Size of the second variable-size array.
      \param n3     Size of the third variable-size array
      \param n4     Size of the fourth variable-size array
      \param n5     Size of the fifth variable-size array.*/
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1, unsigned int n2, unsigned int n3,
              unsigned int n4, unsigned int n5);

  /** Constructor. Gains access to the channel for which the token was
      made, six variable-size arrays.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array.
      \param n2     Size of the second variable-size array.
      \param n3     Size of the third variable-size array
      \param n4     Size of the fourth variable-size array
      \param n5     Size of the fifth variable-size array
      \param n6     Size of the sixth variable-size array.*/
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1, unsigned int n2, unsigned int n3,
              unsigned int n4, unsigned int n5, unsigned int n6);

  /** Constructor. Gains access to the channel for which the token was
      made, seven variable-size arrays.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array.
      \param n2     Size of the second variable-size array.
      \param n3     Size of the third variable-size array
      \param n4     Size of the fourth variable-size array
      \param n5     Size of the fifth variable-size array
      \param n6     Size of the sixth variable-size array
      \param n7     Size of the seventh variable-size array.*/
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1, unsigned int n2, unsigned int n3,
              unsigned int n4, unsigned int n5, unsigned int n6,
              unsigned int n7);

  /** Constructor. Gains access to the channel for which the token was
      made, eight variable-size arrays.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array.
      \param n2     Size of the second variable-size array.
      \param n3     Size of the third variable-size array
      \param n4     Size of the fourth variable-size array
      \param n5     Size of the fifth variable-size array
      \param n6     Size of the sixth variable-size array
      \param n7     Size of the seventh variable-size array
      \param n8     Size of the eighth variable-size array.*/
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1, unsigned int n2, unsigned int n3,
              unsigned int n4, unsigned int n5, unsigned int n6,
              unsigned int n7, unsigned int n8);

   /** Constructor. Gains access to the channel for which the token was
      made, nine variable-size arrays.
      \param token  Read access token.
      \param ts     Time specification.
      \param n1     Size of the variable-size array.
      \param n2     Size of the second variable-size array.
      \param n3     Size of the third variable-size array
      \param n4     Size of the fourth variable-size array
      \param n5     Size of the fifth variable-size array
      \param n6     Size of the sixth variable-size array
      \param n7     Size of the seventh variable-size array
      \param n8     Size of the eighth variable-size array
      \param n9     Size of the ninth variable-size array.*/
  EventWriter(EventChannelWriteToken<T>& token, const DataTimeSpec& ts,
              unsigned int n1, unsigned int n2, unsigned int n3,
              unsigned int n4, unsigned int n5, unsigned int n6,
              unsigned int n7, unsigned int n8, unsigned int n9);

  /** Destructor. Releases the access again with a token. This
      initiates the actual sending of the data. */
  ~EventWriter() { }

private:
  /// Copying is not possible.
  EventWriter(const EventWriter<T>& );

  /// Nor is assignment.
  EventWriter<T>& operator = (const EventWriter<T>& );

  /// And new is certainly forbidden!
  static void* operator new(size_t s);
};

// partial specialisation, no arguments
template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts) :
  DataWriter<T>(token, ts)
{ }

// partial specialisation, one argument
template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1) :
  DataWriter<T>(token, ts, n1)
{ }

template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1,
                            unsigned int n2) :
  DataWriter<T>(token, ts, n1, n2)
{ }

template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1,
                            unsigned int n2,
                            unsigned int n3) :
  DataWriter<T>(token, ts, n1, n2, n3)
{ }

template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1,
                            unsigned int n2,
                            unsigned int n3,
                            unsigned int n4) :
  DataWriter<T>(token, ts, n1, n2, n3, n4)
{ }
#if 0
template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1,
                            unsigned int n2,
                            unsigned int n3,
                            unsigned int n4,
                            unsigned int n5) :
  token(token),
  data_ptr(new T(n1, n2, n3, n4, n5)),
  ts(ts)
{ }

template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1,
                            unsigned int n2,
                            unsigned int n3,
                            unsigned int n4,
                            unsigned int n5,
                            unsigned int n6) :
  token(token),
  data_ptr(new T(n1, n2, n3, n4, n5, n6)),
  ts(ts)
{ }

template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1,
                            unsigned int n2,
                            unsigned int n3,
                            unsigned int n4,
                            unsigned int n5,
                            unsigned int n6,
                            unsigned int n7) :
  token(token),
  data_ptr(new T(n1, n2, n3, n4, n5, n6, n7)),
  ts(ts)
{ }

template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1,
                            unsigned int n2,
                            unsigned int n3,
                            unsigned int n4,
                            unsigned int n5,
                            unsigned int n6,
                            unsigned int n7,
                            unsigned int n8) :
  token(token),
  data_ptr(new T(n1, n2, n3, n4, n5, n6, n7, n8)),
  ts(ts)
{ }

template<class T>
EventWriter<T>::EventWriter(EventChannelWriteToken<T>& token,
                            const DataTimeSpec& ts,
                            unsigned int n1,
                            unsigned int n2,
                            unsigned int n3,
                            unsigned int n4,
                            unsigned int n5,
                            unsigned int n6,
                            unsigned int n7,
                            unsigned int n8,
                            unsigned int n9) :
  token(token),
  data_ptr(new T(n1, n2, n3, n4, n5, n6, n7, n8, n9)),
  ts(ts)
{ }
#endif
DUECA_NS_END
#endif
