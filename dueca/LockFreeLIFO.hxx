/* ------------------------------------------------------------------   */
/*      item            : LockFreeLIFO.hxx
        made by         : Rene van Paassen
        date            : 141110
        category        : header file
        description     : Asynchronous, multi-threaded queue,
                          for design sketch see doc/asyncqueuedesign.xoj
                          following
                          http://nedko.arnaudov.name/soft/L17_Fober.pdf
                          this is also in boost use that!
        changes         : 141110 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef LockFreeLIFO_hxx
#define LockFreeLIFO_hxx

#include <inttypes.h>
#include <iostream>
#include <iomanip>
#include "DAtomics.hxx"

#define LFLIFO_DEB(A)
#ifndef LFLIFO_DEB
#define LFLIFO_DEB(A) std::cout << A << std::endl;
#endif

/** Lock free last-in first out system, with ABBA protection */
template <class E>
class LockFreeLIFO
{
#if defined (__x86_64__)
  /** LIFO element */
  union LIFO_element
  {
    /** Atomic access 8 byte */
    typename atom_type<uint64_t>::type    arep;

    /** normal unsigned int */
    uint64_t                              rep;

    /** pointer representation */
    E*                                    head;

    /** Splitting off the count part / unused in common Linux/OSX pointers */
    struct
    {
      uint16_t _dum[3];
      uint16_t count;
    } d;

    /** Destructor */
    ~LIFO_element() { }

    /** Assignment, complete copy */
    LIFO_element& operator = (const LIFO_element& o)
    { this->rep = o.rep; return *this; }

    /** Constructor */
    LIFO_element(const LIFO_element& o) : rep(o.rep) { }

    /** Default empty */
    LIFO_element() : rep(0ULL) { }
  };

  /** Convert to the template's pointer type */
  inline E* getptr(LIFO_element t)
  {
    // restores data for the right pointer type
    t.d.count = (t.rep & 0x0000800000000000ULL) ? 0xffff : 0x0000;
    return t.head;
  };

#elif defined (__aarch64__)
  union LIFO_element
  {
    typename atom_type<uint64_t>::type    arep;
    uint64_t rep;
    E*       head;
    struct
    {
      uint16_t _dum[3];
      uint16_t count;
    } d;
    ~LIFO_element() { }
    LIFO_element& operator = (const LIFO_element& o)
    { this->rep = o.rep; return *this; }
    LIFO_element(const LIFO_element& o) : rep(o.rep) { }
    LIFO_element() : rep(0ULL) { }
  };

  inline E* getptr(LIFO_element t)
  {
    // assumes / works only in user space
    t.d.count = 0x0000;
    return t.head;
  };
#elif defined(__i686__) || defined(__i586__) || defined(__i386__) || defined(__arm__)
  union LIFO_element
  {
    typename atom_type<uint64_t>::type    arep;
    uint64_t rep;
    E*       head;
    struct
    {
      uint32_t _dum;
      uint32_t count;
    } d;
    ~LIFO_element() { }
    LIFO_element& operator = (const LIFO_element& o)
    { this->rep = o.rep; return *this; }
    LIFO_element(const LIFO_element& o) : rep(o.rep) { }
    LIFO_element() : rep(0ULL) { }
  };

  inline E* getptr(LIFO_element& t)
  {
    return t.head;
  };
#else
# error "not implemented/defined for this architecture"
#endif
  LIFO_element Lf;

public:
  /** Constructor

      Create a LIFO
  */
  LockFreeLIFO()
  {
    Lf.rep = 0ULL;
  }

  /** Get an element/pointer from the LIFO stack

      @returns E*   NULL if empty, otherwise an element.
   */
  E* pop()
  {
    E* res;
    while(1) {
      LIFO_element current = Lf; // atomic, grab the current top
      res = getptr(current);
      if (res == NULL) {
        LFLIFO_DEB("LIFO empty, returning NULL");
        return NULL;
      }
      LIFO_element next;
      next.head = res->next;
      next.d.count = current.d.count + 1;
      if (atomic_swap64(&Lf.arep, current.rep, next.rep)) {
        LFLIFO_DEB("returning " << std::hex << res << " top now 0x" <<
            next.rep << std::dec);
        break;
      }
      LFLIFO_DEB("swap failed in pop");
    }
    return res;
  };

  /** Push an element on the stack */
  void push(E* cell)
  {
    while(1) {
      LIFO_element current = Lf;
      LIFO_element next; next.head = cell; next.d.count = Lf.d.count;
      cell->next = getptr(current);
      if (atomic_swap64(&Lf.arep, current.rep, next.rep)) {
        LFLIFO_DEB("pushed " << std::hex << reinterpret_cast<void*>(cell) <<
            " to 0x" << Lf.rep << std::dec);
        return;
      }
      LFLIFO_DEB("swap failed in push");
    }
  }
};

#endif
