/* ------------------------------------------------------------------   */
/*      item            : ChannelEntryInfo.hxx
        made by         : Rene van Paassen
        date            : 170403
        category        : header file
        description     :
        changes         : 170403 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelEntryInfo_hxx
#define ChannelEntryInfo_hxx

#include <ChannelDef.hxx>
#include <GlobalId.hxx>
#include <iostream>

DUECA_NS_START;

class UChannelEntry;

/** Encapsulating element for available channel entry data. Objects of this
    type are returned by the ChannelWatcher. */
class ChannelEntryInfo
{
public:
  /** Entry index */
  entryid_type entry_id;

  /** Unique identifying id, matches the creation */
  uint32_t     creation_id;

  /** Type of data */
  std::string  data_class;

  /** Magic number for the data class */
  uint32_t     data_magic;

  /** Label given to the data */
  std::string  entry_label;

  /** Is it stream or event data. */
  Channel::EntryTimeAspect time_aspect;

  /** How many entries accepted? */
  Channel::EntryArity arity;

  /** What packing type selected? */
  Channel::PackingMode packingmode;

  /** Transport priority */
  Channel::TransportClass transportclass;

  /** Origin/writer of data */
  GlobalId origin;

  /** Entry created (true) or removed (false)? */
  bool created;

public:
  /** Constructor from entry pointer */
  ChannelEntryInfo(const UChannelEntry* e, bool created=true);

  /** Constructor with individual arguments */
  ChannelEntryInfo(entryid_type entry_id, uint32_t creation_id,
                   const std::string& data_class,
                   const std::string& entry_label,
                   Channel::EntryTimeAspect time_aspect,
                   Channel::EntryArity arity,
                   Channel::PackingMode packingmode,
                   Channel::TransportClass transportclass,
		   const GlobalId& origin);

  /** Empty constructor */
  ChannelEntryInfo();

  /** Copy constructor */
  ChannelEntryInfo(const ChannelEntryInfo&);

  /** Assignment operator */
  ChannelEntryInfo& operator = (const ChannelEntryInfo& o);

  /** JSON print */
  void JSONprint(std::ostream& p);
};



DUECA_NS_END;

PRINT_NS_START;
/** Print a channel entry information object */
std::ostream& operator << (std::ostream& is, const dueca::ChannelEntryInfo& i);
PRINT_NS_END;


#endif
