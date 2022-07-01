/* ------------------------------------------------------------------   */
/*      item            : ChannelWriteToken.cxx
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

#define ChannelWriteToken_cxx
#include "ChannelWriteToken.hxx"
#include <ChannelManager.hxx>
#include <UnifiedChannel.hxx>
#include "DataClassRegistry.hxx"
#include "DataSetConverter.hxx"
#include <UCClientHandle.hxx>
#include <DCOFunctor.hxx>
#include <debug.h>

DUECA_NS_START;

ChannelWriteToken::ChannelWriteToken(const GlobalId& owner,
                                     const NameSet& channelname,
                                     const std::string& dataclassname,
                                     const std::string& entrylabel,
                                     Channel::EntryTimeAspect time_aspect,
                                     Channel::EntryArity arity,
                                     Channel::PackingMode packmode,
                                     TransportClass tclass,
                                     GenericCallback *when_valid,
                                     unsigned nreservations) :
  GenericToken(owner, channelname, dataclassname),
  handle(NULL)
{
  channel = ChannelManager::single()->findOrCreateChannel(this->getName());

  /* DUECA Channels

     Informational message on the issuing of a new write token.
   */
  I_CHN("write token for " << owner << " on " << channelname);
  handle = channel->addWriteToken(this, dataclassname,
                                  isTimeAspectEvent(time_aspect),
                                  isSingleEntryOption(arity),
                                  nreservations,
                                  isFullPacking(packmode),
                                  tclass,
                                  entrylabel, when_valid);
}



ChannelWriteToken::~ChannelWriteToken()
{
  channel->removeWriteToken(handle);
}

bool ChannelWriteToken::isValid()
{
  return handle->entry->isValid();
}

void* ChannelWriteToken::getAccess(uint32_t magic)
{
  if (!isValid() || magic != magic_number) {
    throw InvalidChannelAccessReturn(getChannelId(), getTokenHolder());
  }
  return handle->entry->getDataSpace();
}

void ChannelWriteToken::checkAccess(uint32_t magic)
{
  if (!isValid() || magic != magic_number) {
    throw InvalidChannelAccessReturn(getChannelId(), getTokenHolder());
  }
}

const std::string& ChannelWriteToken::getDataClassName() const
{
  return handle->dataclassname;
}

void ChannelWriteToken::releaseAccess(const void* data_ptr,
                                      const DataTimeSpec& ts)
{
  handle->entry->newData(data_ptr, ts);
}

void ChannelWriteToken::decodeAndWriteData(AmorphReStore& s,
                                           const DataTimeSpec& ts)
{
  void *data_ptr = handle->entry->getDataSpace();
  try {
    converter->unPackData(s, data_ptr);
    handle->entry->newData(data_ptr, ts);
  }
  catch (const AmorphStoreBoundary& e) {
    converter->delData(data_ptr);
    throw(e);
  }
}

void ChannelWriteToken::discardAccess(const void* data_ptr)
{
  converter->delData(data_ptr);
}

bool ChannelWriteToken::applyFunctor(DCOFunctor* fnct,
                                     const DataTimeSpec& time)
{
  void *data_ptr = handle->entry->getDataSpace();
  bool res = fnct->operator()(data_ptr);
  handle->entry->newData(data_ptr, time);
  return res;
}

void ChannelWriteToken::reWrite(const DataTimeSpec& time)
{
  void *data_ptr = handle->entry->getDataSpace();
  handle->entry->newData(data_ptr, time);
}


entryid_type ChannelWriteToken::getEntryId() const
{
  if (handle && handle->entry) {
    return handle->entry->getId();
  }
  return entry_end;
}

ChannelEntryInfo ChannelWriteToken::getChannelEntryInfo() const
{
  if (handle->entry) {
    return ChannelEntryInfo(handle->entry);
  }
  else {
    return ChannelEntryInfo();
  }
}


DUECA_NS_END;
