/* ------------------------------------------------------------------   */
/*      item            : ChannelDistributionExtra.hxx
        made by         : Rene' van Paassen
        date            : ??????
        category        : Main program code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 1999 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** DuecaMain.cxx Starting via Script */

#include "ScriptInterpret.hxx"
#include "Environment.hxx"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <dueca/visibility.h>
#ifdef __MINGW32__

#include <boost/date_time/posix_time/posix_time.hpp>
#endif
#include "DuecaEnv.hxx"
#include "StateGuard.hxx"
#include <dueca-conf.h>
#include <dueca-version.h>
#include <oddoptions.h>
#if defined(SYNC_WITH_RTAI)
#ifdef HAVE_RTAI_LXRT_H
#include <rtai_lxrt.h>
#endif
#endif


#if defined(SYNC_WITH_RTAI)
#ifdef HAVE_RTAI_LXRT_H
#include <rtai_lxrt.h>
#else
#error RTAI configuration
#endif

#elif defined(SYNC_WITH_XENOMAI)
#ifdef HAVE_POSIX_POSIX_H
#include <posix/posix.h>
#else
#error Xenomai configuration
#endif

#endif

#include <debprint.h>
#ifdef BUILD_FEXCEPT
#define _GNU_SOURCE
#include <fenv.h>
#endif

DUECA_NS_START


//extern void init_dueca_scheme(void);
int* p_argc;
char*** p_argv;

DUECA_NS_END

USING_DUECA_NS

LNK_PUBLIC int main(int argc, char* argv[])
{
  /* Enable floating point exceptions when configured. Usually only for
     debug purposes. */
#ifdef BUILD_FEXCEPT
  feenableexcept(FE_INVALID   |
                 FE_DIVBYZERO |
                 FE_OVERFLOW  |
                 FE_UNDERFLOW);
#endif

  /* Initialization of the script might use arguments from the command
     line. Pass the arguments */
  p_argc = &argc;
  p_argv = &argv;

  /* When --version is given, print an overview of all built-in
     components, and give the dueca version and time */
  if (argc == 2 && !::strcmp(argv[1], "--version")) {
    cout << "Dueca version " << DUECA_VERSIONSTRING << endl
         << COPYRIGHT << endl;
    ::exit(0);
  }

  /* Initialises script language interface */
  ScriptInterpret::initializeScriptLang();

  /* Print startup blurp when a "DUECA_SCRIPTINSTRUCTIONS" variable is
     not set. When it is set, the information on creating objects or
     modules from the script is printed.
   */
  if (!DuecaEnv::scriptSpecific()) {
    cout << endl << "This application uses DUECA/DUSIME, version "
         << DUECA_VERSIONSTRING
         << " (" << VERSION_DATE << ")" << endl
         << "Copyright for DUECA libraries, headers & tools:" << endl
         << COPYRIGHT << endl << endl;
    cout << "Licensed under the EUPL-1.2 license" << endl;
  }

  /* Test builds generally do not try to catch exceptions from DUECA
     activities, this speeds up debugging. */
#if !defined(TEST_OPTIONS) && !defined(ACTIV_NOCATCH)
  try {
#endif

    if (!DuecaEnv::scriptSpecific()) {
#if defined(SYNC_WITH_RTAI) || defined(SYNC_WITH_XENOMAI)
      if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("Cannot memlock program");
      }
#endif
    }

    if (DuecaEnv::scriptSpecific()) {
      ::exit(DuecaEnv::handledSpecific());
    }

    cout << "Starting interpretation of the dueca config script" << endl;
    ScriptInterpret::single()->startScript();
#if !defined(TEST_OPTIONS) && !defined(ACTIV_NOCATCH)
  }
  catch (const std::exception& e) {

    // print the cause of the trouble
    cerr << e.what() << endl;

    throw(e);
  }
#endif

  sleep(1);

  cout << "out of main" << endl;
  return Environment::getInstance()->getExitCode();
}
