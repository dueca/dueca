/* ------------------------------------------------------------------   */
/*      item            : debprint.h.in
        made by         : Rene van Paassen
        date            : 180214
        category        : header file
        description     : Helper for debug printing
        changes         : 180214 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifdef DEB
#undef DEB
#endif
#ifdef DEB0
#undef DEB0
#endif
#ifdef DEB1
#undef DEB1
#endif
#ifdef DEB2
#undef DEB2
#endif
#ifdef DEB3
#undef DEB3
#endif
#ifdef DEBPRINTLEVEL
#undef DEBPRINTLEVEL
#endif

#ifdef BUILD_DEBPRINT

#ifdef DEBDEF
#undef DEBDEF
#endif

#else

#ifdef DEBDEF
#error "DEBDEF defined?? Strange"
#endif

#endif


