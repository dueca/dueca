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
#include <ChannelWatcher.hxx>
#include <dueca.h>
#include <ddff/FileWithSegments.hxx>
#include <ddff/SegmentedRecorderBase.hxx>
#include "ddff_ns.h"
#include <memory>
#include <boost/scoped_ptr.hpp>

DDFF_NS_START;

class DDFFLogger;

class EntryWatcher: public dueca::ChannelWatcher
{
  // access to the logger for callback etc.
  DDFFLogger        *master;

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

  /// If non-null, used for logging rate reduction
  DataTimeSpec      *reduction;

  /** Combination of data needed for reading & storing a single entry */
  struct EntryData: SegmentedRecorderBase {

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
              const DDFFLogger *master,
              bool always_logging,
              const DataTimeSpec *reduction);

    /** Destructor */
    ~EntryData();

    /** Perform the logging */
    void accessAndLog(const dueca::TimeSpec& ts);

    /** create the functor, e.g. when logging new file or new location */
    void createFunctor(std::weak_ptr<ddff::FileWithSegments> nfile,
                       const DDFFLogger *master,
                       bool always_logging,
                       const std::string &prefix);
  };

  /** List type of above */
  typedef std::list<boost::intrusive_ptr<EntryData> > entrylist_type;

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
               DDFFLogger* w, bool always_logging,
               const DataTimeSpec *reduction);

  /** Destructor */
  virtual ~EntryWatcher();

  /** Perform the logging */
  void accessAndLog(const dueca::TimeSpec& ts);

  /** re-create Functors for a new file */
  void createFunctors(std::weak_ptr<FileWithSegments> nfile,
                      const std::string &prefix);
};

DDFF_NS_END;

#endif
