/* ------------------------------------------------------------------   */
/*      item            : DataUpdater.hxx
        made by         : Rene van Paassen
        date            : 141023
        category        : header file
        description     :
        changes         : 141023 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataUpdater_hxx
#define DataUpdater_hxx

#include <dueca_ns.h>

DUECA_NS_START

/** Common base class for DataUpdater instantiations. */
class DataUpdaterBase
{
protected:
  /** Access to the channel. */
  ChannelWriteToken &token;

  /** Time specification to be used. */
  DataTimeSpec ts;

protected:
  /** Constructor */
  DataUpdaterBase(ChannelWriteToken& token, const DataTimeSpec& ts) :
    token(token), ts(ts) { }

protected:
  /** Lets the token copy the latest data written (or, if not
      available, return a default object)

      @param magic   Magic check number, used to verify data class

      @returns       A void pointer to the data object
  */
  inline void *baseGetAccess(uint32_t magic)
  { return token.getAccess(magic); }

  /** Releasing the read access means that the data will be sent to
      the channel and made accessible for reading. The associated time
      can be used to trigger clients who requested triggering.

      @param data_ptr Pointer to the data object to be sent
  */
  inline void releaseAccess(void* data_ptr)
  { token.releaseAccess(data_ptr, ts); }
};

/** This is a facilitator for writing multi-stream data. By creating a
    "DataUpdater", the access token is used to gain access to the data
    in the channel. A copy of the latest data written is returned to
    the updater. Using the data() accessor method, any changes to this
    data may be made. When at the end of scope the DataUpdater is
    destroyed, the access is released again, and the actual data is
    sent to the channel.

    An example, suppose you have created a ChannelWriteToken for
    a channel with MyData objects:

    \code
    // it is good practice to start with an opening braket. This
    // starts a new variable scope
    {
      // create the writer
      DataUpdater<MyData> w(my_write_token, ts);

      // put the proper data in
      w.data().a = some_value;
      w.data().b = some_other_value;

      // etc. Note that you get cryptic error messages if you forget
      // that data() is a function, and type "w.data.a"

    } // <-- This closing bracket ends the "scope", in which
      // DataUpdater<MyData> w was created. That means that at this
      // point the destructor is called. The destructor actually sends
      // the data over the channel.
    \endcode

    UnifiedChannel channels work differently from Event and Stream
    channels, in that the data written there is persistent. The next
    time you create a DataUpdater on a channel, the data you had
    written before is present (and readable) in the Updater. If you
    want, you can limit yourself to only writing the changes.

    If you rather start from a blank/default object, use the
    DataWriter template, which will create an object for your
    modification, but always starts from clean, blank objects. Note
    that you should provide a proper definition of what is default
    when you write the .dco file defining your data type, otherwise
    the object's members are undefined.
*/
template<class T>
class DataUpdater: public DataUpdaterBase
{
  /** Temporary pointer to the data to be written. */
  T *data_ptr;

  /** Allocate memory through the token */
  inline T *getAccess()
  { return reinterpret_cast<T*>(baseGetAccess(T::magic_check_number)); }

public:
  /** Constructor. Gains access to the channel for which the token was
      made. The accessed data will contain the latest written values.

      @param token  Write access token.
      @param ts     Time specification. The specification is used, to
                    define the new data validity span, or only the
                    start time is used if the token has been opened with
                    Channel::Events for time aspect.
 */
  DataUpdater(ChannelWriteToken& token, const TimeSpec& ts) :
    DataUpdaterBase(token, ts),
    data_ptr(getAccess())
  { }

  /** Constructor. Gains access to the channel for which the token was
      made.

      @param token  Write access token.
      @param ts     Time specification.
  */
  DataUpdater(ChannelWriteToken& token, const DataTimeSpec& ts) :
    DataUpdaterBase(token, ts),
    data_ptr(getAccess())
  { }

  /** Constructor with tick. Gains access to the channel for which the
      token was made.

      Note that using only a tick does not make sense with stream
      data, only use for event data!

      @param token  Write access token.
      @param ts     Time tick/moment. */
  DataUpdater(ChannelWriteToken& token, const TimeTickType& ts) :
    DataUpdaterBase(token, DataTimeSpec(ts)),
    data_ptr(getAccess())
  { }

  /** Access the data in the channel, returns a reference to the
      data. */
  inline T& data() {
    return *(data_ptr);
  }

  /** Destructor. At destruction of the updater, the data is sent. */
  ~DataUpdater() {
    DataUpdaterBase::releaseAccess(data_ptr);
  }

private:
  /** Copying is not possible. */
  DataUpdater(const DataUpdater<T>& );

  /** Nor is assignment. */
  DataUpdater<T>& operator = (const DataUpdater<T>& );

  /** And new is certainly forbidden! */
  static void* operator new(size_t s);
};

DUECA_NS_END;

#endif
