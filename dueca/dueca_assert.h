/* ------------------------------------------------------------------   */
/*      item            : dueca_assert.h
        made by         : Arthur de Jong (West Consulting)
        date            : 020829
        category        : header file
        description     : definition of assertions used in dueca
        changes         :
        language        : C++
        copyright       : (c) 2002 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include <includeguile.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#ifdef SCRIPT_SCHEME
extern void DUECA_SCM_ASSERT(int cond, SCM arg, int pos, const char *s_subr);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */
