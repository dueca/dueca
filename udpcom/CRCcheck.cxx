/* ------------------------------------------------------------------   */
/*      item            : CRCcheck.cxx
        made by         : Rene' van Paassen
        date            : 200417
        category        : body file
        description     :
        changes         : 200417 first version
        language        : C++
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define CRCcheck_cxx
#include "CRCcheck.hxx"

#include <boost/crc.hpp>

DUECA_NS_START;

uint16_t crc16_ccitt(const char* pbyte, size_t len)
{
  boost::crc_ccitt_type thecrc;
  thecrc.process_bytes(pbyte, len);
  return thecrc.checksum();
  // CRC-16-CCITT
}

DUECA_NS_END;

