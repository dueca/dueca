/* ------------------------------------------------------------------   */
/*      item            : SnapshotInventory.cxx
        made by         : Rene' van Paassen
        date            : 220420
        category        : body file
        description     :
        changes         : 220420 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define SnapshotInventory_cxx
#include "SnapshotInventory.hxx"
#include <dueca/debug.h>
#include <dueca/DataReader.hxx>
#include <dueca/DataWriter.hxx>
#include <dueca/ObjectManager.hxx>
#include <dueca/Entity.hxx>
#include <dueca/EntityCommand.hxx>
#include "DUSIMEExceptions.hxx"
#include <toml.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#define CATCH_TOML_ERROR 0
#define DEBPRINTLEVEL -1
#include <debprint.h>

#define DO_INSTANTIATE
#include <dueca/Callback.hxx>
#include <dueca/CallbackWithId.hxx>
#define NO_TYPE_CREATION
#include <dueca/dueca.h>

DUECA_NS_START;

std::string decode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
        return c == '\0';
    });
}

std::string encode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

// static place for the map
std::map<std::string,SnapshotInventory::pointer> SnapshotInventory::inventories;

const char* const SnapshotInventory::classname = "initial-inventory";

template<> const char* getclassname<SnapshotInventory>()
{ return "initial-inventory"; }


SnapshotInventory::SnapshotInventory(const char* entity_name) :
  NamedObject(NameSet("dueca", "SnapshotInventory", entity_name)),
  state(StartFiles),
  newmode_clients(),
  all_valid(false),
  entity(entity_name),
  latest_incoming(0U),
  snapmap(),
  tomlsnp(),
  basefile(),
  resultfile(),
  snapname("anonymous"),
  cb(this, &SnapshotInventory::receiveSnapshot),
  cbvalid(this, &SnapshotInventory::checkValid),
  cbdusime(this, &SnapshotInventory::followDusime),
  r_snapshots(getId(),
              NameSet(entity, getclassname<Snapshot>(), "get"),
              getclassname<Snapshot>(), entry_any,
              Channel::Events, Channel::ZeroOrMoreEntries,
              Channel::ReadAllData, 0.0, &cbvalid),
  w_snapshots(getId(),
              NameSet(entity, getclassname<Snapshot>(), "set"),
              getclassname<Snapshot>(), entity,
              Channel::Events, Channel::OneOrMoreEntries,
              Channel::MixedPacking, Channel::Bulk, &cbvalid),
  r_dusime(getId(), NameSet("EntityCommand://dusime"),
           getclassname<EntityCommand>(), 0, Channel::Events,
           Channel::OnlyOneEntry, Channel::ReadAllData, 0.0, &cbvalid),
  store_snapshots(getId(), "collect snapshot", &cb, PrioritySpec(0,0)),
  follow_dusime(getId(), "track dusime", &cbdusime, PrioritySpec(0,0))
{
  store_snapshots.setTrigger(r_snapshots);
  store_snapshots.switchOn();
  follow_dusime.setTrigger(r_dusime);
  follow_dusime.switchOn();
}

SnapshotInventory::~SnapshotInventory()
{
  saveFile();
  inventories.erase(entity);
}

void SnapshotInventory::checkValid(const TimeSpec& ts)
{
  bool res = true;
  CHECK_TOKEN(r_snapshots);
  CHECK_TOKEN(w_snapshots);
  CHECK_TOKEN(r_dusime);
  all_valid = res;
}

const std::string SnapshotInventory::findUniqueName()
{
  std::string newname = snapname;
  while (snapmap.find(newname) != snapmap.end()) {
    std::stringstream trynew;
    trynew << snapname << "-"
           << std::setfill('0') << std::setw(8) << std::hex
           << (rand() % 0x100000000UL);
    newname = trynew.str();
  }
  return newname;
}

static const std::string snapshotFileName(std::string base,
                                          const std::string& ext)
{
  // sanitize base name
  char illegal_unwanted[] =
    { '/', '\\', ':', '*', '?', '"', '<', '>', '|', '\n', '\t' };
  for (char c: illegal_unwanted) {
    std::replace(base.begin(), base.end(), c, '_');
  }

  // search until the file does not exist
  while (true) {
    std::stringstream trynew;
    trynew << base << "-" << std::setfill('0') << std::setw(6) << std::hex
           << (rand() % 0x1000000UL) << ext;

    if (access(trynew.str().c_str(), F_OK) == -1) {
      // should add more tests, but maybe simply no file there
      return trynew.str();
    }
  }
}

void SnapshotInventory::receiveSnapshot(const TimeSpec& ts)
{
  while (r_snapshots.haveVisibleSets(ts)) {
#if CATCH_TOML_ERROR
    try
#endif
      {
        DataReader<Snapshot,VirtualJoin> sn(r_snapshots, ts);
        if (ts.getValidityStart() > latest_incoming) {
          latest_incoming = ts.getValidityStart();
          DEB("First data for snapshot tagged " << latest_incoming);
          current_snapset = snapmap.emplace
            (findUniqueName(),
             std::chrono::system_clock::now()).first;
          toml::value newset
            { { "datetime", toml::local_datetime(current_snapset->second.time)},
              { "initial", toml::array() } };
          if (!tomlsnp.contains("initial_set")) {
            tomlsnp["initial_set"] = toml::table();
          }
          // toml::find<toml::array>(tomlsnp, "initial_set").push_back(newset);
          tomlsnp["initial_set"][current_snapset->first] = newset;
          for (auto &fn: newset_clients) {
            fn(current_snapset->first, current_snapset->second);
          }
          loaded = current_snapset->first;

          // assume this will complete
          setMode(IncoRecorded);
        }
        else {
          // check that this module (or a name-alike) has not yet entered
          // this snap
          for (const auto &snap: current_snapset->second.snaps) {
            if (snap.originator == sn.data().originator) {
              throw(double_snapshot_origin(sn.data().originator.name.c_str()));
            }
          }
        }
        current_snapset->second.snaps.push_back(sn.data());

        std::string snapfilename;
        if (sn.data().saveExternal()) {
          snapfilename = snapshotFileName(current_snapset->first,
                                          sn.data().fileExtension());
        }

        tomlsnp["initial_set"][current_snapset->first]["initial"].push_back
          (sn.data().tomlCode(snapfilename));
        for (auto& fn: newsnap_clients) {
          fn(sn.data());
        }
      }
#if CATCH_TOML_ERROR
    catch (const exception& e) {
      /* DUSIME replay&initial

         Problem handling an incoming snapshot */
      E_XTR("Handling snapshot for \"" << entity
            << "\", exception " << e.what());
    }
