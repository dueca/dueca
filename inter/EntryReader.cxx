/* ------------------------------------------------------------------   */
/*      item            : EntryReader.cxx
        made by         : Rene' van Paassen
        date            : 170201
        category        : body file
        description     :
        changes         : 170201 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define EntryReader_cxx
#include "EntryReader.hxx"
#include "ChannelReplicator.hxx"
#include <boost/lexical_cast.hpp>
// #define I_INT
#include <debug.h>
#include <DataTimeSpec.hxx>
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>
#include <dassert.h>

#define DEBPRINTLEVEL 1
#include <debprint.h>

STARTNSREPLICATOR;

EntryReader::EntryReader(const dueca::GlobalId &master_id,
                         const dueca::ChannelEntryInfo &i,
                         const std::string &channelname) :
  EntryHandler(i, channelname, master_id, 0),
  tokenvalid(false),
  cbv(this, &EntryReader::tokenIsValid),
  r_entry(master_id, NameSet(channelname), i.data_class, i.entry_id,
          i.time_aspect, Channel::OnlyOneEntry, Channel::ReadAllData, 0.0,
          &cbv),
  firstread(true)
{
  data_magic = r_entry.getDataClassMagic();
  /* DUECA interconnect.

     Information on the creation of a new entry reader. */
  I_INT("EntryReader " << channelname << " entry# " << i.entry_id << " type "
                       << i.data_class << " " << i.time_aspect);
  DEB("EntryReader " << channelname << " entry# " << i.entry_id);
}

EntryReader::~EntryReader() { DEB("~EntryReader " << channelname); }

void EntryReader::tokenIsValid(const TimeSpec &ts)
{
  tokenvalid = true;
  DEB1("EntryReader token valid " << channelname << " entry# "
                                  << r_entry.getEntryId() << " rid "
                                  << getReplicatorEntryId());
}

bool EntryReader::readChannel(AmorphStore &s, uint16_t channelid)
{
  if (tokenvalid) {
    if (firstread) {
      r_entry.flushOlderSets();
      firstread = false;
    }

    // remember current store pointer
    uint32_t idx0 = s.getSize();

    try {

      // room for channel id and store the entry id
      StoreMark<uint16_t> idmark = s.createMark(channelid);
      ::packData(s, replicator_entryid);

      // use the mark to remember the data size
      s.startMark();

      // pack the data itself
      switch (r_entry.readAndStoreData(s, lasttick)) {
      case ChannelReadToken::DataSuccess:
        DEB1("Channel " << channelid << " packed from " << r_entry.getName()
                        << " entry " << r_entry.getEntryId() << " at "
                        << lasttick << " s" << idx0 << ".." << s.getSize());
        s.finishMark(idmark, channelid);
        s.endMark();
        return true;

      case ChannelReadToken::NoData:
        DEB1("No data from " << r_entry.getName() << " entry "
                             << r_entry.getEntryId() << " at " << lasttick);
        s.setSize(idx0);
        return false;

      case ChannelReadToken::TimeSkip:
        /* DUECA interconnect.

           Information on sending a time skip in received data. */
        I_INT("Channel " << channelid << " with skip " << r_entry.getName()
                         << " entry " << r_entry.getEntryId() << " at "
                         << lasttick << " s" << idx0 << ".." << s.getSize());
        s.finishMark(idmark, uint16_t(0x8000 | channelid));
        s.endMark();
        return true;
      }
    }
    catch (const dueca::AmorphStoreBoundary &e) {
      /* DUECA interconnect.

         When packing channel data for sending to peer(s), a packing
         store full exception (dueca::AmorphStoreBoundary) was
         encountered. The data from the last channel to be packed will
         be held until the next communication opportunity. If this
         occurs frequently, try increasing the message size in the
               ChannelReplicatorMaster. */
      W_INT("No room for channel data from " << r_entry.getName());
      DEB1("Store full, ch "
           << channelid << " packed from " << r_entry.getName() << " entry "
           << r_entry.getEntryId() << " at " << lasttick << " s" << idx0 << ".."
           << s.getSize());
      s.setSize(idx0);
      throw(e);
    }
  }
  return false;
}

void EntryReader::flushEntry()
{
  if (tokenvalid) {
    r_entry.flushOlderSets();
  }
}

ENDNSREPLICATOR;
