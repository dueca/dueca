/* ------------------------------------------------------------------   */
/*      item            : StartIOStream.hxx
        made by         : Rene van Paassen
        date            : 151023
        category        : header file
        description     :
        changes         : 151023 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef StartIOStream_hxx
#define StartIOStream_hxx

#include <dueca_ns.h>
DUECA_NS_START;

/** Initialize iostreams library.

    Since we are running a lot of code before main, and iostreams is
    not guaranteed to be initialized at this time, this call will fix
    that. Used in CoreCreator, TypeCreator code and Init* files
*/
void startIOStream();

DUECA_NS_END;
#endif
