/* ------------------------------------------------------------------   */
/*      item            : Arena.hh
        made by         : Rene' van Paassen
        date            : 001124
        category        : header file
        description     :
        changes         : 001124 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Arena_hh
#define Arena_hh

#include <iostream>
#include <sys/types.h>
using namespace std;
#include <dueca_ns.h>
#include <iostream>
#include <inttypes.h>
#include <DAtomics.hxx>

#define USE_BOOST_LOCKFREE

#ifdef USE_BOOST_LOCKFREE
#include <boost/lockfree/stack.hpp>
#endif

template <class E> class LockFreeLIFO;

DUECA_NS_START

/** Implementation of a memory arena, for fast allocation of
    fixed-size data blocks.

    This implementation is suitable for use in multi-threaded
    applications, it uses a lock-free stack to keep the reserve
    data. */
class Arena
{
#ifdef USE_BOOST_LOCKFREE

  /** Stack with free blocks */
  boost::lockfree::stack<void*> stack;

#else
  /** A 16-byte large placeholder with which the Arena is filled. */
  union PlaceHolder
  {
    /** Pointer to the next placeholder. */
    PlaceHolder* next;

    /** A double value to make sure its 8 bytes minimum and aligned. */
    double area;
  };

  /** The head of the placeholder array. */
  LockFreeLIFO<PlaceHolder> *stack;
#endif

  /** The size multiplier, for arenas of objects larger than 8 bytes. */
  int size_mult;

  /** the current number allocs from the arena. */
  atom_type<uint32_t>::type num_alloc;

  /** the current number of de-allocs from the arena */
  atom_type<uint32_t>::type num_dealloc;

  /** default block size for allocation. */
  atom_type<uint32_t>::type default_alloc;

  /** the current capacity of the Arena. */
  atom_type<uint32_t>::type memtotal_alloc;
public:
  /** Constructor, makes an arena
      \param object_size The size of individual objects
      \param default_alloc The initial size of the Arena, and also the
      size of additional blocks should they be needed. */
  Arena(int object_size, int default_alloc = 512);

  /** Allocate one block out of the Arena. */
  void* alloc(size_t sz);

  /** Release a previously allocated block. */
  void free(void* p);

  /** Return the maximum size an object in this arena can have. */
#ifdef USE_BOOST_LOCKFREE
  inline size_t getMaxObjectSize() const
  {
    if (default_alloc) return size_mult * sizeof(void*);
    else return 0x7fffffff;
  }
#else
  inline size_t getMaxObjectSize() const
  {if (default_alloc) return size_mult * sizeof(PlaceHolder);
    else return 0x7fffffff;}
#endif

  /** Print the alloc data to stream */
  std::ostream& print(std::ostream& os);

  //private:

  /** Destructor. This destructor is private to prevent programs from
      taking out the Arena, and thus pulling the rug from (possibly)
      under objects that have their bytes in the Arena. */
  ~Arena();

  /** An auxiliary routine that allocates and initializes a storage
      area. */
  void extendStorage();

  /** This class is a friend, to prevent compiler complaints from
      having no possibility to destroy an Arena. */
  friend class NoBody;
};

DUECA_NS_END

void* operator new(size_t sz, DUECA_NS ::Arena *a);

std::ostream& operator << (std::ostream& os, DUECA_NS ::Arena& a);

#endif
