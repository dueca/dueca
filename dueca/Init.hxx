/* ------------------------------------------------------------------   */
/*      item            : InitShm.hxx
        made by         : Rene van Paassen
        date            : 021001
        category        : header file
        description     :
        changes         : 021001 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Init_hxx
#define Init_hxx

#include <dueca_ns.h>

DUECA_NS_START
void init_reflective_components();
void init_copying_components();
void init_dueca_accessor();
void init_dueca_genericpacker();
void init_dueca_ipdeps();
DUECA_NS_END

#endif
