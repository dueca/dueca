/* ------------------------------------------------------------------   */
/*      item            : ConfigStorage.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Wed Jun 17 21:21:15 2020
        category        : body file
        description     :
        changes         : Wed Jun 17 21:21:15 2020 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ConfigStorage_cxx

// include the definition of the module class
#include "ConfigStorage.hxx"

// include the debug writing header, by default, write warning and
// error messages
#include <debug.h>

// include additional files needed for your calculation here
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;

// class/module name
const char* const ConfigStorage::classname = "config-storage";

// Parameters to be inserted
const ParameterTable* ConfigStorage::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "file-suffix",
      new VarProbe<_ThisModule_,std::string>
      (&_ThisModule_::file_suffix),
      "Suffix for selecting filenames." },

    { "path-configfiles",
      new VarProbe<_ThisModule_,std::string>
      (&_ThisModule_::path_configfiles),
      "Location of the configuration files." },

    { "receiving-channel",
      new VarProbe<_ThisModule_,std::string>
      (&_ThisModule_::r_channelname),
      "Name of the receiving channel" },

    { "sending-channel",
      new VarProbe<_ThisModule_,std::string>
      (&_ThisModule_::r_channelname),
      "Name of the sending channel" },

    { "allow-overwrite",
      new VarProbe<_ThisModule_,bool>
      (&_ThisModule_::allow_overwrite),
      "Allow overwriting of existing files" },

    { "filename-template",
      new VarProbe<_ThisModule_,std::string>
      (&_ThisModule_::lftemplate),
      "Template for last part of the file name, optional; check boost\n"
      "time_facet for format strings. Is combined with the file name as\n"
      "given in the write request and the suffix. Optional.\n"
      "Default suggestion: -%Y%m%d_%H%M%S" },

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "Storage module for configuration files."} };

  return parameter_table;
}

// constructor
ConfigStorage::ConfigStorage(Entity* e, const char* part, const
                             PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  file_suffix(".plot"),
  path_configfiles("."),
  w_channelname(),
  r_channelname(),
  allow_overwrite(false),
  lftemplate("-%Y%m%d_%H%M%S"),
  watcher(),
  clients()
{
  //
}

ConfigStorage::MyWatcher::MyWatcher(ConfigStorage *master,
                                    const std::string& channelname) :
  ChannelWatcher(NameSet(channelname)),
  master(master)
{
  //
}

void ConfigStorage::MyWatcher::entryAdded(const ChannelEntryInfo& i)
{ master->entryAdded(i); }
void ConfigStorage::MyWatcher::entryRemoved(const ChannelEntryInfo& i)
{ master->entryRemoved(i); }

bool ConfigStorage::complete()
{
  if (!r_channelname.size()) {
    r_channelname = NameSet
      (getEntity(), ConfigFileRequest::classname, "").name;
  }
  if (!w_channelname.size()) {
    w_channelname = NameSet
      (getEntity(), ConfigFileData::classname, "").name;
  }

  // watch the channel
  watcher.reset(new MyWatcher(this, r_channelname));

  // no checks at this point, so everything OK
  return true;
}

// destructor
ConfigStorage::~ConfigStorage()
{
  //
}

// tell DUECA you are prepared
bool ConfigStorage::isPrepared()
{
  // always OK
  return true;
}

// start the module
void ConfigStorage::startModule(const TimeSpec &time)
{
  // always on
}

// stop the module
void ConfigStorage::stopModule(const TimeSpec &time)
{
  // always on
}

namespace bfs = boost::filesystem;

bool endsWith(const std::string& totest, const std::string& ending)
{
  if (totest.length() < ending.length()) return false;
  if (ending.length() == 0) return true;
  return (0 == totest.compare(totest.length() - ending.length(),
                              ending.length(), ending));
}

void ConfigStorage::entryAdded(const ChannelEntryInfo& i)
{
  DEB("New entry detected " << i);
  clients.push_back
    (std::shared_ptr<ConfigClient>(new ConfigClient(this, i)));
}

void ConfigStorage::entryRemoved(const ChannelEntryInfo& i)
{
  DEB("Entry removed " << i);
  for (auto c = clients.begin(); c != clients.end(); c++) {
    if ((*c)->r_request.isValid() &&
        (*c)->r_request.getEntryId() == i.entry_id) {
      clients.erase(c);
      return;
    }
  }

  /* DUECA websockets.

     A client entry is indicated as removed from the configuration
     storage channel, but the corresponding client cannot be found
     locally. Indicates a DUECA programming error.
  */
  W_XTR("Could not remove client entry with id=" << i.entry_id);
}

