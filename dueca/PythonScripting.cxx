/* ------------------------------------------------------------------   */
/*      item            : PythonScripting.cxx
        made by         : Rene' van Paassen
        date            : 180220
        category        : body file
        description     :
        changes         : 180220 first version
        language        : C++
        copyright       : (c) 18 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define PythonScripting_cxx
#include <dueca/PythonScripting.hxx>
#include <dueca/ScriptInterpret.hxx>
#include <dueca/Environment.hxx>
#include <dueca/debug.h>
#define DEBPRINTLEVEL -1
#include <debprint.h>
#include <algorithm>
#include <fstream>
#include <dueca-conf.h>
#include <boost/python/module.hpp>


DUECA_NS_START

extern int* p_argc;
extern char*** p_argv;

PythonScripting::PythonScripting() :
  ScriptHelper("",
               "",
               "sys.exit(0)      # *Added by PythonScripting*",
               "#:end_of_input:#"),
  running(true)
{
  ScriptInterpret::single(this);
}

PythonScripting::~PythonScripting()
{
  //
}

BOOST_PYTHON_MODULE(dueca)
{

  DEB("Python initialized=" << Py_IsInitialized());
#ifdef DEBDEF
  std::wcout << L"program name " << Py_GetProgramFullPath() << std::endl;
#endif
  //  bpy::object inheritance_exception =
  //  bpy::import("exceptions").attr("RunTimeError");

  size_t ninits = ScriptInterpret::single()->getNumInitFunctions()+1;
  const InitFunction *f = ScriptInterpret::single()->getNextInitFunction();

  /* The init functions might not be in the right order, typically
     init for a class might occur before its base class has been
     created. We deal with it in a pythonic fashion, at a failure the
     init is put again at the back of the list.

     ninits is used as a counter to prevent running indefinitely if a
     base class is not in the list at all. */
  while (f && ninits) {
    try {
      DEB("Running init for " << f->name);
      (*f)();
      delete f;
      ninits = ScriptInterpret::single()->getNumInitFunctions() + 1;
      DEB("Init functions left " << ninits)
    }
    catch(const bpy::error_already_set& ex) {

      /* DUECA scripting.

	 A given init function for starting up Python capabilities
	 has failed. Check the C++ program. This will result in a
	 failure to run.
      */
      E_CNF("Error in the script init function for " << f->name);
      PyErr_PrintEx(1);
      throw(ex);
    }
    f = ScriptInterpret::single()->getNextInitFunction();
  }

  if (ScriptInterpret::single()->getNumInitFunctions()) {
    /* DUECA scripting.

       Logical error in the start-up of Python code. Not all
       initialization functions could be run, possibly due to a
       circular dependency on initialization.
    */
    E_CNF("Unable to run all python objects");
  }
  /* module initializations go here */
  //ScriptInterpret::single()->runInitFunctions();
}

#if PY_MAJOR_VERSION >= 3
// according to docs, in static storage
static wchar_t programname[256] = { 0 };
#else
#warning "Python 2 is obsolete!"
#endif

void PythonScripting::initiate()
{
  scratchfile.open("dueca.scratch", ios::out);

  // check that it is OK. Unwriteable files do not return good
  if (!scratchfile.good()) {
    /* DUECA scripting.

       Failure to use the "dueca.scratch" file. For interpreting the
       script, DUECA uses a "dueca.scratch" file. It was not possible
       to open this in write mode. This may be due to an existing file
       for which the current process has no write permissions. Remove
       the "dueca.scratch" file.
    */
    E_CNF("Cannot create the \"dueca.scratch\" file, please remove it");
    throw(scriptexception());
  }

  // the appendtab adds the dueca namespace initialisation
  try {


#if PY_VERSION_HEX >= 0x03080000
    // trying preconfig, does not work on Python 3.7 ?
    PyPreConfig preconfig;
    PyPreConfig_InitIsolatedConfig(&preconfig);
#if defined(FORCE_PYTHON_MALLOC)
    preconfig.allocator = PYMEM_ALLOCATOR_MALLOC;
#endif
    preconfig.utf8_mode = -1;
    preconfig.use_environment = 1;
    PyStatus stat = Py_PreInitialize(&preconfig);
    if (PyStatus_IsError(stat)) {
      /* DUECA scripting.

	 This DUECA/Python combination uses pre-init, to initialize
	 Python. This lead to an error status, see message.
      */
      E_CNF("Error status from python pre-init " << stat.err_msg);
    }
#endif

    if (PyImport_AppendInittab("dueca",
#if PY_MAJOR_VERSION == 2
                               &initdueca
#else
                               &PyInit_dueca
#endif
                               ) == -1) {
      /* DUECA scripting.

         Unexpected error in initializing scripting with Python.
      */
      E_CNF("Could not extend built-in modules with dueca");
    }

    // start python interpreter
#if PY_MAJOR_VERSION >= 3
    swprintf(programname, 255, L"%hs", *p_argv[0]);
    Py_SetProgramName(programname);
#else
    Py_SetProgramName(*p_argv[0]);
#endif

#if PY_VERSION_HEX >= 0x03080000
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    config.isolated = 1;

    stat = Py_InitializeFromConfig(&config);
    if (PyStatus_IsError(stat)) {
      /* DUECA scripting.

	 Error status returned from the Python init call.
      */
      E_CNF("Error status from python init " << stat.err_msg);
    }
#else
#warning "Older python version, no choices on init"
    Py_InitializeEx(0);
#endif
  }
  catch(const bpy::error_already_set& e) {
    /* DUECA scripting.

       Unspecified error in initializing the Python script language
       and starting the interpreter. Check the DUECA and Python
       libraries.
    */
    E_CNF("Error in the python script language initialization");
    PyErr_PrintEx(1);
    throw(e);
  }
}


