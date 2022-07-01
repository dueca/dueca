/* ------------------------------------------------------------------   */
/*      item            : AsyncList.cxx
        made by         : Rene' van Paassen
        date            : 010801
        category        : body file
        description     :
        changes         : 010801 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define AsyncList_cxx
#include <dueca-conf.h>
#include "AsyncList.hxx"
#include <iostream>
using namespace std;
#ifdef TEST_OPTIONS
//#define WRITE_MEMALLOC
#endif
#include <dassert.h>

DUECA_NS_START

#if 0
ChunkAllocator::ChunkAllocator(unsigned int nchunks, size_t size,
                               const char* name) :
  next(NULL),
  nchunks(nchunks > 0 ? nchunks : 1),
  size((size+sizeof(int)-1) / sizeof(int)),
  idx(this->nchunks),
  data(new int[this->size*this->nchunks]),
  name(name)
{
#ifdef WRITE_MEMALLOC
  cerr << "Chunk allocator " << name << '/'
       << reinterpret_cast<const void*>(name)
       << " bs=" << size << " is=" << this->size << " n=" << idx << endl;
#endif
}

ChunkAllocator::ChunkAllocator(ChunkAllocator* previous) :
  next(previous),
  nchunks(previous->nchunks),
  size(previous->size),
  idx(nchunks),
  data(new int[size*nchunks]),
  name(previous->name)
{
#ifdef WRITE_MEMALLOC
  cerr << "More chunks " << name << '/' << reinterpret_cast<const void*>(name)
       << " is=" << this->size << " n=" << idx << endl;
#endif
}


ChunkAllocator::~ChunkAllocator()
{
  delete next;
#ifdef WRITE_MEMALLOC
  cerr << "Deleting chunkallocator " << name << " size=" << this->size << " n="
       << (nchunks - idx) << '/' << nchunks << endl;
#endif
  delete [] data;
}

void* ChunkAllocator::getMore(ChunkAllocator *& chunks)
{
  assert(chunks == this);
  if (idx == 0)
    chunks = new ChunkAllocator(this);
  return reinterpret_cast<void*>(chunks->data + --(chunks->idx) * size);
}

#endif
DUECA_NS_END
