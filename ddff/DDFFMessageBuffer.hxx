/* ------------------------------------------------------------------   */
/*      item            : MessageBuffer.hxx
        made by         : Rene van Paassen
        date            : 161203
        category        : header file
        description     :
        changes         : 161203 first version
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        language        : C++
*/

#ifndef DDFFMessageBuffer_hxx
#define DDFFMessageBuffer_hxx

#include <dueca_ns.h>
#include <inttypes.h>
#include <cstddef>
#include "DAtomics.hxx"

DUECA_NS_START;


/** Buffer struct */
struct DDFFMessageBuffer
{
  /** Define my own pointer type */
  typedef DDFFMessageBuffer* ptr_type;

  /** How is the data stored */
  typedef char value_type;

  /** Size of buffer */
  const uint32_t capacity;

  /** Total use of the buffer */
  uint32_t fill;

  /** If applicable, offset of the first complete object stored in 
      the buffer, for reading, when starting somewhere in the block, 
      offset to put the read pointer. */
  uint32_t object_offset;

  /** Stream ID for this buffer */
  uint32_t stream_id;

  /** Buffer itself */
  char *buffer;

  /** A count, identifying each buffer, for debug purposes */
  static unsigned creation_count;

  /** Identification */
  unsigned creation_id;

public:
  /** Constructor */
  DDFFMessageBuffer(size_t size, size_t offset = 0);

  /** Destructor */
  ~DDFFMessageBuffer();

  /** assignment operator */
  DDFFMessageBuffer& operator=(const DDFFMessageBuffer& o);

  /** Interface for compatibility with msgpack */
  void write(const char* data, std::size_t size);

  /** Other access to the data */
  inline const char* data() const { return buffer; }

  /** Other access to the data */
  inline char* data() { return buffer; }

  /** Not completely full? */
  inline bool partial() { return fill < capacity; }

  /** Size for unpacking */
  inline size_t size() const { return fill; }

  /** Reset the meta information */
  void reset();

  /** Zero remainder, usually for cleanly writing */
  void zeroUnused();

  /** Get the current data pointer */
  inline const char* current() { return &buffer[object_offset]; }

private:
  /** Copy constructor */
  DDFFMessageBuffer(const DDFFMessageBuffer& o);
};

DUECA_NS_END;

#endif
