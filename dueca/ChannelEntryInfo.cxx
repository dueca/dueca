/* ------------------------------------------------------------------   */
/*      item            : ChannelEntryInfo.cxx
        made by         : Rene' van Paassen
        date            : 170403
        category        : body file
        description     :
        changes         : 170403 first version
        language        : C++
        copyright       : (c) 17 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ChannelEntryInfo_cxx
#include "ChannelEntryInfo.hxx"
#include "DataClassRegistry.hxx"
#include "DataSetConverter.hxx"
#include "UnifiedChannel.hxx"
#include "ChannelDef.hxx"
#include "UChannelEntry.hxx"

DUECA_NS_START;

ChannelEntryInfo::ChannelEntryInfo(const UChannelEntry* e, bool created) :
  entry_id(e->getId()),
  creation_id(e->getCreationId()),
  data_class(e->getDataClassName()),
  data_magic(DataClassRegistry::single().getConverter(data_class)->getMagic()),
  entry_label(e->getLabel()),
  time_aspect(e->isEventType() ? Channel::Events : Channel::Continuous),
  arity(e->isExclusive() ? Channel::OnlyOneEntry : Channel::ZeroOrMoreEntries),
  packingmode(e->isFullPack() ?
              Channel::OnlyFullPacking : Channel::MixedPacking),
  transportclass(e->getChannel()->getTransportClass()),
  origin(e->getOrigin()),
  created(created)
{
  //
}

ChannelEntryInfo::ChannelEntryInfo(entryid_type entry_id,
                                   uint32_t creation_id,
                                   const std::string& data_class,
                                   const std::string& entry_label,
                                   Channel::EntryTimeAspect time_aspect,
                                   Channel::EntryArity arity,
                                   Channel::PackingMode packingmode,
                                   Channel::TransportClass transportclass,
				   const GlobalId& origin) :
  entry_id(entry_id),
  creation_id(creation_id),
  data_class(data_class),
  data_magic(DataClassRegistry::single().getConverter(data_class)->getMagic()),
  entry_label(entry_label),
  time_aspect(time_aspect),
  arity(arity),
  packingmode(packingmode),
  transportclass(transportclass),
  origin(origin),
  created(true)
{
  //
}

ChannelEntryInfo::ChannelEntryInfo() :
  entry_id(0),
  creation_id(0),
  data_class(""),
  data_magic(0U),
  entry_label(""),
  time_aspect(Channel::AnyTimeAspect),
  arity(Channel::ZeroOrMoreEntries),
  packingmode(Channel::OnlyFullPacking),
  transportclass(Channel::Regular),
  origin(),
  created(false)
{
  //
}

ChannelEntryInfo& ChannelEntryInfo::operator = (const ChannelEntryInfo& o)
{
  if (this != &o) {
    entry_id = o.entry_id;
    creation_id = o.creation_id;
    data_class = o.data_class;
    data_magic = o.data_magic;
    entry_label = o.entry_label;
    time_aspect = o.time_aspect;
    arity = o.arity;
    packingmode = o.packingmode;
    transportclass = o.transportclass;
    origin = o.origin;
    created = o.created;
  }
  return *this;
}

ChannelEntryInfo::ChannelEntryInfo(const ChannelEntryInfo& o) :
  entry_id(o.entry_id),
  creation_id(o.creation_id),
  data_class(o.data_class),
  data_magic(o.data_magic),
  entry_label(o.entry_label),
  time_aspect(o.time_aspect),
  arity(o.arity),
  packingmode(o.packingmode),
  transportclass(o.transportclass),
  origin(o.origin),
  created(o.created)
{
  //
}

/** JSON print template */
void ChannelEntryInfo::JSONprint(std::ostream& p)
{
  p << "{\"entry_id\":" << entry_id
    << ",\"data_class\":\"" << data_class
    << "\",\"entry_label\":\"" << entry_label
    << "\",\"created\":" << (created ? "true" : "false")
    << "}";
}


DUECA_NS_END;

PRINT_NS_START;
std::ostream& operator << (std::ostream& os, const dueca::ChannelEntryInfo& i)
{
  os << "ChannelEntryInfo(entry_id=" << i.entry_id
     << ", creation_id=" << i.creation_id
     << ", data_class=" << i.data_class
     << ", data_magic=" << i.data_magic
     << ", entry_label=" << i.entry_label
     << ", time_aspect=" << i.time_aspect
     << ", arity=" << i.arity
     << ", packingmode=" << i.packingmode
     << ", transportclass=" << i.transportclass
     << ", origin=" << i.origin
     << ", created=" << i.created << ")";
  return os;
}
PRINT_NS_END;
