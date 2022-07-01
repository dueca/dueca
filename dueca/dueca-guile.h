/* ------------------------------------------------------------------   */
/*      item            : dueca_guile.h
        made by         : René van Paassen
        date            : 030428
        category        : header file
        description     : interface to different versions of the guile
                          family, 1.3.4, 1.4 and 1.6, currently
        changes         : added 1.8 (some time ago) and 2.0 (April 2012)
        language        : C++
        copyright       : (c) 2003 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#if !defined(scriptinterface_h)
#error "Only include dueca-guile.h through scriptinterface.h"
#endif

#ifndef dueca_guile_h
#define dueca_guile_h

#ifndef SCRIPT_SCHEME
#error "Only include dueca-guile.h if building for scheme"
#endif

#include <iostream>
#include "includeguile.h"

#if SCM_MAJOR_VERSION < 2 || \
  (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION == 0 && SCM_MICRO_VERSION < 11)
inline int scm_is_exact_integer(SCM s)
{
  return SCM_INUMP(s);
}
#endif
#if SCM_MAJOR_VERSION == 1 && SCM_MINOR_VERSION < 8
inline int scm_to_int(SCM s)
{
  return SCM_INUM(s);
}
inline int scm_is_real(SCM s)
{
  return SCM_NIMP(s) && SCM_REALP(s);
}
inline double scm_to_double(SCM s)
{
  return scm_num2dbl(s, "scm_to_real");
}
inline int scm_is_string(SCM s)
{
  return SCM_NIMP(s) && SCM_STRINGP(s);
}
#endif


#if (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION >= 2) || SCM_MAJOR_VERSION > 2
#define SCM_USE_FOREIGN
//#warning "needs proper testing!"
#endif

#if (SCM_MAJOR_VERSION == 1 && SCM_MINOR_VERSION >= 8) || SCM_MAJOR_VERSION >= 2

inline const char* dueca_scm_chars(const SCM& data)
{
  static char buf[2048] = { '\000' }; int l;
  if (scm_is_symbol(data)) {
    l = scm_to_locale_stringbuf(scm_symbol_to_string(data), buf, 2047);
  }
  else {
    l = scm_to_locale_stringbuf(data, buf, 2047);
  }
  if (l > 2047) {
    std::cerr << "Truncated scheme string of size " << l << std::endl;
  }
  else {
    buf[l] = '\000';
  }
  return buf;
}

#else
inline const char* dueca_scm_chars(const SCM& data)
{
  return SCM_CHARS(data);
}
#endif

//#if (SCM_MAJOR_VERSION == 1 && SCM_MINOR_VERSION >= 8) || (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION < 2)
#if !defined(SCM_USE_FOREIGN)

#ifndef SCM_SMOB_PREDICATE
#error SCM_SMOB_PREDICATE disappeared in this guile
#endif

inline SCM dueca_new_smob(scm_t_bits tag, void* obj)
{
  SCM smob;
  SCM_NEWSMOB(smob, tag, obj);
  return smob;
}

#ifndef SCM_SMOB_DATA
#error SCM_SMOB_DATA disappeared in this guile
#endif

#elif SCM_MAJOR_VERSION == 1 && SCM_MINOR_VERSION >= 6

#ifndef SCM_SMOB_PREDICATE
#error SCM_SMOB_PREDICATE disappeared in this guile
#endif

#ifndef SCM_SMOB_DATA
#error SCM_SMOB_DATA disappeared in this guile
#endif

inline SCM dueca_new_smob(scm_t_bits tag, void* obj)
{
  SCM smob;
  SCM_NEWCELL(smob);
  SCM_SETCDR(smob, reinterpret_cast<long int>(obj));
  SCM_SETCAR(smob, tag);

  return smob;
}

#elif SCM_MAJOR_VERSION == 1 && SCM_MINOR_VERSION < 6

// add the smob_predicate macro
#define SCM_SMOB_PREDICATE(A,B) \
  (A == SCM_CAR(B))

// scm_t_bits is introduced in 1.6.
// Otherwise we can do with a long
typedef long int scm_t_bits;

inline SCM dueca_new_smob(scm_t_bits tag, void* obj)
{
  SCM smob;
  SCM_NEWCELL(smob);
  SCM_SETCDR(smob, reinterpret_cast<long int>(obj));
  SCM_SETCAR(smob, tag);

  return smob;
}

#define scm_c_define_gsubr scm_make_gsubr

#ifndef SCM_SMOB_DATA
#define SCM_SMOB_DATA(S) \
SCM_CDR( S )
#endif // SCM_SMOB_DATA

#ifndef SCM_BOOLP
#define SCM_BOOLP(A) \
  (!SCM_NIMP(A) && (SCM_FALSEP(A) || SCM_NFALSEP(A)))
#endif

#endif

#if SCM_MAJOR_VERSION <= 1
typedef SCM (*scm_func)();
#else
typedef scm_t_subr scm_func;
#endif

#endif
