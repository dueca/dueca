/* ------------------------------------------------------------------   */
/*      item            : ChannelReadToken.cxx
        made by         : Rene' van Paassen
        date            : 140106
        category        : body file
        description     :
        changes         : 140106 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "GenericCallback.hxx"
#include "UCallbackOrActivity.hxx"
#include <cstddef>
#define ChannelReadToken_cxx
#include "ChannelReadToken.hxx"
#include "DataSetConverter.hxx"
#include <ChannelManager.hxx>
#include <DCOFunctor.hxx>
#include <DataClassRegistry.hxx>
#include <UCClientHandle.hxx>
#include <UnifiedChannel.hxx>
#include <dassert.h>
#include <debug.h>

DUECA_NS_START;

ChannelReadToken::ChannelReadToken(
  const GlobalId &owner, const NameSet &channelname,
  const std::string &dataclassname, const std::string &entrylabel,
  Channel::EntryTimeAspect time_aspect, Channel::EntryArity arity,
  Channel::ReadingMode rmode, double requested_span,
  const UCallbackOrActivity &when_valid) :
  GenericToken(owner, channelname, dataclassname),

  handle(NULL),
  arity(arity)
{
  // finds or creates the channel. Id request comes later in constructor
  channel = ChannelManager::single()->findOrCreateChannel(this->getName());

  // read token with optional entry match
  handle =
    channel->addReadToken(this, dataclassname, entrylabel,
                          isSingleEntryOption(arity) ? 0xfffe : 0xffff,
                          time_aspect, rmode, when_valid, requested_span);
}

ChannelReadToken::ChannelReadToken(
  const GlobalId &owner, const NameSet &channelname,
  const std::string &dataclassname, entryid_type attach_entry,
  Channel::EntryTimeAspect time_aspect, Channel::EntryArity arity,
  Channel::ReadingMode rmode, double requested_span,
  const UCallbackOrActivity &when_valid) :
  GenericToken(owner, channelname, dataclassname),
  handle(NULL),
  arity(arity)
{
  channel = ChannelManager::single()->findOrCreateChannel(this->getName());
  handle =
    channel->addReadToken(this, dataclassname, "", attach_entry, time_aspect,
                          rmode, when_valid, requested_span);
}

// deprecated variant
ChannelReadToken::ChannelReadToken(
  const GlobalId &owner, const NameSet &channelname,
  const std::string &dataclassname, const std::string &entrylabel,
  Channel::EntryTimeAspect time_aspect, Channel::EntryArity arity,
  Channel::ReadingMode rmode, double requested_span,
  Channel::TransportClass tclass, GenericCallback *when_valid) :
  GenericToken(owner, channelname, dataclassname),

  handle(NULL),
  arity(arity)
{
  // finds or creates the channel. Id request comes later in constructor
  channel = ChannelManager::single()->findOrCreateChannel(this->getName());

  // read token with optional entry match
  handle =
    channel->addReadToken(this, dataclassname, entrylabel,
                          isSingleEntryOption(arity) ? 0xfffe : 0xffff,
                          time_aspect, rmode, when_valid, requested_span);
}

// deprecated variant
ChannelReadToken::ChannelReadToken(
  const GlobalId &owner, const NameSet &channelname,
  const std::string &dataclassname, entryid_type attach_entry,
  Channel::EntryTimeAspect time_aspect, Channel::EntryArity arity,
  Channel::ReadingMode rmode, double requested_span,
  Channel::TransportClass tclass, GenericCallback *when_valid) :
  GenericToken(owner, channelname, dataclassname),
  handle(NULL),
  arity(arity)
{
  channel = ChannelManager::single()->findOrCreateChannel(this->getName());
  handle =
    channel->addReadToken(this, dataclassname, "", attach_entry, time_aspect,
                          rmode, when_valid, requested_span);
}

ChannelReadToken::ChannelReadToken(
  const GlobalId &owner, const NameSet &channelname,
  const std::string &dataclassname, entryid_type attach_entry,
  Channel::EntryTimeAspect time_aspect, Channel::EntryArity arity,
  Channel::ReadingMode rmode, const UCallbackOrActivity &when_valid,
  unsigned requested_depth) :
  GenericToken(owner, channelname, dataclassname),
  handle(NULL),
  arity(arity)
{
  channel = ChannelManager::single()->findOrCreateChannel(this->getName());
  handle =
    channel->addReadToken(this, dataclassname, "", attach_entry, time_aspect,
                          rmode, when_valid, 0.0, requested_depth);
}

ChannelReadToken::~ChannelReadToken() { channel->removeReadToken(handle); }

void ChannelReadToken::selectFirstEntry() { channel->selectFirstEntry(handle); }

const GlobalId &ChannelReadToken::getChannelId() const
{
  return channel->getId();
}

void ChannelReadToken::selectNextEntry() { channel->getNextEntry(handle); }

bool ChannelReadToken::isSequential() const
{
  return handle->entry->isSequential();
}

bool ChannelReadToken::haveEntry() const { return handle->entry != NULL; }

const entryid_type ChannelReadToken::getEntryId() const
{
  return handle->entry ? handle->entry->entry->getId() : entry_end;
}

const std::string &ChannelReadToken::getEntryLabel() const
{
  const static std::string nolabel;
  return handle->entry ? handle->entry->entry->getLabel() : nolabel;
}

bool ChannelReadToken::isValid()
{
  return channel->readTokenIsValid(handle) || isZeroEntriesAcceptable(arity);
}

DataTimeSpec ChannelReadToken::getOldestDataTime() const
{
  return channel->getOldestDataTime(handle);
}

DataTimeSpec ChannelReadToken::getLatestDataTime() const
{
  return channel->getLatestDataTime(handle);
}

void ChannelReadToken::addTarget(
  const boost::intrusive_ptr<TriggerTarget> &target, unsigned id)
{
  // TODO: extend to remember added and removed targets, and remove these
  // from the handle at destruction
  // target->setTrigger(*this);
  TriggerPuller::addTarget(target, id);
  if (this->targets.size() == 1) {
    handle->addPuller(this);
    channel->incrementVersion();
  }
}

const void *ChannelReadToken::getAccess(TimeTickType t_request,
                                        DataTimeSpec &ts_actual,
                                        GlobalId &origin, uint32_t magic)
{
  if (magic != magic_number) {
    throw ChannelWrongDataType(getChannelId(), getTokenHolder());
  }
  return channel->getReadAccess(handle, t_request, origin, ts_actual);
}

void ChannelReadToken::releaseAccess(const void *data_ptr)
{
  assert(data_ptr != NULL);
  channel->releaseReadAccess(handle);
}

void ChannelReadToken::releaseAccessKeepData(const void *data_ptr)
{
  channel->releaseReadAccessKeepData(handle);
}

ChannelReadToken::AccessResult
ChannelReadToken::readAndStoreData(AmorphStore &s, TimeTickType &tsprev)
{
  GlobalId origin;
  DataTimeSpec ts_actual;
  const void *data =
    channel->getReadAccess(handle, MAX_TIMETICK, origin, ts_actual);

  if (!data) {
    assert(handle->accessed == NULL);
    return NoData;
  }

  /* if the timespec start matches tsprev, and this is stream data,
     code only the time end, otherwise code a complete timespec. For
     event always code time point */
  try {
    if (tsprev != ts_actual.getValidityStart() &&
        !handle->entry->entry->isEventType()) {
      ::packData(s, ts_actual);
      converter->packData(s, data);
      tsprev = ts_actual.getValidityEnd();
      channel->releaseReadAccess(handle);
      assert(handle->accessed == NULL);
      return TimeSkip;
    }
    else {
      ::packData(s, ts_actual.getValidityEnd());
      converter->packData(s, data);
      tsprev = ts_actual.getValidityEnd();
      channel->releaseReadAccess(handle);
      assert(handle->accessed == NULL);
      return DataSuccess;
    }
  }
  catch (const AmorphStoreBoundary &e) {
    // channel->releaseReadAccess(handle); // this is wrong,
    channel->resetReadAccess(handle);

    handle->accessed = NULL; //->resetDataAccess(handle);
    // revert the read index to previous element? Will this match with
    // gap?
    // handle->entry->read_index = handle->entry->read_index->getPrevious();
    throw(e);
  }
  // assert(handle->accessed == NULL);
  // return DataSuccess;
}

