/* ------------------------------------------------------------------   */
/*      item            : scriptinterface.h
        made by         : Rene van Paassen
        date            : 180322
        category        : header file
        description     : Center for calling in scripting language
                          interface
        changes         : 180322 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef scriptinterface_h
#define scriptinterface_h

#include <dueca/DuecaEnv.hxx>
#include <dueca/StartIOStream.hxx>

/** Base class for all script-created objects */
namespace dueca {
  class ScriptCreatable;
}

/** Start-up calls */
#define STARTUPSECTION(A) \
__attribute__((constructor)) static void init_section() \
{ \
  dueca::startIOStream();             \
  if (!dueca::DuecaEnv::scriptSpecific()) {              \
    std::cout << "Init from     [" #A "]" << std::endl; \
  }

#define ENDSTARTUPSECTION }

#if defined(SCRIPT_SCHEME)

#if defined(SCRIPT_PYTHON)
#error "Only one scripting language can be selected"
#endif

#include "dueca-guile.h"

namespace dueca {
  /** A dummy type to indicate no option/class in a template */
  struct NOOP {};
}

#elif defined(SCRIPT_PYTHON)

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/python.hpp>

namespace bpy = boost::python;
namespace dueca {
  /** A dummy type to indicate no option/class in a template */
  typedef boost::mpl::void_ NOOP;
}

// neuter scheme class stuggles
#define SCHEME_CLASS_DEC(A)
#define SCHEME_CLASS_SINGLE(A,B,C)

#else

#error "No scripting language defined!"

#endif


#endif
