/* ------------------------------------------------------------------   */
/*      item            : ControlBlock.hxx
        made by         : Rene van Paassen
        date            : 211217
        category        : header file
        description     :
        changes         : 211217 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ControlBlock_hxx
#define ControlBlock_hxx

#include <dueca/AmorphStore.hxx>
#include "DDFFMessageBuffer.hxx"
#include <iostream>
#include "ddff_ns.h"

DDFF_NS_START

/** @file ControlBlock.hxx, definition of control information in ddff files

    Each block has the following 28-byte header:

    - 8 byte signed integer, big endian, with the offset of the next
      block in the sequence of blocks with this ID, or -1, for the
      last block. When this is 0 (impossible number), the location of
      this block is not yet known

    - 2-byte unsigned integer crc checksum for the block; all bytes of
      the block's data area except the present are included in this
      checksum

    - 2-byte unsigned integer, big endian, indicates the stream id

    - 4-byte unsigned integer, big endian, indicates this block's
      size.

    - 4-byte unsigned integer, big endian, indicates this block's
      fill level, i.e., number of data bytes, including the 28 control
      bytes.

    - 4-byte unsigned integer, big endian, indicates the offset of this
      block's first started write. 0 if no write started in this block;
      for starting reading at any place

    - 4-byte unsigned integer, big endian, indicates the block number
*/

/** Size of ddff control blocks */
const size_t control_block_size = 28;

/** Write a control block at the start of a message buffer

    @param buffer      Buffer where the block is written.
    @param stream_id   Stream number.
    @param buffer_num  Number of the buffer in the stream.
*/
void control_block_write(DDFFMessageBuffer::ptr_type buffer,
                         uint16_t stream_id, unsigned buffer_num);

/** Object to decode a control block.

    The object's members contain the control block information.
*/
struct ControlBlockRead {

  /** Decoding store; helper object */
  AmorphReStore r;

  /** offset of the next block */
  int64_t   next_offset;

  /** checksum */
  uint16_t  checksum;

  /** stream id */
  uint16_t  stream_id;

  /** block size */
  uint32_t  block_size;

  /** block fill */
  uint32_t  block_fill;

  /** First decodable block */
  uint32_t  object_offset;

  /** block sequence number */
  uint32_t  block_num;

  /** Decode from only the header data, e.g. when inspecting a file

      @param header  Buffer with header data, minimum size 28 bytes.
  */
  ControlBlockRead(const char* header);

  /** Decode from buffer. Throws an exception when the checksum fails.

      @param buffer  Buffer with the data.
      @param offset  Offset in the file, used to complete the error
                     message on failure.
  */
  ControlBlockRead(DDFFMessageBuffer& buffer, std::ios::off_type offset);
};

DDFF_NS_END

#endif