bool ChannelReadToken::readAndPack(AmorphStore &s, DataTimeSpec &ts_actual,
                                   const TimeSpec &tsreq)
{
  GlobalId origin;
  const void *data =
    channel->getReadAccess(handle, tsreq.getValidityEnd(), origin, ts_actual);

  if (!data) {
    assert(handle->accessed == NULL);
    return false;
  }

  /* pack the data, in case of failure tweak the handle access and
     return the exception */
  try {
    converter->packData(s, data);
    channel->releaseReadAccess(handle);
  }
  catch (const AmorphStoreBoundary &e) {
    channel->resetReadAccess(handle);
    handle->accessed = NULL; //->resetDataAccess(handle);
    // revert the read index to previous element? Will this match with
    // gap?
    // handle->entry->read_index = handle->entry->read_index->getPrevious();
    throw(e);
  }

  // reward if we got to here
  return true;
}

bool ChannelReadToken::applyFunctor(DCOFunctor *fnct, TimeTickType time)
{
  GlobalId origin;
  DataTimeSpec ts_actual;
  bool res = true;
  const void *data = channel->getReadAccess(handle, time, origin, ts_actual);
  if (!data) {
    assert(handle->accessed == NULL);
    return false;
  }

  try {
    res = (*fnct)(data, ts_actual);
    channel->releaseReadAccess(handle);
  }
  catch (const std::exception &e) {
    channel->resetReadAccess(handle);
    handle->accessed = NULL;
    throw(e);
  }
  return res;
}

