/* ------------------------------------------------------------------   */
/*      item            : ParameterTable.cxx
        made by         : Rene' van Paassen
        date            : 001006
        category        : body file
        description     :
        changes         : 001006 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ParameterTable_cc
#include "ParameterTable.hxx"
#include "GenericVarIO.hxx"

DUECA_NS_START

const char* set_timing_description =
"Supply a time specification to define the update rate of the main activity";
const char* check_timing_description =
"Supply three integer parameters to specify a check on the timing of\n"
"the main activity: warning limit (in us), critical limit (in us), and\n"
"the number of loops to test before sending a report (optional, dflt=2000)";

ParameterTable::~ParameterTable()
{
  delete probe;
}

DUECA_NS_END
