/* ------------------------------------------------------------------   */
/*      item            : ScriptHelper.cxx
        made by         : Rene' van Paassen
        date            : 180310
        category        : body file
        description     :
        changes         : 180310 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ScriptHelper_cxx
#include <dueca/ScriptHelper.hxx>
#include <dueca/ScriptInterpret.hxx>
#include <debug.h>

DUECA_NS_START

ScriptHelper::ScriptHelper(const char* phase2, const char* phase3,
                           const char* quitline, const char* stopsign) :
  phase2(phase2), phase3(phase3),
  quitline(quitline), stopsign(stopsign)
{ }

ScriptHelper::~ScriptHelper()
{
  //
}

void ScriptHelper::runCode(const char* code)
{
  /* DUECA scripting.

     Currently, only Python script interface can run client code in
     the script language. An attempt to run client code is made while
     using an incompatible script language. */
  W_SYS("this interpreter cannot run client code");
  throw (scriptexception());
}

DUECA_NS_END

