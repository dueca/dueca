/* ------------------------------------------------------------------   */
/*      item            : dstypes.h
        made by         : Rene' van Paassen
        date            : 980219
        category        : header file
        description     : List of class names in dusime. May come in
                          handy sometimes. Is now used mainly as a garbage
                          bin for loose ends.
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef dstypes_h
#define dstypes_h

/** Macro that gives instant access to a reference of the Environment
    singleton */
#define CSE ((*Environment::getInstance()))

/** Different classes of Object. Needed when clearing out. */
enum ObjectType {
  O_Channel,       /** Communications channel end */
  O_Module,        /** Module, DUECA client */
  O_Dueca,         /** Object within DUECA's core */
  O_DuecaSupport,  /** Support object for DUECA, may later be cleared */
  O_CommAccessor,  /** Communications access */
  O_Entity         /** Entity, control of a collection of modules */
};

#endif






