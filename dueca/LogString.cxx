/* ------------------------------------------------------------------   */
/*      item            : LogString.cxx
        made by         : Rene' van Paassen
        date            : 061128
        category        : body file
        description     :
        changes         : 061128 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define LogString_cxx
#include "LogString.hxx"
#include "Dstring.ixx"

DUECA_NS_START
template class Dstring<LSSIZE>;
DUECA_NS_END

PRINT_NS_START
template std::ostream& operator << (std::ostream& os,
                                    const DUECA_NS ::Dstring<LSSIZE>& o);
PRINT_NS_END

template void packData(DUECA_NS ::AmorphStore& s,
                       const DUECA_NS ::Dstring<LSSIZE>& o);
template void unPackData(DUECA_NS ::AmorphReStore& s,
                         DUECA_NS ::Dstring<LSSIZE>& o);

