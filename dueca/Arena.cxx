/* ------------------------------------------------------------------   */
/*      item            : Arena.cxx
        made by         : Rene' van Paassen
        date            : 001124
        category        : body file
        description     :
        changes         : 001124 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define Arena_cc

#include "Arena.hxx"
#include "LockFreeLIFO.hxx"

#include "dueca-conf.h"
#ifdef TEST_OPTIONS
#define I_MEM
#endif
#include "debug.h"
#include <dassert.h>
#include <iostream>
#include <cstdlib>
using namespace std;
#define KEEP_COUNT


DUECA_NS_START

Arena::Arena(int object_size, int default_alloc) :
#ifdef USE_BOOST_LOCKFREE
  stack(1),
  size_mult((object_size - 1)/sizeof(void*) + 1),
#else
  stack(new LockFreeLIFO<PlaceHolder>()),
  size_mult((object_size - 1)/sizeof(PlaceHolder) + 1),
#endif
  num_alloc(0),
  num_dealloc(0),
  default_alloc(default_alloc),
  memtotal_alloc(0)
{
  extendStorage();
}

Arena::~Arena()
{
  // do nothing
}

void Arena::extendStorage()
{
  // cannot extend on a haphazard storage
  if (!default_alloc) return;

#ifdef USE_BOOST_LOCKFREE
  stack.reserve(default_alloc);
  for (int ii = default_alloc; ii--; ) {
    stack.push(new void*[size_mult]);
  }
#else
  // block allocate room for the objects
  PlaceHolder* to_return = new PlaceHolder[default_alloc*size_mult];

  // initalize the pointers
  for (int ii = default_alloc; ii--; ) {
    stack->push(&to_return[ii*size_mult]);
  }
#endif

  atomic_add32(memtotal_alloc, atomic_access(default_alloc));

#ifdef I_MEM_ACTIVE
  /* DUECA memory.

     Information on additional memory needed for reclaimable objects
     of a specific size. */
  I_MEM("Extended the \"" << size_mult*sizeof(PlaceHolder) <<
          "\" arena to " << memtotal_alloc);
#endif
}

void* Arena::alloc(size_t sz)
{
  if (default_alloc) {

#ifdef USE_BOOST_LOCKFREE
    void* res;
    while (!stack.pop(res)) {
      extendStorage();
    }
#ifdef KEEP_COUNT
    atomic_increment32(num_alloc);
#endif
    return res;

#else
    PlaceHolder* p = stack->pop();
    while (p == NULL) {
      extendStorage();
      p = stack->pop();
    }
#ifdef KEEP_COUNT
    atomic_increment32(num_alloc);
#endif
    return reinterpret_cast<void*>(p);
#endif
  }
  else {
    return ::malloc(sz);
  }
}

void Arena::free(void* p)
{
  if (default_alloc) {
    assert(p != NULL);
#ifdef USE_BOOST_LOCKFREE
    stack.push(p);
#else
    stack->push(reinterpret_cast<PlaceHolder*>(p));
#endif
#ifdef KEEP_COUNT
    atomic_increment32(num_dealloc);
#endif
  }
  else {
    ::free(p);
  }
}

std::ostream& Arena::print(std::ostream& os)
{
#if 0
  // the arena should NOT be operational now
  LockFreeLIFO<PlaceHolder> tmpstack;

  uint32_t count1 = 0, count2 = 0;
  PlaceHolder* p = stack->pop();
  while (p != NULL) {
    tmpstack.push(p);
    p = stack->pop();
    count1++;
  }
  p = tmpstack.pop();
  while (p != NULL) {
    stack->push(p);
    p = tmpstack.pop();
    count2++;
  }
  std::cerr << count1 <<  ' ' << count2 << std::endl;
  assert(count1 == count2);
#endif

  return os << "Arena(size_mult=" << size_mult
            << ", num_alloc=" << num_alloc
            << ", num_dealloc=" << num_dealloc
            << ", default_alloc=" << default_alloc
            << ", memtotal_alloc=" << memtotal_alloc
            << ")";
}

DUECA_NS_END

void* operator new(size_t sz, DUECA_NS ::Arena *a)
{
  assert(sz <= a->getMaxObjectSize());
  return a->alloc(sz);
}

std::ostream& operator << (std::ostream& os, DUECA_NS ::Arena& a)
{
  return a.print(os);
}
