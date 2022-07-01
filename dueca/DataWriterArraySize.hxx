/* ------------------------------------------------------------------   */
/*      item            : DataWriterArraySize.hxx
        made by         : Rene van Paassen
        date            : 210826
        category        : header file
        description     :
        changes         : 210826 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataWriterArraySize_hxx
#define DataWriterArraySize_hxx

#include <dueca_ns.h>
DUECA_NS_START

/** Distinguishing class for array size initialization */
struct DataWriterArraySize {
  size_t size;
  DataWriterArraySize(size_t s) : size(s) {}
};

DUECA_NS_END;

#endif
