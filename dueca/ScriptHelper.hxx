/* ------------------------------------------------------------------   */
/*      item            : ScriptHelper.hxx
        made by         : Rene van Paassen
        date            : 180310
        category        : header file
        description     :
        changes         : 180310 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ScriptHelper_hxx
#define ScriptHelper_hxx

#include <string>
#include <dueca/dueca_ns.h>

DUECA_NS_START

/** Helper class processing script-specific actions */
struct ScriptHelper
{
  /** For the classic scheme interface, the script interpret needs to
      write typical phrases. This continues to model creation */
  std::string phase2;

  /** Script phrase to go to running */
  std::string phase3;

  /** Line to stop the script */
  std::string quitline;

  /** Stopsign (a comment in the script, indicates script running */
  std::string stopsign;

  /** Initiate scripting library */
  virtual void initiate() = 0;

  /** Enter the interpreter */
  virtual void interpreter() = 0;

  /** Read a single line from the model script */
  virtual bool readline(std::string& line) = 0;

  /** Write s single line to a scratch file */
  virtual bool writeline(const std::string& line) = 0;

  /** Run a single string of code directly */
  virtual void runCode(const char* code);

  /** Constructor. */
  ScriptHelper(const char* phase2, const char* phase3,
               const char* quitline, const char* stopsign);

  /** Destructor */
  virtual ~ScriptHelper();
};

DUECA_NS_END

#endif
