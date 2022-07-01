/* ------------------------------------------------------------------   */
/*      item            : dusime.h
        made by         : Rene van Paassen
        date            : 010319
        category        : header file
        description     :
        changes         : 010319 first version
                          041210 added comments
        language        : C++
        copyright       : (c) 2001 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** \file

    The dusime.h file includes the header files commonly used for a
    simulation module. Instead of including these header files
    separately, you can include this file.

    In most cases you want to include this file *twice*. One time in your
    header file. If you use template classes, you should include it a second
    time in the body file (cxx file), preceded with a define:
    \code
      #define DO_INSTANTIATE
      #include <dusime.h>
    \endcode
    With the DO_INSTANTIATE defined, the implementation of templated
    classes is generated.

    This file also switches on the dueca:: namespace. This behaviour is
    for compatibility reasons. If you do not want the dueca namespace,
    define
    \code
    #define NO_DUECA_NAMESPACE
    \endcode
    before calling this header
*/

// build forth on dueca.
#ifdef NO_DUECA_NAMESPACE
#define NO_DUECA_NAMESPACE_DEFINED
#else
#undef NO_DUECA_NAMESPACE
#endif
#include <dueca.h>

#if !defined(DO_INSTANTIATE)

// non-templated headers
#include <dusime/IncoTable.hxx>
#include <dusime/Snapshot.hxx>
#include <dusime/SimulationModule.hxx>
#include <dusime/HardwareModule.hxx>
#include <dusime/RTWModule.hxx>
#include <dusime/DataRecorder.hxx>

#else

// some sources that are only needed in the implementation
#undef DO_INSTANTIATE

// template implementation
#define DO_INSTANTIATE

#endif

#define CHECK_RECORDER(A) \
  if (! (A) .isValid() ) {                               \
    W_MOD(getId() << '/' << classname                    \
          << " recorder " #A << " not (yet) valid");     \
    res = false; \
  }

#ifdef NO_DUECA_NAMESPACE_DEFINED
#undef NO_DUECA_NAMESPACE_DEFINED
#else
USING_DUECA_NS;
#endif


