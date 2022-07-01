/* ------------------------------------------------------------------   */
/*      item            : UCEntryDataCache.cxx
        made by         : Rene' van Paassen
        date            : 141030
        category        : body file
        description     :
        changes         : 141030 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define UCEntryDataCache_cxx

#include "UCEntryDataCache.hxx"
#include "AmorphStore.hxx"
#include <cstring>

DUECA_NS_START;

UCRawDataCache::UCRawDataCache(const char* indata, size_t offset,
                                 size_t isize) :
  data(new char[isize]),
  size(isize),
  next(NULL)
{
  std::memcpy(data, &indata[offset], isize);
}

UCRawDataCache::~UCRawDataCache()
{
  delete [] data;
}

UCEntryDataCache::UCEntryDataCache() :
  head(NULL),
  tail(NULL),
  instore(NULL)
{
  //
}


UCEntryDataCache::~UCEntryDataCache()
{
  while (head != NULL) {
    UCRawDataCache::pointer_type tmp = head;
    head = head->getNext();
    delete tmp;
  }
  delete instore;
}

void UCEntryDataCache::append(UCRawDataCache::pointer_type c)
{
  if (tail) {
    tail->setNext(c);
    tail = c;
  }
  else {
    head = tail = c;
  }
}

AmorphReStore UCEntryDataCache::getStore()
{
  delete instore;
  instore = head; head = instore->getNext();
  return AmorphReStore(instore->getData(), instore->getSize());
}


DUECA_NS_END

