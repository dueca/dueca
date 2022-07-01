/* ------------------------------------------------------------------   */
/*      item            : Packer.hh
        made by         : Rene' van Paassen
        date            : 990611
        category        : header file
        description     :
        changes         : 990611 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Packer_hh
#define Packer_hh

#include "GenericPacker.hxx"
#include "UnifiedChannel.hxx"
#include <list>
using namespace std;
#include <dueca_ns.h>
DUECA_NS_START
class TimeSpec;
struct ParameterTable;
class AmorphStore;
class IPAccessor;

/** Object that packs messages from the channels into areas offered by
    the transport media accessor. */
class Packer: public GenericPacker
{
  /** Pointer to an array store objects used to pack the data. */
  AmorphStore* store;

  /** Store being handled currently. */
  int current_store;

  /** Number of stores available. */
  int no_of_stores;

  /** Helper function, packs one set of data into a store. */
  bool packOneSet(AmorphStore& store, const PackUnit& c);

public:
  SCM_FEATURES_DEF;

  /** Parameter table. */
  static const ParameterTable* getParameterTable();

public:
  /** Constructor. */
  Packer();

  /** completion, implicit. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Destructor. */
  virtual ~Packer();

  /** Run built-up notifications */
  virtual void packWork();

  /** Run built-up notifications */
  void packWork(AmorphStore& store);

  /** Initialising call, done by the media accessor. Give the areas
      available for packing, status flags for the areas, number of
      stores and the size of each store. */
  void initialiseStores(char** area, int* store_status,
                        int n_stores, int store_size);

  /** Stop packing, used at destruction time. */
  void stopPacking();

  /** Returns the store to send in the variable store_no,
      returns the fill level of the current store. */
  int changeCurrentStore(int& store_no);

  /** Print to stream, debugging. */
  friend ostream& operator << (ostream& os, const Packer& p);
};

DUECA_NS_END
#endif
