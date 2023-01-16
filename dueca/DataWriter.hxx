/* ------------------------------------------------------------------   */
/*      item            : DataWriter.hxx
        made by         : Rene van Paassen
        date            : 140106
        category        : header file
        description     :
        changes         : 140106 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataWriter_hxx
#define DataWriter_hxx
#include <dueca_ns.h>
#include "SimTime.hxx"
#include "DataWriterArraySize.hxx"
#include "ChannelWriteToken.hxx"

DUECA_NS_START

/** Common base class for DataWriter instantiations. */
class DataWriterBase
{
protected:
  /** Access to the channel. */
  ChannelWriteToken &token;

  /** Time specification to be used. */
  DataTimeSpec ts;

protected:
  /** Constructor

      @param token  Token of the channel to be written to
      @param ts     Time for writing
  */
  DataWriterBase(ChannelWriteToken& token, const DataTimeSpec& ts) :
    token(token), ts(ts) { }

protected:
  /** Releasing the read access means that the data will be made
      accessible for reading

      @param data_ptr    Pointer for passing the data
  */
  inline void releaseAccess(void* data_ptr)
  { token.releaseAccess(data_ptr, ts); }

  /** Verify the magic number and validity for access.

      @param magic       magic number, must match the data type magic number
      @throws            InvalidChannelAccessReturn, if the magic is
                         not correct or the token is not valid.
  */
  inline void baseCheckAccess(uint32_t magic)
  { token.checkAccess(magic); }
};

/** This is a facilitator for writing multi-stream data. By creating a
    "DataWriter", the access token is used to gain access to the
    data in the channel. When at the end of scope the DataWriter is
    destroyed, the access is released again, and the actual data is
    sent.

    An example, suppose you have created a ChannelWriteToken for
    a channel with MyData objects:

    \code
    // it is good practice to start with an opening braket. This
    // starts a new variable scope
    {
      // create the writer
      DataWriter<MyData> w(my_write_token, ts);

      // put the proper data in
      w.data().a = some_value;
      w.data().b = some_other_value;
      // etc. Note that you get cryptic error messages if you forget
      // that data() is a function, and type "w.data.a"

    } // <-- This closing bracket ends the "scope", in which
      // DataWriter<MyData> w was created. That means that at this
      // point the destructor is called. The destructor actually sends
      // the data over the channel.
    \endcode

    The DataWriter template, which will create an object for your
    modification, but always starts from clean, blank objects. Note
    that you should provide a proper definition of what is default
    when you write the .dco file defining your data type.

    If you rather update an object you had previously sent, you can
    use the DataUpdater template. Note that the DataUpdater does not
    work for initialisation of array sizes.
*/
template<class T>
class DataWriter: public DataWriterBase
{
  /** Temporary pointer to the data to be written. */
  T *data_ptr;

public:
  /** Constructor. Gains access to the channel for which the token was
      made.

      @param token  Write access token.
      @param ts     Time specification. The specification is used, to
                    define the new data validity span, or only the
                    start time is used if the token has been opened with
                    Channel::Events for time aspect.
  */
  DataWriter(ChannelWriteToken& token, const TimeSpec& ts) :
    DataWriterBase(token, ts),
    data_ptr(new T()) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made.

      @param token  Write access token.
      @param ts     Time specification.
  */
  DataWriter(ChannelWriteToken& token, const DataTimeSpec& ts) :
    DataWriterBase(token, ts),
    data_ptr(new T()) { baseCheckAccess(T::magic_check_number); }

  /** Constructor with tick. Gains access to the channel for which the
      token was made.

      Note that this does not make sense with stream data, only for
      event data!

      @param token  Write access token.
      @param ts     Time tick/moment. */
  DataWriter(ChannelWriteToken& token,
             const TimeTickType ts = SimTime::getTimeTick()) :
    DataWriterBase(token, DataTimeSpec(ts)),
    data_ptr(new T()) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with one parameter, e.g. a
      variable-sized array.

      @param token  Write access token.
      @param ts     Time specification.
      @param n1     First parameter passed to the constructor of the
                    written object
  */
  template <typename N1>
  DataWriter(ChannelWriteToken& token, const TimeSpec& ts, N1 n1) :
    DataWriterBase(token, ts),
    data_ptr(new T(DataWriterArraySize(n1))) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with one parameter, e.g. a
      variable-sized array.

      @param token  Write access token.
      @param ts     Time specification.
      @param n1     First parameter passed to the constructor of the
                    written object
  */
  template <typename N1>
  DataWriter(ChannelWriteToken& token, const DataTimeSpec& ts, N1 n1) :
    DataWriterBase(token, ts),
    data_ptr(new T(DataWriterArraySize(n1))) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with one parameter, e.g. a
      variable-sized array.

      Note that using only a tick does not make sense with stream
      data, only use for event data!

