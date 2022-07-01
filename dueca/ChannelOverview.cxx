/* ------------------------------------------------------------------   */
/*      item            : ChannelOverview.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Mon Apr 23 18:18:36 2018
        category        : body file
        description     :
        changes         : Mon Apr 23 18:18:36 2018 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ChannelOverview_cxx

// include the definition of the module class
#include "ChannelOverview.hxx"

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

// include additional files needed for your calculation here
#define DEBPRINTLEVEL -1
#include <debprint.h>
#include "ChannelDataMonitor.hxx"
#include <DataReader.hxx>
#include <DataWriter.hxx>
#include <ParameterTable.hxx>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <Callback.hxx>
#include <CallbackWithId.hxx>
#define NO_TYPE_CREATION
#include <dueca.h>

DUECA_NS_START

// class/module name
const char* const ChannelOverview::classname = "channel-overview";

// Parameters to be inserted
const ParameterTable* ChannelOverview::getMyParameterTable()
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
      "Produces an overview of the channels used in this DUECA process.\n"
      "In addition, individual channel entries may be queried for their.\n"
      "current data."} };

  return parameter_table;
}

// constructor
ChannelOverview::ChannelOverview(Entity* e, const char* part, const
                   PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  cmanager(ChannelManager::single()),
  countid(0),

  // channel watchers
  watch_readinfo(this),
  watch_writeinfo(this),

  // channels
  w_countreq(getId(), NameSet("ChannelCountRequest://dueca"),
             ChannelCountRequest::classname, "",
             Channel::Events, Channel::OnlyOneEntry,
             Channel::OnlyFullPacking, Channel::Bulk),

  r_countres(getId(), NameSet("ChannelCountResult://dueca"),
             ChannelCountResult::classname, entry_any,
             Channel::Events, Channel::ZeroOrMoreEntries,
             Channel::ReadAllData),

  w_monitorreq(getId(), NameSet("ChannelMonitorRequest://dueca"),
             ChannelMonitorRequest::classname, "",
             Channel::Events, Channel::OnlyOneEntry,
             Channel::OnlyFullPacking, Channel::Bulk),

  r_monitorres(getId(), NameSet("ChannelMonitorResult://dueca"),
             ChannelMonitorResult::classname, entry_any,
             Channel::Events, Channel::OneOrMoreEntries,
             Channel::ReadAllData),

  delay_countcollect(max(int(0.05/Ticker::single()->getTimeGranule()),
                         Ticker::single()->getCompatibleIncrement())),
  count_check(),
  cb1(this, &_ThisModule_::processCount),
  do_count(getId(), "process incoming count", &cb1, PrioritySpec(0, 0)),
  cb2(this, &_ThisModule_::processMonitorData),
  do_monitor(getId(), "process monitor data", &cb2, PrioritySpec(0, 0))
{
  do_count.setTrigger(count_check);
  do_count.switchOn();
  do_monitor.setTrigger(r_monitorres);
  do_monitor.switchOn();
  readinfo_file.open("dueca.channelreadinfo");
  ChannelReadInfo::printhead(readinfo_file);
  writeinfo_file.open("dueca.channelwriteinfo");
  ChannelWriteInfo::printhead(writeinfo_file);
}

bool ChannelOverview::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
ChannelOverview::~ChannelOverview()
{
  readinfo_file.close();
  writeinfo_file.close();
}

// as an example, the setTimeSpec function
bool ChannelOverview::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // or do this with the clock if you have it (don't do both!)
  // myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// tell DUECA you are prepared
bool ChannelOverview::isPrepared()
{
  bool res = true;

  for (t_monitorentrylist::iterator tt = watch_readinfo.r_readinfo.begin();
       tt !=  watch_readinfo.r_readinfo.end(); tt++) {
    CHECK_TOKEN((*tt)->r_info);
  }
  for (t_monitorentrylist::iterator tt = watch_writeinfo.r_writeinfo.begin();
       tt !=  watch_writeinfo.r_writeinfo.end(); tt++) {
    CHECK_TOKEN((*tt)->r_info);
  }
  CHECK_TOKEN(w_countreq);
  CHECK_TOKEN(r_countres);

  // return result of checks
  return res;
}

// start the module
void ChannelOverview::startModule(const TimeSpec &time)
{
  //do_calc.switchOn(time);
}

// stop the module
void ChannelOverview::stopModule(const TimeSpec &time)
{
  //do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void ChannelOverview::processReadInfo(const TimeSpec& ts, ChannelReadToken*& r)
{
  while (r->getNumVisibleSets()) {
    try {
      DataReader<ChannelReadInfo> ri(*r);
      _processReadInfo(ri.data());
      ri.data().printline(readinfo_file);
      DEB("read info " << ri.data());
    }
    catch (const std::exception& e) {
      /* DUECA UI.

         Unexpected error, cannot access expected information on a
         channel reading client.
       */
      W_STS("cannot access ChannelReadInfo " << e.what());
    }
  }
}

