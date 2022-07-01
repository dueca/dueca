/* ------------------------------------------------------------------   */
/*      item            : ChronoTimePoint.hxx
        made by         : Rene van Paassen
        date            : 220513
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 220513 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChronoTimePoint_hxx
#define ChronoTimePoint_hxx

#include <chrono>
#include <iostream>
#include <string>
#include <dueca/dueca_ns.h>

/// \cond DO_NOT_DOCUMENT
DUECA_NS_START
/// \endcond

/** @file ChronoTimePoint.hxx  conversion between time point and string */

/** Print a timepoint */
std::ostream& operator << (std::ostream& os,
                           const std::chrono::system_clock::time_point& tp);

/** Convert a timepoint */
std::string timePointToString
(const std::chrono::system_clock::time_point& time);

/** Extract a timepoint */
std::chrono::system_clock::time_point
timePointFromString(const std::string& date);

/// \cond DO_NOT_DOCUMENT
DUECA_NS_END;
/// \endcond
#endif