      @param token  Write access token.
      @param ts     Time tick/moment.
      @param n1     First parameter passed to the constructor of the
                    written object
  */
  template <typename N1>
  DataWriter(ChannelWriteToken& token, const TimeTickType& ts, N1 n1) :
    DataWriterBase(token, DataTimeSpec(ts)),
    data_ptr(new T(DataWriterArraySize(n1))) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with two parameters.

      @param token  Write access token.
      @param ts     Time specification.
      @param n1     First parameter passed to the constructor of the
                    written object
      @param n2     Second parameter passed to the constructor of the
                    written object
  */
  template <typename N1,typename N2>
  DataWriter(ChannelWriteToken& token, const TimeSpec& ts, N1 n1, N2 n2) :
    DataWriterBase(token, ts),
    data_ptr(new T(DataWriterArraySize(n1),
                   DataWriterArraySize(n2))) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with two parameters.

      @param token  Write access token.
      @param ts     Time specification.
      @param n1     First parameter passed to the constructor of the
                    written object
      @param n2     Second parameter passed to the constructor of the
                    written object
  */
  template <typename N1,typename N2>
  DataWriter(ChannelWriteToken& token, const DataTimeSpec& ts, N1 n1, N2 n2) :
    DataWriterBase(token, ts),
    data_ptr(new T(DataWriterArraySize(n1),
                   DataWriterArraySize(n2))) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with two parameters.

      Note that using only a tick does not make sense with stream
      data, only use for event data!

      @param token  Write access token.
      @param ts     Time tick/moment.
      @param n1     First parameter passed to the constructor of the
                    written object
      @param n2     Second parameter passed to the constructor of the
                    written object
  */
  template <typename N1,typename N2>
  DataWriter(ChannelWriteToken& token, const TimeTickType& ts, N1 n1, N2 n2) :
    DataWriterBase(token, DataTimeSpec(ts)),
    data_ptr(new T(DataWriterArraySize(n1),
                   DataWriterArraySize(n2))) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with three parameters

      @param token  Write access token.
      @param ts     Time specification.
      @param n1     First parameter passed to the constructor of the
                    written object
      @param n2     Second parameter passed to the constructor of the
                    written object
      @param n3     Third parameter passed to the constructor of the
                    written object
  */
  template <typename N1,typename N2,typename N3>
  DataWriter(ChannelWriteToken& token, const TimeSpec& ts,
             N1 n1, N2 n2, N3 n3) :
    DataWriterBase(token, ts),
    data_ptr(new T(DataWriterArraySize(n1),
                   DataWriterArraySize(n2),
                   DataWriterArraySize(n3))) { baseCheckAccess(T::magic_check_number); }

  /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with three parameters

      @param token  Write access token.
      @param ts     Time specification.
      @param n1     First parameter passed to the constructor of the
                    written object
      @param n2     Second parameter passed to the constructor of the
                    written object
      @param n3     Third parameter passed to the constructor of the
                    written object
  */
  template <typename N1,typename N2,typename N3>
  DataWriter(ChannelWriteToken& token, const DataTimeSpec& ts,
             N1 n1, N2 n2, N3 n3) :
    DataWriterBase(token, ts),
    data_ptr(new T(DataWriterArraySize(n1),
                   DataWriterArraySize(n2),
                   DataWriterArraySize(n3))) { baseCheckAccess(T::magic_check_number); }

   /** Constructor. Gains access to the channel for which the token was
      made, for initializing objects with three parameters

      Note that using only a tick does not make sense with stream
      data, only use for event data!

      @param token  Write access token.
      @param ts     Time tick/moment.
      @param n1     First parameter passed to the constructor of the
                    written object
      @param n2     Second parameter passed to the constructor of the
                    written object
      @param n3     Third parameter passed to the constructor of the
                    written object
  */
  template <typename N1,typename N2,typename N3>
  DataWriter(ChannelWriteToken& token, const TimeTickType& ts,
             N1 n1, N2 n2, N3 n3) :
    DataWriterBase(token, DataTimeSpec(ts)),
    data_ptr(new T(DataWriterArraySize(n1),
                   DataWriterArraySize(n2),
                   DataWriterArraySize(n3)))
  { baseCheckAccess(T::magic_check_number); }

  /** Access the data in the channel, returns a reference to the
      data. */
  inline T& data() {
    return *(data_ptr);
  }

  /** Destructor */
  ~DataWriter() {
    if (data_ptr)
      DataWriterBase::releaseAccess(data_ptr);
  }

private:
  /** Copying is not possible. */
  DataWriter(const DataWriter<T>& );

  /** Nor is assignment. */
  DataWriter<T>& operator = (const DataWriter<T>& );

  /** And new is certainly forbidden! */
  static void* operator new(size_t s);
};

DUECA_NS_END;

#endif
