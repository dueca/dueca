/* ------------------------------------------------------------------   */
/*      item            : SchemeScripting.cxx
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

#define SchemeScripting_cxx

#include <scriptinterface.h>
#include "SchemeScripting.hxx"
#include <debug.h>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

SchemeScripting::SchemeScripting() :
  ScriptHelper("(pass-control 2) ; *Added by ScriptInterpret*",
               "(pass-control 3) ; *Added by ScriptInterpret*",
               "(quit) ; *Added by ScriptInterpret*",
               ";;end_of_input::")
{
  ScriptInterpret::single(this);
}

SchemeScripting::~SchemeScripting()
{
  to_scheme << "(quit)" << endl;
  to_scheme.close();
}

void SchemeScripting::initiate()
{
  to_scheme.open("dueca.scratch", ios::out);

  // check that it is OK. Unwriteable files do not return good
  if (!to_scheme.good()) {
    /* DUECA scripting.

       Failure to use the "dueca.scratch" file. For interpreting the
       script, DUECA uses a "dueca.scratch" file. It was not possible
       to open this in write mode. This may be due to an existing file
       for which the current process has no write permissions. Remove
       the "dueca.scratch" file.
    */
    E_CNF("Cannot write the \"dueca.scratch\" file, please remove it");
    std::exit(1);
  }
}

// callback from scheme
void scheme_inner_main(void *closure, int argc, char **argv)
{
  /* module initializations go here */
#if 0
  for (list<voidfunc>::iterator ii =
         ScriptInterpret::singleton->init_functions.begin();
       ii != ScriptInterpret::singleton->init_functions.end(); ii++) {
    (*(*ii))();
  }
#endif
  for (const InitFunction* f = ScriptInterpret::single()->getNextInitFunction();
       f; f = ScriptInterpret::single()->getNextInitFunction()) {
    (*f)();
    delete f;
  }
  // call the scheme shell, never returns!
  scm_shell (argc, argv);
}

void SchemeScripting::interpreter()
{
  // arguments for the scheme interpreter
#if SCM_MAJOR_VERSION >= 2
  int argc = 4;
#else
  int argc = 3;
#endif
  char* argv[4] = {const_cast<char*>("dusime_guile"),
#if SCM_MAJOR_VERSION >= 2
                   const_cast<char*>("--no-auto-compile"),
#endif
                   const_cast<char*>("-s"),
                   const_cast<char*>("dueca.scratch")};

  {
    // now write the basic configuration to the scratch file
    std::string buff;

    std::ifstream cnf("dueca.cnf");
    if (!cnf.good()) {
      std::cerr << "Error opening dueca.cnf" << std::endl;
      throw(scriptexception());
    }
    while(getline(cnf, buff)) {
      to_scheme << buff << std::endl;
    }
    to_scheme << "(pass-control 1) ; *Added by ScriptInterpret*" << std::endl;
    to_scheme.flush();
  }

#if defined(SCM_USE_FOREIGN)
  DEB("calling guile with arguments: " << argv[0] << ' '
      << argv[1] << ' ' << argv[2] << ' ' << argv[3]);
#else
  DEB("calling guile with arguments: " << argv[0] << ' '
      << argv[1] << ' ' << argv[2]);
#endif

  scm_boot_guile(argc, argv, scheme_inner_main, 0);
}

bool SchemeScripting::readline(std::string& line)
{
  static ifstream mod("dueca.mod");
  if (!mod.good()) {
    cerr << "Error opening dueca.mod" << endl;
    throw(scriptexception());
  }
  return bool(getline(mod, line));
}

bool SchemeScripting::writeline(const std::string& line)
{
  if (line == stopsign) {

    // flush the output file
    to_scheme.flush();
    return true;
  }
  to_scheme << line << endl;
  return false;
}


DUECA_NS_END
