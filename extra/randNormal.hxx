/* ------------------------------------------------------------------   */
/*      item            : randNormal.hxx
        made by         : Rene van Paassen
        date            : 010615
        category        : header file
        description     :
        documentation   : DUECA_API
        changes         : 010615 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef randNormal_hxx
#define randNormal_hxx

#ifdef randNormal_cxx
#endif

#ifndef RANDNORMALTEST
#include <dueca_ns.h>
DUECA_NS_START
#endif

/** \file randNormal.hxx
    Normal distribution (well, approximately) random generator. */

/** Returns a random number. */
double randNormal();

#ifndef RANDNORMALTEST
DUECA_NS_END
#endif
#endif
