/* ------------------------------------------------------------------   */
/*      item            : WrapSendEvent.hxx
        made by         : Rene van Paassen
        date            : 131227
        category        : header file
        description     :
        changes         : 131227 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef WrapSendEvent_hxx
#define WrapSendEvent_hxx

#include <dueca_ns.h>
#include <EventAccessToken.hxx>
#include <ChannelWriteToken.hxx>

/** @file WrapSendEvent.hxx Compatibility header, providing wrapper
    functions for the older .sendEvent options of event write tokens */


DUECA_NS_START;

/** @private Internal function for the wrapSendEvent */
inline void _wrapSendEvent(ChannelWriteToken &t, const void* edata,
                           const DataTimeSpec& tic)
{
  t.releaseAccess(edata, tic);
}

/** The "wrapSendEvent" function can be used to send an event with
    self-allocated event data.

    @param t     Valid channel write token, configured for events.
    @param edata Pointer to the object to be written. Must be a valid
                 DCO object, dynamically allocated, and the channel will
                 assume ownership!
    @param tick  Time tick for the writing.

    Note that the use of this function should be reserved to people
    who really know what they are doing, and only if there is a real
    (performance) need, if you are not one of these, inform yourself
    well or use an DataWriter, which provides a much safer interface
    to the channel.
*/
inline void wrapSendEvent(ChannelWriteToken &t, const void* edata,
                          const TimeTickType& tick)
{
  _wrapSendEvent(t, edata, DataTimeSpec(tick, tick));
}


/** The "wrapSendEvent" function can be used to send an event with
    self-allocated event data.

    Note that the use of this function should be reserved to people
    who really know what they are doing, and only if there is a real
    (performance) need, if you are not one of these, inform yourself
    well or use a DataWriter, which provides a much safer interface
    to the channel.
*/
template <class T>
inline void wrapSendEvent(EventChannelWriteToken<T> &t, const T* edata,
                          const TimeTickType& tick)
{
  _wrapSendEvent(t, edata,  DataTimeSpec(tick, tick));
}

/** The "wrapSendData" function can be used to send an event or stream
    data with self-allocated data.

    @param t     Valid channel write token, configured for events.
    @param edata Pointer to the object to be written. Must be a valid
                 DCO object, dynamically allocated, and the channel will
                 assume ownership!
    @param tick  Time spec for the writing.

    Note that the use of this function should be reserved to people
    who really know what they are doing, and only if there is a real
    (performance) need, if you are not one of these, inform yourself
    well or use a DataWriter, which provides a much safer interface
    to the channel.
*/
inline void wrapSendData(ChannelWriteToken &t, const void* edata,
                          const DataTimeSpec& tick)
{
  _wrapSendEvent(t, edata, tick);
}



DUECA_NS_END;


#endif
