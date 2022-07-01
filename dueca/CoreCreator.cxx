/* ------------------------------------------------------------------   */
/*      item            : CoreCreator.cxx
        made by         : Rene' van Paassen
        date            : 030508
        category        : implementation file
        description     :
        changes         : 030508 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#if !defined(STATIC)

#define IMPLEMENT_GETSCMVAR
#include "CoreCreator.hxx"
#include "CoreCreator.ixx"

#if defined(SCRIPT_PYTHON)
template<>
const char* core_creator_name<mpl_::void_>(const char*)
{ return NULL; }
#endif

#endif