void PythonScripting::interpreter()
{
  // access the namespace
  main_module = bpy::import("__main__");
  main_namespace = main_module.attr("__dict__");

  DEB("calling python with dueca_cnf.py");
  try {
    bpy::object ignored = bpy::exec
      ("exec(open('dueca_cnf.py').read(), globals())",
       main_namespace, main_namespace);
  }
  catch(const bpy::error_already_set& e) {
    /* DUECA scripting.

       Found an error in the dueca_cnf.py script. Locate the error
       using the messages and line number, and correct it.
    */
    E_CNF("Error in the dueca_cnf.py script");
    PyErr_PrintEx(1);
    throw(e);
  }

  /* jump into environment thread */
  Environment::getInstance()->proceed(1);

  try {
    /* After exiting, the scratch file has been filled with module
       creation and flushed */
    bpy::object ignored = bpy::exec
      ("exec(open('dueca.scratch').read(), globals())",
       main_namespace, main_namespace);
  }
  catch(const bpy::error_already_set& e) {
    /* DUECA scripting.

       An error occurred in the dueca_mod.py script. Check the script
       at the indicated error line (and above).
    */
    E_CNF("Error in the dueca_mod.py script");
    PyErr_PrintEx(1);
    throw(e);
  }

  /* recycle scratchfile */
  scratchfile.close(); scratchfile.open("dueca.scratch", ios::out);

  /* back to environment thread */
  Environment::getInstance()->proceed(2);

  /* process additional script input */
  while(running) {
    try {
      bpy::object ignored = bpy::exec
        ("exec(open('dueca.scratch').read(), globals())",
         main_namespace, main_namespace);
    }
    catch(const bpy::error_already_set& e) {
      /* DUECA scripting.

         A Python error occurred in an attempt to load an additional
         configuration file. Check the error message and correct the
         program.
      */
      E_CNF("Error in additional model code running");
      PyErr_PrintEx(1); throw(e); } scratchfile.close();
      scratchfile.open("dueca.scratch", ios::out);

    Environment::getInstance()->proceed(3);
  }

}

void PythonScripting::runCode(const char* code)
{
  try {
    bpy::object ignored = bpy::exec
      (code, main_namespace, main_namespace);
  }
  catch(const bpy::error_already_set& e) {
    /* DUECA scripting.

       There is an error in a code snippet supplied from the DUECA C++
       program. Find the given code and correct. */
    E_CNF("Error in custom code running:\n" << code);
    PyErr_PrintEx(1);
    throw(e);
  }
}

bool PythonScripting::readline(std::string& line)
{
  static ifstream mod("dueca_mod.py");
  if (!mod.good()) {
    cerr << "Error opening dueca_mod.py" << endl;
    throw(scriptexception());
  }
  return bool(getline(mod, line));
}

bool PythonScripting::writeline(const std::string& line)
{
  if (line == stopsign) {

    // flush the output file
    scratchfile.flush();
    return true;
  }
  if (line == quitline) {
    running = false;
    return false;
  }
  scratchfile << line << endl;
  return false;
}


DUECA_NS_END
