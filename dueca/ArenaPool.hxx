/* ------------------------------------------------------------------   */
/*      item            : ArenaPool.hh
        made by         : Rene van Paassen
        date            : 010207
        category        : header file
        description     :
        changes         : 010207 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ArenaPool_hh
#define ArenaPool_hh

#include <map>
#include <sys/types.h>
using namespace std;

// forward declaration
#include <dueca_ns.h>
DUECA_NS_START
class Arena;

/** Specification of an Arena. */
struct ArenaSpec
{
  /** Size of the objects in the arena. */
  size_t object_size;

  /** Number of objects at default creation. */
  int num_objects;
};

/** This is a simple implementation for a pool of memory Arenas.

    This ArenaPool allocates a number of Arenas, starting with the
    smallest (8-byte) Arenas going up via powers of 2 (16, 32, 64,
    etc.) to an Arena of size max_size. It can be queried to return an
    appropriate-sized Arena. */
class ArenaPool
{

  /** Array with a pool of arenas. */
  map<size_t,Arena*> pool;

  /** Maximum size available in the arenas. */
  size_t             max_size;
public:

  /** Singleton object reference. */
  static ArenaPool& single();

  /** Constructor. This makes an ArenaPool. \param size gives an array
      with the required sized for the Arena's in the pool. This array
      is "taken over" by the arena pool, so don't destroy it during
      the lifetime of the pool. */
  ArenaPool(const ArenaSpec* size);

  /** Destructor. */
  ~ArenaPool();

  /** Return the Arena appropriate for objects of a certain size.
      Note that this is not super-efficient. The idea is that
      object-creating code finds a pointer to the proper Arena once,
      and then continues to use this pointer.
      \param size size of the object. */
  Arena* findArena(size_t size) const;

  /** Return the maximum size of objects that fit in the arenas of
      this pool. */
  inline size_t getMaxSize() const { return max_size;}
};
DUECA_NS_END

#define arena_pool DUECA_NS ::ArenaPool::single()

#endif
