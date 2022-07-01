/* ------------------------------------------------------------------   */
/*      item            : ControlBlock.cxx
        made by         : Rene' van Paassen
        date            : 211217
        category        : body file
        description     :
        changes         : 211217 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ControlBlock_cxx
#include "ControlBlock.hxx"
#include <udpcom/CRCcheck.hxx>
#include <dueca/AmorphStore.hxx>
#include "DDFFExceptions.hxx"
#define DEBPRINTLEVEL -1
#include <debprint.h>

DDFF_NS_START;

void control_block_write(DDFFMessageBuffer::ptr_type buffer,
                         uint16_t stream_id, unsigned buffer_num)
{
  AmorphStore s(buffer->buffer, control_block_size);
  s.packData(int64_t(std::numeric_limits<int64_t>::max()));
  StoreMark<uint16_t> checksum_mark = s.createMark(uint16_t());
  s.packData(stream_id);
  s.packData(uint32_t(buffer->capacity));
  s.packData(uint32_t(buffer->fill));
  s.packData(uint32_t(buffer->object_offset));
  s.packData(uint32_t(buffer_num));

  // note; this uses the entire buffer; normally the remainder is zeroed
  uint16_t crc = crc16_ccitt(&(buffer->data()[10]), buffer->capacity-10);

  // debug print
  DEB1("stream " << stream_id << " buffer " << buffer_num << " fill " <<
       buffer->fill << " crc " << crc);
  s.finishMark(checksum_mark, crc);
}

ControlBlockRead::ControlBlockRead(const char* header) :
  r(header, control_block_size),
  next_offset(r),
  checksum(r),
  stream_id(r),
  block_size(r),
  block_fill(r),
  object_offset(r),
  block_num(r)
{
  //
}

ControlBlockRead::ControlBlockRead(DDFFMessageBuffer& buffer,
                                   std::ios::off_type offset) :
  r(buffer.data(), control_block_size),
  next_offset(r),
  checksum(r),
  stream_id(r),
  block_size(r),
  block_fill(r),
  object_offset(r),
  block_num(r)
{
  if (buffer.capacity < block_size) {
    throw buffer_too_small();
  }
  buffer.fill = block_fill;
  buffer.object_offset = object_offset;
  buffer.stream_id = stream_id;
  uint16_t crc = crc16_ccitt(&(buffer.data()[10]), buffer.capacity-10);
  DEB1("unpack " << stream_id << " buffer " << block_num << " fill " <<
       buffer.fill << " crc " << crc);
  if (checksum != crc) {
    throw block_crc_error(offset, buffer.fill);
  }
}


DDFF_NS_END;
