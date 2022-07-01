/* ------------------------------------------------------------------   */
/*      item            : UCEntryDataCache.hxx
        made by         : Rene van Paassen
        date            : 141030
        category        : header file
        description     :
        changes         : 141030 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef UCEntryDataCache_hxx
#define UCEntryDataCache_hxx

#include <dassert.h>
#include <cstddef>
#include "dueca_ns.h"

DUECA_NS_START;

class AmorphReStore;

/** Single cache object, holding channel entry data coming in to be
    processed later. */
class UCRawDataCache
{
public:
  /** pointer type defined. */
  typedef UCRawDataCache* pointer_type;

private:
  /** contains the data */
  char*  data;

  /** size of the data */
  size_t       size;

  /** point to the next cache object */
  pointer_type next;

public:
  /** Constructor
      @param indata  raw, to-be-unpacked data
      @param offset  offset of the data in the array
      @param isize   size of the to be cached data */
  UCRawDataCache(const char* indata, size_t offset, size_t isize);

  /** Destructor */
  ~UCRawDataCache();

  /** Set link to next one */
  inline void setNext(pointer_type n) { assert(next == NULL); next = n; }

  /** get link to next one */
  inline pointer_type getNext() const { return next; }

  /** get the data */
  inline const char* getData() const { return data; }

  /** get the size */
  inline size_t getSize() const { return size; }
};

/** Cache object, holds data coming in for a channel entry when the
    channel entry has not yet been configured.
*/
class UCEntryDataCache
{
  /** Keep a list, head pointer */
  UCRawDataCache::pointer_type   head;

  /** tail pointer */
  UCRawDataCache::pointer_type   tail;

  /** object currently in a store */
  UCRawDataCache::pointer_type   instore;

public:
  /** Constructor */
  UCEntryDataCache();

  /** Destructor */
  ~UCEntryDataCache();

  /** add another piece of raw data to the list */
  void append(UCRawDataCache::pointer_type c);

  /** returns true if there is data to process */
  inline bool isNotEmpty() { return head != NULL;}

  /** return an unpacking device with the next piece of data. Consumes
      that data. */
  AmorphReStore getStore();
};

DUECA_NS_END;

#endif
