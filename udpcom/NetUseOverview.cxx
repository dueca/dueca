/* ------------------------------------------------------------------   */
/*      item            : NetUseOverview.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Mon Apr 19 23:01:51 2021
        category        : body file
        description     :
        changes         : Mon Apr 19 23:01:51 2021 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define NetUseOverview_cxx

// include the definition of the module class
#include "NetUseOverview.hxx"

// include the debug writing header, by default, write warning and
// error messages
#include <debug.h>

// include additional files needed for your calculation here
#define DEBPRINTLEVEL -1
#include <debprint.h>
#include <DataReader.hxx>
#include <DataWriter.hxx>
#include <ParameterTable.hxx>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <Callback.hxx>
#include <CallbackWithId.hxx>
#define NO_TYPE_CREATION
#include <dueca.h>

DUECA_NS_START;

// class/module name
const char* const NetUseOverview::classname = "net-use-overview";

// Parameters to be inserted
const ParameterTable* NetUseOverview::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "Produce files with data on the network communication timing and\n"
      "network communication load" }
  };

  return parameter_table;
}

// constructor
NetUseOverview::NetUseOverview(Entity* e, const char* part, const
                   PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  cb0(this, &_ThisModule_::channelOpen),

  // entry 0 should be the timing data
  r_timingdata(getId(), NameSet("NetCommLog://dueca"),
               NetTimingLog::classname, 0,
               Channel::Events, Channel::OneOrMoreEntries,
               Channel::ReadAllData, 0.2, &cb0),

  // watcher to get info on all capacity data
  watch_useinfo(this),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::processNetUseData),
  // the module's main activity
  do_calc(getId(), "update net use info", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(r_timingdata);

  // open the logfiles
  nettiming_file.open("dueca.nettiming");
  netload_file.open("dueca.netload");
}

void NetUseOverview::channelOpen(const TimeSpec& ts)
{
  // after the validity callback, write the header in the files
  // the label 
  ChannelEntryInfo ei = r_timingdata.getChannelEntryInfo();
  NetCapacityLog::printhead(netload_file, ei.entry_label);
  NetTimingLog::printhead(nettiming_file, ei.entry_label);
}


bool NetUseOverview::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  do_calc.switchOn(0);
  return true;
}

// destructor
NetUseOverview::~NetUseOverview()
{
  nettiming_file.close();
  netload_file.close();
}

// tell DUECA you are prepared
bool NetUseOverview::isPrepared()
{
  bool res = true;

  CHECK_TOKEN(r_timingdata);
  for (auto &nl: watch_useinfo.loads) {
    CHECK_TOKEN(nl->r_info);
  }

  // return result of checks
  return res;
}

// start the module
void NetUseOverview::startModule(const TimeSpec &time)
{
  //do_calc.switchOn(time);
}

// stop the module
void NetUseOverview::stopModule(const TimeSpec &time)
{
  //do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void NetUseOverview::processNetUseData(const TimeSpec& ts)
{
  DataReader<NetTimingLog> rt(r_timingdata, ts);
  rt.data().printline(nettiming_file, ts.getValidityStart());
  updateTiming(rt.data());
}

void NetUseOverview::processLoadInfo(const TimeSpec& ts,
                                     ChannelReadToken *&r_token)
{
  DataReader<NetCapacityLog> rc(*r_token, ts);
  rc.data().printline(netload_file, ts.getValidityStart());
  updateLoad(rc.data());
}


NetUseOverview::NetLoadEntry::NetLoadEntry
(const NameSet& ns, entryid_type entry_id,
 const char* classname, NetUseOverview *ptr,
 void (NetUseOverview::*h)(const TimeSpec& t, ChannelReadToken *&r)) :
  r_info(ptr->getId(), ns, classname, entry_id, Channel::Events,
         Channel::OneOrMoreEntries, Channel::ReadAllData),
  cb(ptr, h, &r_info),
  get_info(ptr->getId(), "receive net load info", &cb, PrioritySpec(0,0))
{
  get_info.setTrigger(r_info);
  get_info.switchOn();
}

NetUseOverview::WatchNetLoadInfo::WatchNetLoadInfo(NetUseOverview* ptr) :
  ChannelWatcher(NameSet("NetCommLog://dueca")),
  ptr(ptr)
{
  //
}

void NetUseOverview::WatchNetLoadInfo::entryAdded(const ChannelEntryInfo& i)
{
  if (i.data_class == std::string(NetCapacityLog::classname)) {
    loads.push_back
      (std::shared_ptr<NetUseOverview::NetLoadEntry>
       (new NetLoadEntry(NameSet("NetCommLog://dueca"),
                         i.entry_id, NetCapacityLog::classname,
                         ptr, &NetUseOverview::processLoadInfo)));
  }
}

void NetUseOverview::updateTiming(const NetTimingLog& data)
{
  // not reimplemented here
}

void NetUseOverview::updateLoad(const NetCapacityLog& data)
{
  // not reimplemented here
}


DUECA_NS_END;

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
//static TypeCreator<NetUseOverview> a(NetUseOverview::getMyParameterTable());

