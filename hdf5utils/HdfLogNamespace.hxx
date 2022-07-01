/* ------------------------------------------------------------------   */
/*      item            : HdfLogNamespace.hxx
        made by         : repa
        date            : Mon Jan 30 21:42:36 2017
        category        : header file
        description     :
        changes         : Mon Jan 30 21:42:36 2017 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2 - Rene van Paassen
*/

#ifndef HdfLogNamespace_hxx
#define HdfLogNamespace_hxx

#define STARTHDF5LOG namespace dueca { namespace hdf5log {
#define ENDHDF5LOG } }

STARTHDF5LOG;
class HDF5Logger;
class EntryWatcher;
ENDHDF5LOG;

//#define REP_DEBUG(A) std::cerr << A << std::endl;
#ifndef REP_DEBUG
#define REP_DEBUG(A)
#endif

#endif
