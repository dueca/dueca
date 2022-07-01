/* ------------------------------------------------------------------   */
/*      item            : UChannelEntryData.cxx
        made by         : Rene' van Paassen
        date            : 141021
        category        : body file
        description     :
        changes         : 141021 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define UChannelEntryData_cxx

#include "UChannelEntryData.hxx"
#include <dassert.h>
#include "DataSetConverter.hxx"
#include "Arena.hxx"
#include "ArenaPool.hxx"

#define DEBPRINTLEVEL -1
#include "debprint.h"

DUECA_NS_START

void* UChannelEntryData::operator new(size_t size)
{
  static Arena* my_arena = arena_pool.findArena
    (sizeof(UChannelEntryData));
  return my_arena->alloc(size);
}

void UChannelEntryData::operator delete(void* v)
{
  static Arena* my_arena = arena_pool.findArena
    (sizeof(UChannelEntryData));
  my_arena->free(v);
}

UChannelEntryData::UChannelEntryData(const TimeTickType& ts,
                                     UChannelEntryData* current) :
  time_or_time_end(ts),
  data(NULL),
  older(current),
  newer(NULL),
  read_accesses(1),
  seq_id(current->seq_id + 1)
{
  // note that this constructor should always be called with a
  // non-NULL current. If current is NULL or a specific sequence id is
  // set, the other constructor is to be used.
  current->newer = this;
  DEB("seq #" << seq_id << " new");
}

UChannelEntryData::UChannelEntryData(const TimeTickType& ts,
                                     const void *data,
                                     UChannelEntryData* current,
                                     uchan_seq_id_t seq_id) :
  time_or_time_end(ts),
  data(data),
  older(current),
  newer(NULL),
  read_accesses(1),
  seq_id(seq_id)
{
  if (current) current->newer = this;
  DEB("seq #" << seq_id << " new with data");
}



UChannelEntryData::~UChannelEntryData()
{
  //
}

// this call is only executed for stream data
UChannelEntryData* UChannelEntryData::tryDeleteData
(const TimeTickType tick, const DataSetConverter* converter)
{
  if (tick > time_or_time_end &&
      atomic_swap32(&read_accesses, uint32_t(1), uint32_t(0))) {

    DEB("seq #" << seq_id << " trydelete");
    // remove the data and mark this.
    converter->delData(data);
    data = NULL;
    return newer;
  }
  // the read_accesses counter is left at 0
  DEB("seq #" << seq_id << " trydelete fail " << read_accesses);
  return NULL;
}

bool UChannelEntryData::finalDeleteData(UChannelEntryDataPtr& pnt,
                                        const DataSetConverter* converter)
{
  if (atomic_swap32(&(pnt->read_accesses),
                    uchan_seq_id_t(1), uchan_seq_id_t(0))) {

    DEB("seq #" << pnt->seq_id << " finaldelete");

    // remove the data and mark this.
    converter->delData(pnt->data);
    UChannelEntryData* n = pnt->newer;
    delete pnt;
    pnt = n;
    return (pnt != NULL);
  }
  return false;
}

const void* UChannelEntryData::getData(DataTimeSpec& ts_actual,
                                       UCClientHandlePtr client,
                                       bool eventtype)
{
  assert(client->accessed == NULL);
  if (!newer) return NULL;
  uint32_t ra = read_accesses;
  while (ra) {
    if (atomic_swap32(&read_accesses, ra, ra+1)) {
      if (eventtype) {
        ts_actual = DataTimeSpec(time_or_time_end, time_or_time_end);
      }
      else {
        ts_actual = DataTimeSpec(time_or_time_end, newer->time_or_time_end);
      }
      client->accessed = this;
      DEB("seq #" << seq_id << " getData, increment to" << ra+1);
      return data;
    }
    ra = read_accesses;
  }
  return NULL;
}

const void* UChannelEntryData::monitorGetData(DataTimeSpec& ts_actual,
                                              bool eventtype)
{
  if (!newer) return NULL;
  uint32_t ra = read_accesses;
  while (ra) {
    if (atomic_swap32(&read_accesses, ra, ra+1)) {
      if (eventtype) {
        ts_actual = DataTimeSpec(time_or_time_end, time_or_time_end);
      }
      else {
        ts_actual = DataTimeSpec(time_or_time_end, newer->time_or_time_end);
      }
      DEB("seq #" << seq_id << " getData, increment to" << ra+1);
      return data;
    }
    ra = read_accesses;
  }
  return NULL;
}

const void* UChannelEntryData::getSequentialData(DataTimeSpec& ts_actual,
                                                 UCClientHandlePtr client,
                                                 bool eventtype)
{
  assert(client->accessed == NULL);
  assert(newer != NULL);
  assert(read_accesses > 1);
  if (eventtype) {
    ts_actual = DataTimeSpec(time_or_time_end, time_or_time_end);
  }
  else {
    ts_actual = DataTimeSpec(time_or_time_end, newer->time_or_time_end);
  }
  client->accessed = this;
  DEB("seq #" << seq_id << " getSequentialData, at " << read_accesses);
  return data;
}

void UChannelEntryData::returnData(UCClientHandlePtr client)
{
  assert(client->accessed == this);
  client->accessed = NULL;
  atomic_decrement32(read_accesses);
  DEB("seq #" << seq_id << " return data, decrement");
}

void UChannelEntryData::monitorReturnData()
{
  atomic_decrement32(read_accesses);
  DEB("seq #" << seq_id << " monitor return data, decrement");
}

void UChannelEntryData::resetDataAccess(UCClientHandlePtr client)
{
  assert(client->accessed == this);
  client->accessed = NULL;
  if (!client->entry->isSequential()) {
    atomic_decrement32(read_accesses);
    DEB("seq #" << seq_id << " reset data access, decrement");
  }
}

void UChannelEntryData::assumeData(UCClientHandlePtr client)
{
  assert(client->accessed == this);
  client->accessed = NULL;
  data = NULL;
  atomic_decrement32(read_accesses);
  DEB("seq #" << seq_id << " assume data, decrement");
}

DUECA_NS_END

