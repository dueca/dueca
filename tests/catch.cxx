/* ------------------------------------------------------------------   */
/*      item            : catch.cxx
        made by         : Rene' van Paassen
        date            : 030218
        category        : body file
        description     :
        changes         : 030218 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define catch_cxx

#include <iostream>
#include <libguile.h>
using namespace std;

typedef SCM (*scm_func) ();
static SCM main_thread(SCM stage)
{
  try {
    throw int(2);
  }
  catch (int& i) {
    cout << "caught " << i << endl;
  }

  return SCM_BOOL_T;
}

static void
inner_main (void *closure, int argc, char **argv)
{
  try {
    throw int(1);
  }
  catch (int& i) {
    cout << "caught " << i << endl;
  }

  scm_make_gsubr("pass-control", 1, 0, 0, (scm_func) main_thread);

  // call the scheme shell, never returns!
  scm_shell (argc, argv);
}

int main()
{
  // arguments for the scheme interpreter
  int argc = 3;
  char* argv[3] = {"catch.x", "-s", "guile.script"};
  scm_boot_guile(argc, argv, inner_main, 0);
}
