/* ------------------------------------------------------------------   */
/*      item            : DUSIMEExceptions.cxx
        made by         : Rene' van Paassen
        date            : 211215
        category        : body file
        description     :
        changes         : 211215 first version
        language        : C++
        copyright       : (c) 2021 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DUSIMEExceptions_cxx
#include "DUSIMEExceptions.hxx"
#include <stdio.h>

DUECA_NS_START



double_snapshot_origin::
double_snapshot_origin(const char* originator)
{
  snprintf(str, sizeof(str), "Duplicate snapshot received from : %s", originator);
}

initial_file_mismatch::
initial_file_mismatch(const char* originator)
{
  snprintf(str, sizeof(str), "Problem parsing initial file : %s", originator);
}

DUECA_NS_END
