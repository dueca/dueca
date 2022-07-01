/* ------------------------------------------------------------------   */
/*      item            : ddff_ns.h
        made by         : Rene' van Paassen
        date            : 20010308
        category        : header file (in)
        description     : Defines the choice for namespace.
        changes         : 161124 RvP; make ddff always use namespace
        documentation   : in DUECA_API, but directly from configure
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** \file ddff_ns.h
    This file defines the namespace DDFF for dueca data format
    and dusime. It defines the following macros:

    <ul>
    <li>DDFF_NS_ON. If defined, this DUECA version uses namespace
    <li>DDFF_NS_START. Used in DUECA code to indicate the start of
    the namespace. Defined as "namespace dueca { namespace ddff {".
    <li>DDFF_NS_END. Used in DUECA code to indicate the end of the
    namespace. Defined as "}}".
    <li>DDFF_NS. The name of the DUECA namespace, "dueca:ddff"
    <li>USING_DDFF_NS. Defined as "using namespace dueca::ddff;"
    </ul>
*/

#ifndef ddff_ns_h
#define ddff_ns_h

namespace dueca { namespace ddff { } };

/** If defined, the dueca classes are encapsulated in the dueca
    namespace. */
#define DDFF_NS_ON 1

/** This is a shortcut for start of the namespace. It should only be
    used within dueca/dusime code, not in application code. */
#define DDFF_NS_START namespace dueca { namespace ddff {
  /** This is a shortcut for the end  of the namespace. It should only be
      used within dueca/dusime code, not in application code. */
#define DDFF_NS_END } } /* namespace dueca */
/** This defines the name of the dueca namespace. */
#define DDFF_NS dueca::ddff
/** This defines the "absolute" dueca namespace, starting with a
    double colon. */
#define CCDDFF_NS ::dueca::ddff
/** Use this define in your code, if you want to use the dueca
    namespace. */
#define USING_DDFF_NS using namespace dueca::ddff;


#endif