unsigned int ChannelReadToken::getNumVisibleSets(const TimeTickType ts) const
{
  return channel->getNumVisibleSets(handle, ts);
}

bool ChannelReadToken::haveVisibleSets(const TimeTickType ts) const
{
  return channel->haveVisibleSets(handle, ts);
}

unsigned int
ChannelReadToken::getNumVisibleSetsInEntry(const TimeTickType ts) const
{
  return channel->getNumVisibleSetsInEntry(handle, ts);
}

bool ChannelReadToken::haveVisibleSetsInEntry(const TimeTickType ts) const
{
  return channel->haveVisibleSetsInEntry(handle, ts);
}

unsigned int ChannelReadToken::flushTotalAvailableSets() const
{
  unsigned nflushed = 0;
  channel->selectFirstEntry(handle);
  while (handle->entry) {
    if (handle->entry->isSequential()) {
      nflushed += handle->entry->entry->flushAll(handle);
    }
    channel->getNextEntry(handle);
  }
  channel->selectFirstEntry(handle);
  return nflushed;
}

unsigned int ChannelReadToken::flushOlderSets() const
{
  unsigned nflushed = 0;
  channel->selectFirstEntry(handle);
  while (handle->entry) {
    if (handle->entry->isSequential()) {
      nflushed += handle->entry->entry->spinToLast(handle);
    }
    channel->getNextEntry(handle);
  }
  channel->selectFirstEntry(handle);
  return nflushed;
}

unsigned int ChannelReadToken::flushOlderSets(TimeTickType ts) const
{
  unsigned nflushed = 0;
  channel->selectFirstEntry(handle);
  while (handle->entry) {
    if (handle->entry->isSequential()) {
      nflushed += handle->entry->entry->spinToLast(handle, ts);
    }
    channel->getNextEntry(handle);
  }
  channel->selectFirstEntry(handle);
  return nflushed;
}

unsigned int ChannelReadToken::flushOne() const
{
  channel->selectFirstEntry(handle);
  while (handle->entry) {
    if (handle->entry->isSequential() &&
        handle->entry->entry->flushOne(handle)) {
      return 1U;
    }
    channel->getNextEntry(handle);
  }
  channel->selectFirstEntry(handle);
  return 0U;
}

ChannelEntryInfo ChannelReadToken::getChannelEntryInfo() const
{
  if (handle->entry) {
    return ChannelEntryInfo(handle->entry->entry);
  }
  else {
    return ChannelEntryInfo();
  }
}

DUECA_NS_END;
