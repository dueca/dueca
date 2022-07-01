/* ------------------------------------------------------------------   */
/*      item            : dueca.h
        made by         : Rene van Paassen
        date            : 010319
        category        : header file
        description     :
        changes         : 010319 first version
                          041210 added comments
                          131225 switch on dueca namespace
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** \file

    The dueca.h file includes the header files commonly used for
    a module. Instead of including these header files separately, you
    can include this file.

    In most cases you want to include this file *twice*. One time in your
    header file. If you use template classes, you should include it a second
    time in the body file (cxx file), preceded with a define:
    \code
      #define DO_INSTANTIATE
      #include <dueca.h>
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

    Note that if you are using a dusime module, you should include the
    dusime.h file instead; it automatically chains in dueca.h
*/


#if !defined(DO_INSTANTIATE)
#include "dueca_ns.h"
#include "dueca-version.h"

DUECA_NS_START
struct ParameterTable;
DUECA_NS_END

// templated headers
#include "Callback.hxx"
#include "CallbackWithId.hxx"
#include "EventAccessToken.hxx"
#include "StreamAccessToken.hxx"
#include "MultiStreamReadToken.hxx"
#include "MultiStreamWriteToken.hxx"

// non-templated headers
#include "ChannelReadToken.hxx"
#include "ChannelWriteToken.hxx"
#include "Module.hxx"
#include "NameSet.hxx"
#include "AperiodicAlarm.hxx"
#include "PeriodicAlarm.hxx"
#include "Activity.hxx"
#include "ClockTime.hxx"

#include "DataReader.hxx"
#include "DataWriter.hxx"

#else

#undef DO_INSTANTIATE
// some sources that are only needed in the implementation
#include "ParameterTable.hxx"

// template implementation
#define DO_INSTANTIATE
#include "Callback.hxx"
#include "CallbackWithId.hxx"
#include "EventAccessToken.hxx"
#include "StreamAccessToken.hxx"
#include "MultiStreamReadToken.hxx"
#include "MultiStreamWriteToken.hxx"

// templated classes, only used in body files
#include "EventReader.hxx"
#include "EventWriter.hxx"
#include "StreamReader.hxx"
#include "StreamWriter.hxx"
#include "MultiStreamWriter.hxx"
#include "MultiStreamReader.hxx"
#include "StreamReaderLatest.hxx"

// check script types are selected
#if defined(SCRIPT_PYTHON) || defined(SCRIPT_SCHEME)
# include "TypeCreator.hxx"
#else
# ifndef NO_TYPE_CREATION
#  error "No scripting language was selected"
# endif
#endif
#include "VarProbe.hxx"
#include "MemberCall.hxx"
#include "Event.hxx"

#endif

#ifndef dueca_h_CHECK_MACROS
#define dueca_h_CHECK_MACROS

/** A macro that verifies the validity of a channel access token. If not
    valid, an error message is printed and the boolean variable res
    (which has to be created beforehand) is set to false.
    \param   A    Channel token to be checked. */
#define CHECK_TOKEN(A) \
  if (! (A) .isValid() ) {                              \
    W_MOD(getId() << '/' << getclassname(*this)         \
          << " channel token " #A " for channel "       \
          << (A) .getName() << " not (yet) valid");     \
    res = false; \
  }

/** A macro that verifies validity of any condition. If not
    valid, an error message is printed and the boolean variable res
    (which has to be created beforehand) is set to false.
    \param   A    Condition to be checked. */
#define CHECK_CONDITION(A) \
  if (! ( A ) ) { \
    W_MOD(getId() << '/' << getclassname(*this)         \
          << " condition " #A " not valid");            \
    res = false; \
  }

/** A macro that verifies validity of any condition. If not
    valid, an error message is printed and the boolean variable res
    (which has to be created beforehand) is set to false.
    \param   A    Condition to be checked.
    \param   B    Message that will be printed */
#define CHECK_CONDITION2(A,B) \
  if (! ( A ) ) {                                         \
    W_MOD(getId() << '/' << getclassname(*this) << ' ' << B); \
    res = false; \
  }

#endif

#ifndef NO_DUECA_NAMESPACE
USING_DUECA_NS;
#endif
