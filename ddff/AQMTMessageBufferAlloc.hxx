/* ------------------------------------------------------------------   */
/*      item            : AQMTMessageBufferAlloc.hxx
        made by         : Rene van Paassen
        date            : 211217
        category        : header file
        description     :
        changes         : 211217 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef AQMTMessageBufferAlloc_hxx
#define AQMTMessageBufferAlloc_hxx

#include "ddff_ns.h"
#include "DDFFMessageBuffer.hxx"
#include <new>

DDFF_NS_START

/** Specific allocator for MessageBuffer objects in async queue list.

    MessageBuffer objects are used to gather written data and to store
    read data for communication between the DDFF FileHandler and reading/
    writing streams.

    MessageBuffer variables are re-used in the following manner:

    - capacity; normal use, indicating the size of the MessageBuffer data
      area

    - offset; record the location in the buffer where the first complete
      object is written, so reading somewhere halfway can start there.

    - regular; used as a flag, to indicate when a buffer has been
      previously partly written, and is now written again, at the
      corresponding location.

    - fill; indicate the fill level of the buffer

    - origin; contains the stream id

    - message_cycle;
      * when reading, contains the buffer number in the stream
      * when writing, is zero, unless this buffer contains the start of
        the first data in a recording stretch (Advance), then it
        contains the offset of that data within the buffer. At buffer
        writing for tagged streams, this will be added to the buffer
        base offset, and recorded in the tags stream (#1).
*/
struct AQMTMessageBufferAlloc
{
  /** Size of the buffers, needed for the allocation of data space */
  unsigned bufsize;

  /** Single elements of data. */
  class Element
  {
  public:
    /** Pointer to the next element in the list. */
    Element * volatile next;

    /** Data of the element. */
    DDFFMessageBuffer data;

    /** Constructor. */
    inline Element(Element* next, unsigned bufsize) :
      next(next), data(bufsize) {  }

    /** Destructor. */
    inline ~Element() { }
  };

  /** Type definition */
  typedef Element element_type;

  /** Type definition */
  typedef Element* element_ptr;

  /** Create a list element with data */
  inline Element* operator () (element_ptr next=NULL)
  { return new Element(next, bufsize); }

  /** Create a list element with data on recycled memory, with
      placement new */
  inline Element* operator () (void* mem, element_ptr next=NULL)
  { element_ptr res = reinterpret_cast<element_ptr>(mem);
    res->data.reset();
    res->next = next;
    return res; }

  /** Constructor */
  AQMTMessageBufferAlloc(): bufsize(4096U) {}
};

DDFF_NS_END

#endif
