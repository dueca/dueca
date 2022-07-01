/* ------------------------------------------------------------------   */
/*      item            : SchemeScripting.hxx
        made by         : Rene van Paassen
        date            : 180220
        category        : header file
        description     :
        changes         : 180220 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SchemeScripting_hxx
#define SchemeScripting_hxx

#include <dueca/ScriptInterpret.hxx>
#include <dueca/ScriptHelper.hxx>
#include <string>
#include <dueca_ns.h>

DUECA_NS_START

/** Implements the interface to Scheme */
struct SchemeScripting: public ScriptHelper
{
  /** scratch file */
 ofstream to_scheme;

  /** Constructor */
  SchemeScripting();

  /** Destructor */
  ~SchemeScripting();

  /** Perform preliminary initialisation */
  void initiate();

  /** Start the interpreter */
  void interpreter();

  /** Read a single line from the module script file */
  bool readline(std::string& line);

  /** Write a single line to a scratch file */
  bool writeline(const std::string& line);
};

DUECA_NS_END
#endif
