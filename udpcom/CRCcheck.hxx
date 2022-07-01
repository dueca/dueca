/* ------------------------------------------------------------------   */
/*      item            : CRCcheck.hxx
        made by         : Rene van Paassen
        date            : 200417
        category        : header file
        description     :
        changes         : 200417 first version
        language        : C++
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CRCcheck_hxx
#define CRCcheck_hxx

#include <cstdlib>
#include <cstdint>
#include <dueca_ns.h>

DUECA_NS_START;

uint16_t crc16_ccitt(const char* pbyte, size_t len);

DUECA_NS_END;

#endif
