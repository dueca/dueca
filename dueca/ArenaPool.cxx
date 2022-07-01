/* ------------------------------------------------------------------   */
/*      item            : ArenaPool.cxx
        made by         : Rene' van Paassen
        date            : 010207
        category        : body file
        description     :
        changes         : 010207 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ArenaPool_cc
#include "ArenaPool.hxx"
#include "Arena.hxx"
#include <dassert.h>
DUECA_NS_START

ArenaPool& ArenaPool::single()
{
  static const ArenaSpec asize[] =
  {
   {16,   256},        // 16 kb for the smallest objects (min size?)
   {32,   256},        // 32 kb for the common ones
   {40,  1024},        // 40 bytes is UChannelEntryData size
   {64,   256},        // 16 kb for reasonable size
   {128,  256},        // common size
   {256,  128},        // 32 kb for biggies
   {512,   32},        // want more
   {768,   16},
   {1024,  16},        // and the same for smobs
   {1536,   8},        // some people (Dirk?) use really big objects
   {2048,   4},        // so this is for them
   {3072,   1},
   {4096,   1},
   {6144,   1},
   {8192,   1},        // Tomasz wants even bigger ...
   {12288,  1},
   {16384,  1},
   {0, 0}};            // close off

  static ArenaPool* singleton = new ArenaPool(asize);
  return *singleton;
}

ArenaPool::ArenaPool(const ArenaSpec* asize) :
  max_size(0)
{
  while (asize->object_size != 0) {

    // check that this size was not previously allocated
    if (pool.find(asize->object_size) != pool.end()) {
      cerr << "Incorrect size specification for the Arena pool"
           << "\nsize " << asize->object_size << " specified twice"
           << endl;
    }
    else {

      // create a new arena
      pool[asize->object_size] = new Arena(asize->object_size,
                                           asize->num_objects);

      // get an index of the maximum size in this pool
      if (asize->object_size > max_size) {
        max_size = asize->object_size;
      }
    }
    asize++;
  }

  // create a zero-size arena, for all things outside the pool
  pool[0x7fffffff] = new Arena(0, 0);
}

ArenaPool::~ArenaPool()
{
  // leave the arenas alone for now
}

Arena* ArenaPool::findArena(size_t size) const
{
  // get an iterator to the first element in the map. The map is
  // supposed to be sorted
  map<size_t, Arena*>::const_iterator ii = pool.begin();

  // find the arena with sufficient size
  for(; ii != pool.end() && ii->first < size; ii++);

  // have no code yet to handle this case
  if (ii == pool.end()) {
    cerr << "Could not find an arena for size " << size << endl;
  }
  assert(ii != pool.end());

  // return a pointer to the proper arena
  return ii->second;
}
DUECA_NS_END