#endif
  }
}

void SnapshotInventory::followDusime(const TimeSpec& ts)
{
 try {
   DataReader<EntityCommand,MatchIntervalStartOrEarlier> rd(r_dusime, ts);
       if (rd.data().command == EntityCommand::NewState) {
      switch(rd.data().new_state.t) {
      case SimulationState::Advance:
      case SimulationState::Replay:
        setMode(UnSet);
        loaded = "";
        break;
      default:
        break;
      }
    }
  }
  catch (const std::exception& e) {
    /* DUSIME replay&initial

       Not able to follow DUSIME command change. Please report. */
    W_XTR("SnapshotInventory failure reading DUSIME command " << e.what());
  }
}


SnapshotInventory::SnapshotData::
SnapshotData(const std::chrono::system_clock::time_point& tm) :
  snaps(),
  time(tm)
{
  //
}

std::string SnapshotInventory::SnapshotData::getTimeLocal() const
{
  time_t tt = std::chrono::system_clock::to_time_t(time);
  std::tm tm = *std::localtime(&tt);
  //return std::ctime(&time)
  stringstream tmp;
  tmp << std::put_time(&tm, "%Y %b %d %H:%M");
  return tmp.str();
}

void SnapshotInventory::setFiles(const std::string& bfile,
                                 const std::string& sfile)
{
  resultfile = sfile;

  if (bfile.size()) {
    try {
      tomlsnp = toml::parse<toml::preserve_comments>(bfile);

      // check that the entity is correct
      if (entity != toml::find<std::string>(tomlsnp, "entity")) {
        throw (initial_file_mismatch(bfile.c_str()));
      }

      // follow the table/map of initial_set objects, all named
      for (const auto &iset: toml::find<toml::table>(tomlsnp, "initial_set")) {
        const auto date = toml::find<toml::local_datetime>
          (iset.second, "datetime");
        const auto name = iset.first;
        auto tmpsnap =
          snapmap.emplace(name, std::chrono::system_clock::time_point(date));

        // read the [[initial]] array in there, thanks to a constructor
        // for Snapshot from toml::value, this seems easy
        auto iniarr = toml::find<toml::array>(iset.second, "initial");
        for (const auto &ini: iniarr) {
          tmpsnap.first->second.snaps.emplace_back(ini);
        }
      }
    }
    catch (const exception& e) {
      /* DUSIME replay&initial

         Trying to read a file with initial (snapshot) states. This
         file should be in TOML format, most likely either the file is
         not readable, or there is a format error.
      */
      W_MOD("Error reading initial file " << bfile << " :" << e.what());
    }
  }
  else {
    tomlsnp = toml::value({ { "entity", entity} });
  }

  // set the new state; unset
  setMode(UnSet);
}


void SnapshotInventory::saveFile() const
{
  if (resultfile.size()) {
    try {
      std::ofstream outfile(resultfile.c_str());
      outfile << std::setw(76) << std::setprecision(12) << tomlsnp;
    }
    catch (const std::exception& e) {
      /* DUSIME replay&initial

         An error occurred when trying to write a file with updated/
         added initial (snapshot) states. Check the message, see whether
         file or folder is writable.
      */
      W_XTR("Error trying to write initial states file \"" << resultfile <<
            "\" : " << e.what());
    }
  }
}

const SnapshotInventory::pointer
SnapshotInventory::findSnapshotInventory(const std::string& entity)
{
  const auto entry = inventories.find(entity);
  if (entry == inventories.end()) {

    // create an inventory for this entity
    auto reslt = inventories.emplace
      (entity, new SnapshotInventory(entity.c_str()));
    return reslt.first->second;
  }
  return entry->second;
}

ObjectType SnapshotInventory::getObjectType() const
{
  return O_DuecaSupport;
}

bool SnapshotInventory::sendSelected()
{
  auto mapit = snapmap.find(selected);
  if (mapit == snapmap.end()) {
    /* DUSIME replay&initial

       Cannot find the given selected initials to send */
    W_XTR("Entity " << entity << ", cannot send initial states \"" <<
          selected << "\"");
    return false;
  }

  for (const auto &snap: mapit->second.snaps) {
    DataWriter<Snapshot> ds(w_snapshots);
    ds.data() = snap;
  }

  // new inco loaded
  setMode(IncoLoaded);
  loaded = selected;

  return true;
}


void SnapshotInventory::setMode(IncoInventoryMode mode)
{
  state = mode;
  static const std::string empty;
  switch (mode) {
  case IncoLoaded:
    for (const auto& fn: newmode_clients) {
      fn(mode, selected);
    }
    break;
  case IncoRecorded:
    for (const auto& fn: newmode_clients) {
      fn(mode, current_snapset->first);
    }
    break;
  default:
    for (const auto& fn: newmode_clients) {
      fn(mode, empty);
    }
  }
}

DUECA_NS_END;