struct match_readinfo
{
  const uint32_t creationid;
  match_readinfo(const uint32_t& ri) : creationid(ri) { }
  bool operator()
  (const
   boost::shared_ptr<ChannelOverview::ChannelInfoSet::
   EntryInfoSet::ReadInfoSet>& s)
  { return creationid == s->rdata.creationid; }
};

unsigned ChannelOverview::_processReadInfo(const ChannelReadInfo& data)
{
  unsigned chanid = data.channelid.getObjectId();
  entryid_type entryid = data.entryid;

  if (chanid >= infolist.size() || infolist[chanid].get() == NULL) {
    // reserve the data
    DEB("read info, channel not yet known");
    tmplist.push_back(data);
    return 0U;
  }

  if (entryid >= infolist[chanid]->entries.size() ||
      infolist[chanid]->entries[entryid].get() == NULL) {
    DEB("read info, entry not yet known");
    tmplist.push_back(data);
    return 0U;
  }

  if (data.channelid.getLocationId() == 0) {
    infolist[chanid]->accessfromzero = true;
  }

  unsigned readerid = 0;
  if (data.action == ChannelReadInfo::Deleted ||
      data.action == ChannelReadInfo::Detached) {
    auto re = std::find_if
      (infolist[chanid]->entries[entryid]->rdata.begin(),
       infolist[chanid]->entries[entryid]->rdata.end(),
       match_readinfo(data.creationid));
    if (re != infolist[chanid]->entries[entryid]->rdata.end()) {
      readerid = (*re)->readerid;
      infolist[chanid]->entries[entryid]->rdata.erase(re);
    }
    else {
      DEB("entry already disappeared, creationid " << data.creationid);
    }
  }
  else {
    readerid = infolist[chanid]->entries[entryid]->readerid++;
    infolist[chanid]->entries[entryid]->rdata.push_back
      (boost::shared_ptr<ChannelInfoSet::EntryInfoSet::ReadInfoSet>
       (new ChannelInfoSet::EntryInfoSet::ReadInfoSet(readerid, data)));
  }

  reflectChanges(chanid, entryid, data.creationid);
  return readerid;
}

ChannelOverview::ChannelInfoSet::ChannelInfoSet(const std::string& name,
                                                bool accesszero) :
  name(name),
  accessfromzero(false)
{ }

ChannelOverview::ChannelInfoSet::EntryInfoSet::EntryInfoSet
(const ChannelWriteInfo& wdata) :
  readerid(0U),
  wdata(wdata),
  seq_id(0),
  monitor(NULL)
{ }

ChannelOverview::ChannelInfoSet::EntryInfoSet::ReadInfoSet::ReadInfoSet
(unsigned readerid, const ChannelReadInfo& rdata) :
  readerid(readerid),
  rdata(rdata),
  seq_id(0)
{ }

