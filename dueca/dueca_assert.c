/* ------------------------------------------------------------------   */
/*      item            : dueca_assert.c
        made by         : Arthur de Jong (West Consulting)
        date            : 020829
        category        : body file
        description     : definition of assertions used in dueca
        changes         :
        language        : C++
        copyright       : (c) 2002 TUDelft-AE-C&S
*/

#include "dueca_assert.h"

void DUECA_SCM_ASSERT(int cond, SCM arg, int pos, const char *s_subr)
{
  SCM_ASSERT(cond,arg,pos,s_subr);
}
