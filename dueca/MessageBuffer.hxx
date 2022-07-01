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

#ifndef MessageBuffer_hxx
#define MessageBuffer_hxx

#include <dueca_ns.h>
#include <inttypes.h>
#include <cstddef>
#include "DAtomics.hxx"

DUECA_NS_START;


/** Buffer struct */
struct MessageBuffer
{
  /** Define my own pointer type */
  typedef MessageBuffer* ptr_type;

  /** How is the data stored */
  typedef char value_type;

  /** Size of buffer */
  const size_t capacity;

  /** Offset of unpacking data start in the buffer; used by UDP transport
      for reserving control data. */
  size_t offset;

  /** Use of buffer for tier-1 data */
  size_t regular;

  /** Total use of the buffer */
  size_t fill;

  /** User count, if multiple accesses are needed for unpacking */
  atom_type<uint32_t>::type nusers;

  /** Origin of data */
  uint32_t origin;

  /** current message number */
  uint32_t message_cycle;

  /** Buffer itself */
  char *buffer;

  /** A count, identifying each buffer, for debug purposes */
  static unsigned creation_count;

  /** Identification */
  unsigned creation_id;

public:
  /** Constructor */
  MessageBuffer(size_t size, size_t offset = 0);

  /** Destructor */
  ~MessageBuffer();

  /** assignment operator */
  MessageBuffer& operator=(const MessageBuffer& o);

  /** Increment use */
  void claim();

  /** Decrement use, returns true if buffer is free */
  bool release();

  /** Interface for compatibility with msgpack */
  void write(const char* data, std::size_t size);

  /** Other access to the data */
  inline const char* data() const { return buffer; }

  /** Other access to the data */
  inline char* data() { return buffer; }

  /** Size for unpacking */
  inline size_t size() const { return fill; }

  /** Reset the meta information */
  void reset();

  /** Zero remainder, usually for cleanly writing */
  void zeroUnused();

private:
  /** Copy constructor */
  MessageBuffer(const MessageBuffer& o);
};

DUECA_NS_END;

#endif