ConfigStorage::ConfigClient::ConfigClient(const ConfigStorage* master,
                                          const ChannelEntryInfo& i) :
  master(master),
  r_request(master->getId(), NameSet(master->r_channelname),
            ConfigFileRequest::classname, i.entry_id,
            Channel::Events, Channel::OneOrMoreEntries),
  w_answer(master->getId(), NameSet(master->w_channelname),
           ConfigFileData::classname, i.entry_label,
           Channel::Events, Channel::OneOrMoreEntries,
           Channel::OnlyFullPacking, Channel::Bulk),
  cb(this, &ConfigStorage::ConfigClient::respondRequest),
  do_respond(master->getId(),
             (std::string("config response ") + i.entry_label).c_str(),
             &cb, PrioritySpec(0,0))
{
  do_respond.setTrigger(r_request);
  do_respond.switchOn();
  DEB("ConfigClient created");
}

static std::string FormatTime(const std::string& lft)
{
  using namespace boost::posix_time;
  static std::locale loc(std::cout.getloc(), new time_facet(lft.c_str()));
  std::basic_stringstream<char> wss;
  wss.imbue(loc);
  wss << second_clock::universal_time();
  return wss.str();
}

void ConfigStorage::ConfigClient::respondRequest(const TimeSpec& ts)
{
  DEB("Entered respondRequest, tokens " <<
      r_request.isValid() << w_answer.isValid());

  if (!r_request.isValid() || !w_answer.isValid()) return;

  DataReader<ConfigFileRequest> r(r_request, ts);
  DEB("Request" << r.data());
  DataWriter<ConfigFileData> w(w_answer, ts);

  if (r.data().config.size()) {
    std::ofstream ofile;
    bfs::path p = master->path_configfiles;
    std::string ofname = r.data().name;
    if (master->lftemplate.size()) {
      ofname = r.data().name + FormatTime(master->lftemplate) +
        master->file_suffix;
    }
    else {
      if (!endsWith(ofname, master->file_suffix)) {
        ofname += master->file_suffix;
      }
    }
    if (!master->allow_overwrite && bfs::is_regular_file(p / ofname)) {
      /* DUECA websockets.

         The configuration storage is configured to not overwrite
         files, and a request came in with new data for a file. The
         request is skipped. Adjust the configuration or your external
         client program to prevent this from happening. */
      W_XTR("Not overwriting file \"" << ofname << "\"");
    }
    else {
      ofile.open((p / ofname).string(), std::ios_base::out);
      ofile << r.data().config;
      ofile.close();
    }
    DEB("Written file " << ofname);
  }

  // compose reply
  if (r.data().name.size() == 0 || r.data().config.size() != 0) {
    // empty request means a reply with list, also reply with new list
    // after a write action
    for (auto& itr: boost::make_iterator_range
           (bfs::directory_iterator(master->path_configfiles))) {
      if (bfs::is_regular_file(itr.status()) &&
          endsWith(itr.path().filename().string(), master->file_suffix)) {
        std::time_t time = last_write_time(itr.path());
        struct tm* ttime = localtime(&time);
        char timebuf[20];
        strftime(timebuf, 20, "%Y-%m-%d %H:%M:%S", ttime);
        w.data().filenames.push_back
          (NameSizeDate(itr.path().filename().string(),
                        bfs::file_size(itr.path()),
                        timebuf));
      }
    }
  }
  else {
    // name indicated, config size 0 means a request for configuration
    // content
    std::ifstream ifile;
    ifile.open(master->path_configfiles + std::string("/") + r.data().name);
    if (ifile.good()) {
      std::stringstream ifstr; ifstr << ifile.rdbuf();
      w.data().config = ifstr.str();
      w.data().name = r.data().name;
    }
    else {
      /* DUECA websockets.

         Failed to read a requested file. Check file permissions or
         your external client program for asking for a non-existent
         file. */
      W_XTR("Cannot read configuration file \"" << r.data().name << "\"");
    }
    ifile.close();
  }
  DEB("sending " << w.data());
}

const GlobalId& ConfigStorage::ConfigClient::getId() const
{
  return master->getId();
}

DUECA_NS_END;

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
// static TypeCreator<ConfigStorage> a(ConfigStorage::getMyParameterTable());

