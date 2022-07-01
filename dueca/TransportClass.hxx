/* ------------------------------------------------------------------   */
/*      item            : TransportClass.hxx
        made by         : Rene' van Paassen
        date            : 001024
        category        : header file
        description     :
        changes         : 001024 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef TransportClass_hh
#define TransportClass_hh

#include <iostream>
using namespace std;
#include <dueca_ns.h>
#include <ChannelDef.hxx>

DUECA_NS_START

/** \file TransportClass.hxx
    This file defines the TransportClass enumerated type. */

#if 0
/** Quality of service definition for transport of channel data in
    DUECA. Three different levels are defined, it depends on the
    transport mechanism implemented and selected how exactly these
    mechanisms are implemented. */
enum TransportClass {
  Bulk,  /**< Bulk priority is lowest priority in DUECA
            transport. Very large objects can be sent by bulk, because
            in principle the objects do not need to fit in a single
            buffer used by the transport device. Send and arrival
            order of Bulk data among its class is preserved, bulk data
            may arrive later than non-bulk data sent at a later time.*/
  Regular, /**< Regular priority data is sent at the earliest possible
              moment. Triggering of arrival at the next node is no
              later than the first following communication clock
              tick. Send/receive ordering among high priority and
              regular data is preserved. */
  HighPriority /**< High priority data is sent at earliest possible
                  moment, (like regular data). However, triggering in
                  the next node is immediately after arrival in that
                  node. The difference between regular and high
                  priority is currently only possible in set-ups with
                  reflective memory communications. */
};

/** Convenient printing of the transport class. */
ostream& operator << (ostream& os, const TransportClass& tc);

#endif

/** Re-typed TransportClass for compatibility with old code */
typedef Channel::TransportClass TransportClass;
static const Channel::TransportClass Bulk = Channel::Bulk;
static const Channel::TransportClass Regular = Channel::Regular;
static const Channel::TransportClass HighPriority = Channel::HighPriority;

DUECA_NS_END
#endif
