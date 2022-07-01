/* ------------------------------------------------------------------   */
/*      item            : FillPacker.hh
        made by         : Rene' van Paassen
        date            : 001023
        category        : header file
        description     :
        changes         : 001023 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef FillPacker_hh
#define FillPacker_hh

#include "GenericPacker.hxx"
#include "AsyncList.hxx"
#include <dueca_ns.h>
#include <dueca-conf.h>
#include "MessageBuffer.hxx"

DUECA_NS_START

class AmorphStore;
struct ParameterTable;

/** Object that will fill any excess storage space available on a
    transport medium to pack (possibly) large, low-priority messages. */
class FillPacker: public GenericPacker
{
  /** Pointer to an array store objects used to pack the data. */
  AmorphStore* store;

  /** Number of stores, always the same. */
  static const int no_of_stores;

  /** Number of the store to fill, and the one to send off. \{ */
  int store_to_fill, store_to_send;  /// \}

  /** Number of bytes to send off, index of the next byte to send. \{ */
  int bytes_to_send, index_to_send;  /// \}

  /** Size of the packing buffers. */
  int buffer_size;

  /** Helper function, packs one set of data into a store. */
  bool packOneSet(AmorphStore& s, const PackUnit& c);

#ifdef FILLPACKER_SEND_ID
  /** Counter for the number of packages sent, in debug mode */
  uint32_t pkg_count;
#endif

public:
  SCM_FEATURES_DEF;

  /** Return table with tunable parameters. */
  static const ParameterTable* getParameterTable();

public:
  /** Constructor. */
  FillPacker();

  /** Complete construction after all tunable parameters are set. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Destructor. */
  virtual ~FillPacker();

  /** Stop packing, used at destruction time. */
  void stopPacking() { };

  /** re-fill the empty space in a message. */
  //int stuffMessage(char* store, int size);

  /** re-fill the empty space in a message. */
  int stuffMessage(char* store, int size,
                   MessageBuffer::ptr_type buffer = NULL);

  /** Routine called by the IPAccessor, pack all the stuff left. */
  void packWork();
};

DUECA_NS_END
#endif
