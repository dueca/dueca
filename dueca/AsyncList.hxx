/* ------------------------------------------------------------------   */
/*      item            : AsyncList.hxx
        made by         : Rene van Paassen
        date            : 010801
        category        : header file
        description     :
        changes         : 010801 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef AsyncList_hxx
#define AsyncList_hxx

#include "AsyncQueueMT.hxx"

DUECA_NS_START

/** @file AsyncList.hxx

    There used to be a separate AsyncList class, but the newer, more
    capable and better tested AsyncQueueMT is currently used. */

/** Re-defined AsyncList in terms of AsyncQueueMT */
template <class T, class Alloc=ListElementAllocator<T> >
using AsyncList = AsyncQueueMT<T,Alloc>;
/** Re-defined AsyncListWriter in terms of AsyncQueueWriter */
template <class T, class Alloc=ListElementAllocator<T> >
using AsyncListWriter = AsyncQueueWriter<T,Alloc>;

DUECA_NS_END

#endif
