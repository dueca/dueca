/* ------------------------------------------------------------------   */
/*      item            : EntryHandler.cxx
        made by         : Rene' van Paassen
        date            : 170208
        category        : body file
        description     :
        changes         : 170208 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define EntryHandler_cxx
#include "EntryHandler.hxx"

STARTNSREPLICATOR;

EntryHandler::EntryHandler(const dueca::ChannelEntryInfo& entryinfo,
                           const std::string& channelname,
                           const dueca::GlobalId& master_id,
                           dueca::entryid_type replicator_entryid) :
  entryinfo(entryinfo),
  channelname(channelname),
  master_id(master_id),
  replicator_entryid(replicator_entryid),
  data_magic(0),
  lasttick(0)
{

}


EntryHandler::~EntryHandler()
{

}

const dueca::GlobalId& EntryHandler::getId() const
{
  return master_id;
}

ENDNSREPLICATOR;
