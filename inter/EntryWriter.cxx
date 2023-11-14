/* ------------------------------------------------------------------   */
/*      item            : EntryWriter.cxx
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

#define EntryWriter_cxx
#include "EntryWriter.hxx"

#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>
#include <AmorphStore.hxx>
#include "ReplicatorExceptions.hxx"
#include <PeerTiming.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>
#define W_INT
#include <debug.h>

STARTNSREPLICATOR;


EntryWriter::EntryWriter(const GlobalId& master_id,
                         unsigned originator_id,
                         uint16_t rid,
                         const std::string& channelname,
                         const std::string& dataclass,
                         uint32_t data_magic,
                         const std::string& entrylabel,
                         Channel::EntryTimeAspect time_aspect,
                         Channel::EntryArity arity,
                         Channel::PackingMode packmode,
                         Channel::TransportClass tclass,
			 const GlobalId& origin) :
  EntryHandler(dueca::ChannelEntryInfo
               (dueca::entry_end, 0, dataclass, entrylabel, time_aspect,
                arity, packmode, tclass, origin),
               channelname,
               master_id, rid),
  valid(false),
  originator_id(originator_id),
  cbvalid(this, &EntryWriter::tokenIsValid),
  w_entry(master_id, dueca::NameSet(channelname), dataclass, entrylabel,
          time_aspect, arity, packmode, tclass, &cbvalid)
{
  if (data_magic != w_entry.getDataClassMagic()) {
    /* DUECA interconnect.

       Received a different magic number for a data class. This
       usually means that the data class differs between DUECA
       processes; align your data */
    E_INT("ChannelReplicator incorrect magic writing data class "
          << dataclass);
    throw(dataclassdiffers());
  }
  this->data_magic = data_magic;
  /* DUECA interconnect.

     Information on a new entrywriter. */
  I_INT("EntryWriter " << channelname);
  DEB("new EntryWriter " << channelname
      << " for originator " << originator_id);
}


EntryWriter::~EntryWriter()
{
  DEB("removing EntryWriter " << channelname
      << " for originator " << originator_id);
}

void EntryWriter::tokenIsValid(const TimeSpec& ts)
{
  entryinfo.entry_id = w_entry.getEntryId();
  /* DUECA interconnect.

     Information on token validity for an entrywriter. */
  I_INT("EntryWriter token valid " << channelname <<
        " entry# " << entryinfo.entry_id << " orig " << originator_id <<
        " rid " << getReplicatorEntryId());
  valid = true;
}

void EntryWriter::writeChannel(AmorphReStore& s,
                               const PeerTiming& timeshift,
                               bool spanskip)
{
  // remember the store level, for when unpacking is not possible
  unsigned level = s.getIndex();

  // unpack the size mark
  uint16_t mark __attribute__((unused)) (s);

  // unpack the timing & then the data
  DataTimeSpec ts;

  if (entryinfo.time_aspect == Channel::Continuous) {
    if (spanskip) {
      ::unPackData(s, ts);
    }
    else {
      TimeTickType tend(s);
      ts = DataTimeSpec(lasttick, tend);
    }
    lasttick = ts.getValidityEnd();
  }
  else {
    TimeTickType tevent(s);
    ts = DataTimeSpec(tevent, tevent);
  }
  if (timeshift.translate(ts)) {
    w_entry.decodeAndWriteData(s, ts);
  }
  else {
    /* DUECA interconnect.

       Timing translation is not yet possible for this message,
       discarding data. */
    I_NET("Cannot (yet) translate timing " << channelname
          << " rid=" << replicator_entryid);
    s.setIndex(level);
    s.gobble();
  }
}

ENDNSREPLICATOR;
