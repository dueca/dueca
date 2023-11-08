/* ------------------------------------------------------------------   */
/*      item            : UCClientHandle.cxx
        made by         : Rene' van Paassen
        date            : 140110
        category        : body file
        description     :
        changes         : 140110 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define UCClientHandle_cxx
#include "UCClientHandle.hxx"
#include "UChannelEntry.hxx"
#include "ChannelWriteToken.hxx"
#include "ChannelReadToken.hxx"
#include "GlobalId.hxx"
#include "Ticker.hxx"
#include "Trigger.hxx"

DUECA_NS_START;

UCClientHandle::UCClientHandle(ChannelReadToken* token,
                               const std::string& dataclassname,
                               const std::string& entrylabel,
                               GenericCallback* callback,
                               entryid_type requested_entry,
                               Channel::ReadingMode readmode,
                               double requested_span,
                               unsigned requested_depth,
                               unsigned creation_id) :
  token(token),
  dataclasslink(NULL),
  config_change(NULL),
  access_count(1),
  dataclassname(dataclassname),
  class_lead(NULL),
  entry(NULL),
  callback(callback),
  requested_entry(requested_entry),
  entrylabel(entrylabel),
  accessed(NULL),
  // calculation of requested span avoids ticker access, to enable
  // creation of the initial ChannelManager channels
  requested_span(requested_span == 0.0 ? 0 :
                 int(round(max(requested_span, 0.0) /
                           Ticker::single()->getTimeGranule()))),
  requested_depth(requested_depth),
  reading_mode(readmode),
  client_creation_id(creation_id),
  trigger_target(NULL)
{ }

UCClientHandle::~UCClientHandle()
{
  //
}

void UCClientHandle::addPuller(TriggerPuller* target)
{
  if (trigger_target != NULL) {
    auto el = class_lead;
    while (el) {
      el->entry->requestRemoveTrigger(this);
      el = el->next;
    }
  }

  trigger_target = target;
  auto el = class_lead;
  while (el) {
    el->entry->requestIncludeTrigger(this);
    el = el->next;
  }
}

const GlobalId& UCClientHandle::getId() const
{ return token->getTokenHolder(); }

UCWriterHandle::UCWriterHandle(ChannelWriteToken* token,
                               UChannelEntryPtr entry,
                               const std::string& dataclassname,
                               GenericCallback* valid) :
  token(token),
  writer_id(token == NULL? GlobalId(): token->getTokenHolder()),
  dataclassname(dataclassname),
  entry(entry),
  callback(valid)
{
  entry->setWriterHandle(this);
}


UCEntryClientLink::UCEntryClientLink(UChannelEntryPtr entry,
                                     uint32_t client_id,
                                     bool sequential_read,
                                     UCEntryClientLinkPtr next) :
  next(next),
  entry(entry),
  entry_creation_id(entry->getCreationId()),
  client_id(client_id),
  sequential_read(sequential_read),
  read_index(sequential_read ? entry->latchSequentialRead() : NULL),
  seq_id(0)
{
  //
}

bool UCEntryClientLink::isMatch(const UCEntryClientLinkPtr other) const
{
  return entry_creation_id == other->entry_creation_id;
}

bool UCEntryClientLink::isMatch(uint32_t other) const
{
  return entry_creation_id == other;
}

bool UCEntryClientLink::entryMatch() const
{
  return entry_creation_id == entry->getCreationId();
}


DUECA_NS_END;
