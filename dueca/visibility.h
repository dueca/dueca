/* ------------------------------------------------------------------   */
/*      item            : visibility.hxx
        made by         : Rene van Paassen
        date            : 180321
        category        : header file
        description     :
        changes         : 180321 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef visibility_h
#define visibility_h

/** @file Support macros for visibility */

//https://gcc.gnu.org/wiki/Visibility

#if __GNUC__ >= 4 || defined(__clang__)
#define LNK_PUBLICC __attribute__ ((visibility ("default"))) __attribute__ ((constructor))
#define LNK_PUBLIC __attribute__ ((visibility ("default")))
#define LNK_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define LNK_PUBLICC __attribute__ ((constructor))
#define LNK_PUBLIC
#define LNK_LOCAL
#endif
#define LNK_WEAK __attribute__ ((weak))

#if (__GNUC__ >= 4 && __GNUC_MINOR >= 5) || __GNUC__ > 4
#define DUECA_DEPRECATED(A) \
  __attribute__((deprecated(A)))
#else
#define DUECA_DEPRECATED(A) \
  __attribute__((deprecated))
#endif

#endif
