/* ------------------------------------------------------------------   */
/*      item            : PythonScripting.hxx
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

#ifndef PythonScripting_hxx
#define PythonScripting_hxx

#include <dueca/ScriptHelper.hxx>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/python.hpp>
#undef HAVE_PTHREAD_H
#undef HAVE_SCHED_H
#undef HAVE_SETUID
#undef HAVE_SIGNAL_H
#undef HAVE_STDLIB_H
#undef HAVE_SYS_PARAM_H
#undef HAVE_SYS_TIME_H
#undef HAVE_UNISTD_H
#undef HAVE_FCNTL_H
#include <fstream>
namespace bpy = boost::python;

#include <string>
#include <dueca_ns.h>

DUECA_NS_START

/** Implements the interface to Python */
struct PythonScripting: public ScriptHelper
{
  /** scratch file */
  std::ofstream scratchfile;

  /** main module? */
  bpy::object main_module;

  /** main namespace for the script */
  bpy::object main_namespace;

  /** continue flag */
  bool running;

  /** Constructor */
  PythonScripting();

  /** Destructor */
  ~PythonScripting();

  /** Perform preliminary initialisation */
  void initiate();

  /** Start the interpreter */
  void interpreter();

  /** Read a single line from the module script file */
  bool readline(std::string& line);

  /** Write a single line to a scratch file */
  bool writeline(const std::string& line);

  /** Run a single string of code directly */
  void runCode(const char* code);
};

DUECA_NS_END
#endif
