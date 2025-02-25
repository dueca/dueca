/* ------------------------------------------------------------------   */
/*      item            : EntryWatcher.cxx
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

#include "ddff_ns.h"
#include <rapidjson/stringbuffer.h>
#include <dueca/DCOtypeJSON.hxx>
#define EntryWatcher_cxx
#include "EntryWatcher.hxx"
#include "DDFFLogger.hxx"
#include <sstream>
#include <iomanip>
//#define I_XTR
#include <debug.h>

DDFF_NS_START;

EntryWatcher::EntryWatcher(const std::string& channelname,
                           const std::string& path,
                           DDFFLogger* w, bool always_logging,
                           const DataTimeSpec *reduction) :
  ChannelWatcher(channelname, true),
  master(w),
  channelname(channelname),
  path(path),
  tpath(path),
  eidx(0),
  always_logging(always_logging),
  reduction(reduction ? new DataTimeSpec(*reduction) : NULL)
{
  //
}


EntryWatcher::~EntryWatcher()
{
  disableWatcher();
}

void EntryWatcher::checkChanges()
{
  if (checkChange(einfo)) {
    if (einfo.created) {

      /* DUECA ddff.

         Information on a new entry in a monitored channel. */
      I_XTR("DDFF log, channel " << channelname << " new entry " <<
            einfo.entry_id << " dataclass " << einfo.data_class <<
            " label '" << einfo.entry_label << "'");
      entrylist.push_back(entrylist_type::value_type
                          (new EntryData(einfo, channelname, tpath,
                                         eidx++, master, always_logging,
                                         reduction)));
    }
    else {

      /* DUECA ddff.

         Information on a removed entry in a monitored channel. */
      I_XTR("DDFF log, channel " << channelname << " remove entry " <<
            einfo.entry_id);
      entrylist_type::iterator ee = entrylist.begin();
      while (ee != entrylist.end() && (*ee)->entry_id != einfo.entry_id) ee++;
      if (ee == entrylist.end()) {

        /* DUECA ddff.

           A removed entry in a monitored channel could not be found
           in the Loggers/Watchers data structure. Indicates a
           programming error in DUECA.
        */
        W_XTR("DDFF log monitored entry " << einfo.entry_id << " channel " <<
              channelname << " could not be removed");
        return;
      }
      entrylist.erase(ee);
    }
  }
}


void EntryWatcher::accessAndLog(const dueca::TimeSpec& ts)
{
  checkChanges();

  for (entrylist_type::iterator ee = entrylist.begin();
       ee != entrylist.end(); ee++) {
    (*ee)->accessAndLog(ts);
  }
}

void EntryWatcher::createFunctors(std::weak_ptr<FileWithSegments> nfile,
                                  const std::string &prefix)
{
  checkChanges();
  tpath = prefix + path;

  for (entrylist_type::iterator ee = entrylist.begin();
       ee != entrylist.end(); ee++) {
    (*ee)->createFunctor(nfile, master, always_logging,
                         tpath);
  }
}

EntryWatcher::EntryData::EntryData(const dueca::ChannelEntryInfo& i,
                                   const std::string& channelname,
                                   const std::string& path,
                                   unsigned eidx,
                                   const DDFFLogger *master,
                                   bool always_logging,
                                   const DataTimeSpec *reduction) :
  r_token(master->getId(), NameSet(channelname), i.data_class, i.entry_id,
          Channel::AnyTimeAspect, Channel::OneOrMoreEntries,
          Channel::ReadAllData),
  entry_id(i.entry_id),
  eidx(eidx),
  functor(),
  reduction(reduction ? new PeriodicTimeSpec(*reduction) : NULL)
{
  // if the master file is already open (standard loggin), create the functor
  if (master->getFile().lock()) {

    createFunctor(master->getFile(), master, always_logging,
                  path);
  }
}

EntryWatcher::EntryData::~EntryData()
{

}

void EntryWatcher::EntryData::accessAndLog(const dueca::TimeSpec& ts)
{
  if (!r_token.isValid()) return;
  if (reduction) {
    DataTimeSpec tsc0 = r_token.getOldestDataTime();
    DataTimeSpec tsc1 = r_token.getLatestDataTime();

    while (tsc0.getValidityStart() < tsc1.getValidityStart()) {
      if (tsc0.getValidityStart() > reduction->getValidityEnd() +
          reduction->getValiditySpan()) {
        reduction->forceAdvance(tsc0.getValidityStart() - 1);
      }
      if (reduction->greedyAdvance(tsc0) && functor) {
        w_stream->markItemStart();
        r_token.applyFunctor(functor.get());
        dirty = true;
      }
      else {
        r_token.flushOne();
      }
      tsc0 = r_token.getOldestDataTime();
    }
  }
  else {
    if (functor) {
      do {
        w_stream->markItemStart();
      }
      while (r_token.applyFunctor(functor.get()));
      dirty = true;
    }
    else {
      r_token.flushOlderSets(ts.getValidityStart());
    }
  }

  // confirm we got here
  marked_tick = ts.getValidityEnd();
}

void EntryWatcher::EntryData::createFunctor(std::weak_ptr<FileWithSegments> nfile,
                                            const DDFFLogger *master,
                                            bool always_logging,
                                            const std::string &path)
{
  // find the meta information
  ChannelEntryInfo ei = r_token.getChannelEntryInfo();

  // this metafunctor can create the logging functor
  std::weak_ptr<DDFFDCOMetaFunctor> metafunctor
    (r_token.getMetaFunctor<DDFFDCOMetaFunctor>("msgpack"));

  // dpath is the identifying path, add an e00000 counter for each entry
  std::stringstream dpath;
  dpath << path << "/e" << std::setw(6) << std::setfill('0') << eidx;

  // get a description of the data for the stream label
  rapidjson::StringBuffer doc;
  DCOtypeJSON(doc, ei.data_class.c_str());

  // request a stream in the file
  w_stream = nfile.lock()->createNamedWrite
    (dpath.str(), doc.GetString());

  // check in with the recorder,
  nfile.lock()->recorderCheckIn(dpath.str(), this);

  // use the stream for the functor
  functor.reset(metafunctor.lock()->getReadFunctor
                (w_stream, master->getOpTime(always_logging)));
}


DDFF_NS_END;
