/* ------------------------------------------------------------------   */
/*      item            : dueca_ns.h
        made by         : Rene' van Paassen
        date            : 20010308
        category        : header file (in)
        description     : Defines the choice for namespace.
        changes         : 161124 RvP; make dueca always use namespace
        documentation   : in DUECA_API, but directly from configure
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** \file dueca_ns.h
    This file defines the namespace DUECA for dueca
    and dusime. It defines the following macros:

    <ul>
    <li>DUECA_NS_ON. If defined, this DUECA version uses namespace
    <li>DUECA_NS_START. Used in DUECA code to indicate the start of
    the namespace. Defined as "namespace dueca {".
    <li>DUECA_NS_END. Used in DUECA code to indicate the end of the
    namespace. Defined as "}".
    <li>DUECA_NS. The name of the DUECA namespace, "dueca"
    <li>CCDUECA_NS. Absolute DUECA namespace, "::dueca"
    <li>USING_DUECA_NS. Defined as "using namespace dueca;"
    <li>MSGPACKUS_NS_START. The namespace version for unpacking from
    msgpack. Defined as "namespace msgunpack {".
    <li>MSGPACKUS_NS_END. Defined as "}".
    <li>CCMSGPACKUS_NS. Absolute unpack namespace, "::msgunpack"
    </ul>
*/

#ifndef dueca_ns_h
#define dueca_ns_h

/** If defined, the dueca classes are encapsulated in the dueca
    namespace. */
#define DUECA_NS_ON 1

/** This is a shortcut for start of the namespace. It should only be
    used within dueca/dusime code, not in application code. */
#define DUECA_NS_START namespace dueca {
  /** This is a shortcut for the end  of the namespace. It should only be
      used within dueca/dusime code, not in application code. */
#define DUECA_NS_END } /* namespace dueca */
/** This defines the name of the dueca namespace. */
#define DUECA_NS dueca
/** This defines the "absolute" dueca namespace, starting with a
    double colon. */
#define CCDUECA_NS ::dueca
/** Use this define in your code, if you want to use the dueca
    namespace. */
#define USING_DUECA_NS using namespace dueca;

/** This is used internally in DUECA, for printing (operator<<) functions */
#define PRINT_NS_START namespace std {
/** End printing function. */
#define PRINT_NS_END }

/** A specific set of message-pack compatible routines for quick
    matching unpack. */
#define MSGPACKUS_NS_START namespace msgunpack {
/** End of the message-pack compatible routines namespace. */
#define MSGPACKUS_NS_END }
/** Absolute reference to message-pack compatible unpack routines */
#define CCMSGPACKUS_NS ::msgunpack

#endif
