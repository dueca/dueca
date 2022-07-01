/* ------------------------------------------------------------------   */
/*      item            : DDFFExceptions.cxx
        made by         : Rene' van Paassen
        date            : 211215
        category        : body file
        description     :
        changes         : 211215 first version
        language        : C++
        copyright       : (c) 2021 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DDFFExceptions_cxx
#include "DDFFExceptions.hxx"
#include <stdio.h>

DDFF_NS_START

file_read_error::file_read_error(unsigned long offset) :
  std::exception()
{
  snprintf(str, sizeof(str), "Read error with offset %lu", offset);
}

block_crc_error::block_crc_error(uint64_t offset, uint32_t size) :
  std::exception()
{
  snprintf(str, sizeof(str),
           "CRC failure for block at %#018lx, size %#010x", offset, size);
}


buffer_read_sequence_error::buffer_read_sequence_error
(unsigned stream_id, unsigned buffer_num,
 unsigned expected_buffer_num,
 unsigned offset, unsigned expected_offset) :
  std::exception()
{
  snprintf(str, sizeof(str),
           "Block reading sequence error stream %ud, expect #%ud, got #%ud,"
           " expect offset %#018x, got %#018x", stream_id,
           expected_buffer_num, buffer_num, expected_offset, offset);
}

replay_synchronization::
replay_synchronization(const char* entity, unsigned recid,
                       TimeTickType treq0,  TimeTickType treq1,
                       TimeTickType tres0,  TimeTickType tres1)
{
  snprintf(str, sizeof(str),
           "Entity %s, recorder %d timing request (%d,%d) result (%d,%d)",
           entity, recid, treq0, treq1, tres0, tres1);
}

data_recorder_configuration_error::
data_recorder_configuration_error(const char* detail) :
  std::exception(),
  str("data recorder configuration error: ")
{
  snprintf(str, sizeof(str), "Data recorder configuration error : %s", detail);
}

cannot_find_segment::cannot_find_segment(const char* entity, unsigned idx)
{
  snprintf(str, sizeof(str), "Segment %d not available in : %s", idx, entity);
}

tag_information_not_matching_recorders::tag_information_not_matching_recorders
(const char* entity, unsigned idx)
{
  snprintf(str, sizeof(str), "Segment %d, offset information does not match : %s", idx, entity);
}


DDFF_NS_END
