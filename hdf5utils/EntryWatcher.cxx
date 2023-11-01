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

#define EntryWatcher_cxx
#include "EntryWatcher.hxx"
#include "HDF5Logger.hxx"
#include <sstream>
#include <iomanip>
//#define I_XTR
#include <debug.h>

STARTHDF5LOG;

EntryWatcher::EntryWatcher(const std::string& channelname,
                           const std::string& path,
                           HDF5Logger* w, bool always_logging,
                           bool compress,
                           const DataTimeSpec *reduction,
                           unsigned chunksize) :
  ChannelWatcher(channelname, true),
  master(w),
  channelname(channelname),
  path(path),
  tpath(path),
  eidx(0),
  always_logging(always_logging),
  compress(compress),
  reduction(reduction ? new DataTimeSpec(*reduction) : NULL),
  chunksize(chunksize)
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

      /* DUECA hdf5.

         Information on a new entry in a monitored channel. */
      I_XTR("HDF5 log, channel " << channelname << " new entry " <<
            einfo.entry_id << " dataclass " << einfo.data_class <<
            " label '" << einfo.entry_label << "'");
      entrylist.push_back(entrylist_type::value_type
                          (new EntryData(einfo, channelname, tpath,
                                         eidx++, master, always_logging,
                                         compress, reduction, chunksize)));
    }
    else {

      /* DUECA hdf5.

         Information on a removed entry in a monitored channel. */
      I_XTR("HDF5 log, channel " << channelname << " remove entry " <<
            einfo.entry_id);
      entrylist_type::iterator ee = entrylist.begin();
      while (ee != entrylist.end() && (*ee)->entry_id != einfo.entry_id) ee++;
      if (ee == entrylist.end()) {

        /* DUECA hdf5.

           A removed entry in a monitored channel could not be found
           in the Loggers/Watchers data structure. Indicates a
           programming error in DUECA.
        */
        W_XTR("HDF5 log monitored entry " << einfo.entry_id << " channel " <<
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

void EntryWatcher::createFunctors(std::weak_ptr<H5::H5File> nfile,
                                  const std::string &prefix)
{
  checkChanges();
  tpath = prefix + path;

  for (entrylist_type::iterator ee = entrylist.begin();
       ee != entrylist.end(); ee++) {
    (*ee)->createFunctor(nfile, master, chunksize, always_logging,
                         compress, tpath);
  }
}

EntryWatcher::EntryData::EntryData(const dueca::ChannelEntryInfo& i,
                                   const std::string& channelname,
                                   const std::string& path,
                                   unsigned eidx,
                                   const HDF5Logger *master,
                                   bool always_logging,
                                   bool compress,
                                   const DataTimeSpec *reduction,
                                   unsigned chunksize) :
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

    createFunctor(master->getFile(), master, chunksize, always_logging,
                  compress, path);
  }
}

EntryWatcher::EntryData::~EntryData()
{
  // deletes?
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
        r_token.applyFunctor(functor.get());
      }
      else {
        r_token.flushOne();
      }
      tsc0 = r_token.getOldestDataTime();
    }
  }
  else {
    if (functor) {
      while (r_token.applyFunctor(functor.get())) {
        // nothing to be done, could put message here
      }
    }
    else {
      r_token.flushOlderSets(ts.getValidityStart());
    }
  }
}

void EntryWatcher::EntryData::createFunctor(std::weak_ptr<H5::H5File> nfile,
                                            const HDF5Logger *master,
                                            unsigned chunksize,
                                            bool always_logging,
                                            bool compress,
                                            const std::string &path)
{
  // find the meta information
  ChannelEntryInfo ei = r_token.getChannelEntryInfo();

  // metafunctor can create the logging functor
  std::weak_ptr<HDF5DCOMetaFunctor> metafunctor
    (r_token.getMetaFunctor<HDF5DCOMetaFunctor>("hdf5"));

  std::stringstream dpath;
  dpath << path << "/e" << std::setw(6) << std::setfill('0') << eidx;

  functor.reset(metafunctor.lock()->getWriteFunctor
                (nfile.lock(), dpath.str(), chunksize,
                 ei.entry_label, master->getOpTime(always_logging),
                 compress));
}


ENDHDF5LOG;