void ChannelOverview::processWriteInfo(const TimeSpec& ts, ChannelReadToken*& r)
{
  while (r->getNumVisibleSets()) {
    try {
      DataReader<ChannelWriteInfo> wi(*r);
      DEB("wrte info " << wi.data());
      wi.data().printline(writeinfo_file);

      // ensure the channel info entry is there
      unsigned chanid = wi.data().channelid.getObjectId();

      if (chanid >= infolist.size()) {
        infolist.resize(chanid+1);
      }

      // create info entry for the channel if not yet present
      if (!infolist[chanid].get()) {
        infolist[chanid].reset
          (new ChannelInfoSet
           (ChannelManager::single()->getGlobalNameSet(chanid).name,
            wi.data().channelid.getLocationId() == 0));

        // creates the channel with entries
        reflectChanges(chanid);
      }
      else {
        if (wi.data().channelid.getLocationId() == 0) {
          infolist[chanid]->accessfromzero = true;
        }
      }

      // entries sizing
      entryid_type entryid = wi.data().entryid;
      if (entryid >= infolist[chanid]->entries.size()) {
        infolist[chanid]->entries.resize(entryid+1);
      }

      // add or remove the entry
      if (wi.data().clientid.validId()) {

	if (infolist[chanid]->entries[entryid].get() != NULL) {
	/** DUECA UI.
	    
	    For the ChannelOverview, a message is generated that
	    indicates a new channel entry, but the entry is already
	    reported as created. These glitches are sometimes
	    introduced by incomplete creation and subsequent deletion
	    of modules, due to errors in the start script. Check
	    preceding messages.
	*/
	  W_STS("Double entry configuration detected " << entryid <<
		" channel #" << chanid);
	  
	  // just in case, remove the monitor
	  delete infolist[chanid]->entries[entryid]->monitor;
	  infolist[chanid]->entries[entryid].reset();
	}
	
        infolist[chanid]->entries[entryid].reset
          (new ChannelInfoSet::EntryInfoSet(wi.data()));

        // delete, add, replace the entry
        reflectChanges(chanid, entryid);

        for (auto ri = tmplist.begin(); ri != tmplist.end(); ) {
          if (ri->channelid.getObjectId() == chanid &&
              ri->entryid == wi.data().entryid) {
            DEB("rply info " << *ri);
            _processReadInfo(*ri);
            ri = tmplist.erase(ri);
          }
          else {
            ri++;
          }
        }
      }
      else {
        if (infolist[chanid]->entries[entryid].get() != NULL) {

	  // just in case, remove the monitor
	  delete infolist[chanid]->entries[entryid]->monitor;
	  infolist[chanid]->entries[entryid].reset();
	  
	  // delete, add, replace the entry
	  reflectChanges(chanid, entryid);
	}
	else {
	  /** DUECA UI.

	      For the ChannelOverview, a message is generated that
	      indicates a disappearing channel entry, but the entry as
	      such has not been reported as created. These glitches
	      are sometimes introduced by incomplete creation and
	      subsequent deletion of modules, due to errors in the
	      start script. Check preceding messages indicating that.
	  */
	  W_STS("Cannot find dissappearing entry " << entryid <<
		" channel #" << chanid);
	}
      }

    }
    catch (const std::exception& e) {
      /* DUECA UI.

         Unexpected error. Cannot access or process information on a
         channel writing access. */
      W_STS("cannot access or process ChannelWriteInfo " << e.what());
    }
  }
}

ChannelOverview::WatchWriteInfo::WatchWriteInfo(ChannelOverview* ptr) :
  ChannelWatcher(NameSet("ChannelWriteInfo://dueca")),
  ptr(ptr)
{
  //
}

