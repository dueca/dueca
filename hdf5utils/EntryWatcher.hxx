/* ------------------------------------------------------------------   */
/*      item            : EntryWatcher.hxx
        made by         : Rene van Paassen
        date            : 170201
        category        : header file
        description     :
        changes         : 170201 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef EntryWatcher_hxx
#define EntryWatcher_hxx

#include <string>
#include <H5Cpp.h>
#include <ChannelWatcher.hxx>
#include <dueca.h>
#include "HdfLogNamespace.hxx"
#include <memory>
#include <boost/scoped_ptr.hpp>

STARTHDF5LOG;

class EntryWatcher: public dueca::ChannelWatcher
{
  // access to the logger for callback etc.
  HDF5Logger        *master;

  // remember which channel we watch
  std::string        channelname;

  // path where we store
  std::string        path;

  // total path including any later defined prefixes
  std::string        tpath;

  // index of the next new entry
  unsigned           eidx;

  // log switched or all
  bool               always_logging;

  /** compress dataset */
  bool               compress;

  /// If non-null, used for logging rate reduction
  DataTimeSpec      *reduction;

  /// size of data chunks
  unsigned           chunksize;

  /** Combination of data needed for reading & storing a single entry */
  struct EntryData {

    /** Acccess to the channel */
    ChannelReadToken                r_token;

    /** channel entry */
    entryid_type                    entry_id;

    /** count entry id */
    unsigned                        eidx;

    /** logging functor object */
    boost::scoped_ptr<DCOFunctor>   functor;

    /** Reduction for logging */
    PeriodicTimeSpec               *reduction;

    /** Constructor, also creates read token */
    EntryData(const dueca::ChannelEntryInfo& i,
              const std::string& channelname,
              const std::string& path,
              unsigned eidx,
              const HDF5Logger *master,
              bool always_logging,
              bool compress,
              const DataTimeSpec *reduction,
              unsigned chunksize);

    /** Destructor, removes the token again */
    ~EntryData();

    /** Perform the logging */
    void accessAndLog(const dueca::TimeSpec& ts);

    /** create the functor, e.g. when logging new file or new location */
    void createFunctor(std::weak_ptr<H5::H5File> nfile,
                       const HDF5Logger *master,
                       unsigned chunksize, bool always_logging,
                       bool compress,
                       const std::string &prefix);
  };

  /** List type of above */
  typedef std::list<std::shared_ptr<EntryData> > entrylist_type;

  /** The resulting list */
  entrylist_type entrylist;

  /** Variable to hold change results */
  ChannelEntryInfo  einfo;

  /** Poll for changes to see if the channel has new entries, or
      entries have been deleted. */
  void checkChanges() ;

public:
  /** Constructor */
  EntryWatcher(const std::string& channelname, const std::string& path,
               HDF5Logger* w, bool always_logging, bool compress,
               const DataTimeSpec *reduction,
               unsigned chunksize);

  /** Destructor */
  virtual ~EntryWatcher();

  /** Perform the logging */
  void accessAndLog(const dueca::TimeSpec& ts);

  /** re-create Functors for a new file */
  void createFunctors(std::weak_ptr<H5::H5File> nfile,
                      const std::string &prefix);
};

ENDHDF5LOG;

#endif