void ChannelOverview::WatchWriteInfo::entryAdded(const ChannelEntryInfo& i)
{
  DEB("write info additional entry " << i)
  r_writeinfo.push_back
    (boost::shared_ptr<MonitorEntry>
     (new MonitorEntry(NameSet("ChannelWriteInfo://dueca"), i.entry_id,
                       "ChannelWriteInfo",i.origin, ptr,
                       &ChannelOverview::processWriteInfo)));
}

void ChannelOverview::WatchWriteInfo::entryRemoved(const ChannelEntryInfo& i)
{
  DEB("write info remove entry " << i)
}

ChannelOverview::WatchReadInfo::WatchReadInfo(ChannelOverview* ptr) :
  ChannelWatcher(NameSet("ChannelReadInfo://dueca")),
  ptr(ptr)
{
  //
}

void ChannelOverview::WatchReadInfo::entryAdded(const ChannelEntryInfo& i)
{
  DEB("read info additional entry " << i)
  r_readinfo.push_back
    (boost::shared_ptr<MonitorEntry>
     (new MonitorEntry(NameSet("ChannelReadInfo://dueca"), i.entry_id,
                       "ChannelReadInfo", i.origin, ptr,
                       &ChannelOverview::processReadInfo)));
}

void ChannelOverview::WatchReadInfo::entryRemoved(const ChannelEntryInfo& i)
{
  DEB("read info remove entry " << i)
}

ChannelOverview::MonitorEntry::MonitorEntry
  (const NameSet& ns, entryid_type entry_id,
   const char* classname, const GlobalId& origin,
   ChannelOverview *ptr,
   void (ChannelOverview::*h)(const TimeSpec& t, ChannelReadToken *&r)) :
  r_info(ptr->getId(), ns, classname, entry_id, Channel::Events,
         Channel::OneOrMoreEntries, Channel::ReadReservation),
  cb(ptr, h, &r_info),
  get_info(ptr->getId(), "receive channel use info", &cb, PrioritySpec(0,0))
{
  get_info.setTrigger(r_info);
  get_info.switchOn();
}

void ChannelOverview::reflectChanges(unsigned chanid)
{
  DEB("updated channel " << chanid);
}

void ChannelOverview::reflectChanges(unsigned chanid, unsigned entryid)
{
  DEB("updated channel " << chanid << " entry " << entryid);
}

void ChannelOverview::reflectChanges(unsigned chanid, unsigned entryid,
                                     uint32_t readid)
{
  DEB("updated channel " << chanid << " entry " << entryid);
}

void ChannelOverview::reflectCounts()
{
  DEB("new count");
}

void ChannelOverview::showChanges()
{
  //
}

void ChannelOverview::refreshCounts()
{
  TimeTickType tnow = SimTime::now();
  DataWriter<ChannelCountRequest> wc(w_countreq, tnow);
  wc.data().countid = ++countid;
  count_check.requestAlarm(tnow + delay_countcollect);
}

void ChannelOverview::processCount(const TimeSpec& ts)
{
  bool moredata = true;
  bool collected = false;
  while (moredata) {
    try {
      DataReader<ChannelCountResult,VirtualJoin>
        pc(r_countres);
      if (pc.data().countid == countid) {
        _processCount(pc.data(), pc.origin());
      }
      else {
        /* DUECA UI.

           Received an old count result, not matching the current
           request number, ignoring that. 
        */
        W_STS("Ignoring old count result, channel " <<
              pc.data().channelid << " for count " << pc.data().countid);
      }
      collected = true;
    }
    catch (const NoDataAvailable& e) {
      moredata = false;
    }
  }

  reflectCounts();
  showChanges();
  if (collected) { // be sure no more data
    count_check.requestAlarm(SimTime::now() + delay_countcollect);
  }
}

void ChannelOverview::_processCount(const ChannelCountResult& cnt,
                                    const GlobalId& origin)
{
  unsigned chanid = cnt.channelid.getObjectId();
  for (unsigned ee = cnt.entries.size(); ee--; ) {
    entryid_type entryid = cnt.entries[ee].entryid;
    for (unsigned cc = cnt.entries[ee].counts.size(); cc--; ) {
      if (infolist[chanid].get() == NULL ||
          entryid >= infolist[chanid]->entries.size() ||
          infolist[chanid]->entries[entryid].get() == NULL) {
        DEB("count info, channel " << chanid << " entry " <<
            entryid << " not yet known");
      }
      else {
        if (cnt.entries[ee].counts[cc].clientid == 0) {

          // if clientid is zero, the count refers to the write count
          infolist[chanid]->entries[entryid].get()->seq_id =
            cnt.entries[ee].counts[cc].count;
        }
        else {
          auto re = std::find_if
            (infolist[chanid]->entries[entryid]->rdata.begin(),
             infolist[chanid]->entries[entryid]->rdata.end(),
             match_readinfo(cnt.entries[ee].counts[cc].clientid));
          if (re == infolist[chanid]->entries[entryid]->rdata.end()) {
            DEB("Unmatched client count, from id " <<
                cnt.entries[ee].counts[cc].clientid <<
                " channel " << chanid << " entry " << entryid);
          }
          else {
            (*re)->seq_id = cnt.entries[ee].counts[cc].count;
          }
        }
      }
    }
  }
}

void ChannelOverview::refreshMonitor(unsigned channelno, unsigned entryno)
{
  if (channelno < infolist.size() &&
      infolist[channelno].get() != NULL &&
      entryno < infolist[channelno]->entries.size() &&
      infolist[channelno]->entries[entryno]->monitor != NULL) {
    DataWriter<ChannelMonitorRequest> req(w_monitorreq, SimTime::now());

    // write the request
    req.data().chanid = infolist[channelno]->entries[entryno]->wdata.channelid;
    req.data().entryid = entryno;
  }
  else {
    /* DUECA UI.

       The channel entry for which a data view refresh has been
       requested has disappeared. */
    W_STS("Monitor request for a write entry that disappeared " << channelno
          << " entry " << entryno);
  }
}

void ChannelOverview::setMonitor(unsigned channelno, unsigned entryno,
                                 ChannelDataMonitor* monitor)
{
  if (channelno < infolist.size() &&
      infolist[channelno].get() != NULL &&
      entryno < infolist[channelno]->entries.size() &&
      ( (monitor == NULL &&
         infolist[channelno]->entries[entryno]->monitor != NULL) ||
        (monitor != NULL &&
         infolist[channelno]->entries[entryno]->monitor == NULL))) {
    infolist[channelno]->entries[entryno]->monitor = monitor;
  }
  else {
    /* DUECA UI.

       The channel entry for which a data view has been requested has
       disappeared. */
    W_STS("Monitor creation for a write entry that disappeared " << channelno
          << " entry " << entryno);
  }
}

void ChannelOverview::processMonitorData(const TimeSpec& ts)
{
  bool moredata = true;
  while (moredata) {
    try {
      DataReader<ChannelMonitorResult,VirtualJoin>
        mr(r_monitorres);
      unsigned chanid = mr.data().channelid.getObjectId();
      unsigned entryid = mr.data().entryid;
      if (chanid < infolist.size() &&
          infolist[chanid].get() != NULL &&
          entryid < infolist[chanid]->entries.size() &&
          infolist[chanid]->entries[entryid].get() != NULL &&
          infolist[chanid]->entries[entryid]->monitor != NULL) {
        infolist[chanid]->entries[entryid]->monitor->refreshData(mr.data());
      }
      else {
        DEB("Unmatched monitor data " << mr.data());
      }
    }
    catch (const NoDataAvailable& e) {
      moredata = false;
    }
  }
}

void ChannelOverview::closeMonitor(unsigned channelno, unsigned entryno)
{
  DEB("should close " << channelno << " e:" << entryno);
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
//static TypeCreator<ChannelOverview> a(ChannelOverview::getMyParameterTable());

DUECA_NS_END
