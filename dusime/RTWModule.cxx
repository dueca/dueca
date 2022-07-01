/* ------------------------------------------------------------------   */
/*      item            : RTWModule.cxx
        made by         : Joost Ellerbroek
        date            : 080208
        category        : body file
        description     :
        changes         : 080208 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek
        license         : EUPL-1.2
*/

#include "RTWModule.hxx"

#include <EventReader.hxx>
#include <debug.h>
#include "Snapshot.hxx"
#include <XmlSnapshot.hxx>

#define DO_INSTANTIATE
#include "Event.hxx"
#include "Callback.hxx"
#include "EventAccessToken.hxx"
#include <NameSet.hxx>
#include <WrapSendEvent.hxx>
#include <debprint.h>

DUECA_NS_START

RTWModule::RTWModule(Entity* e, const char* m_class, const char* part,
                     const IncoTable* inco_table, int state_size) :
  SimulationModule(e, m_class, part, inco_table, state_size),

  xml_snap_state(SnapshotState::SnapClear),
  future_xml_snap_time(MAX_TIMETICK),

  // a callback to my module that processes the data on the xml snapshot channel
  cb1(this, &RTWModule::receiveXmlSnapshot),
  cb2(this, &RTWModule::initXmlChannels),

  // a token for reading commands from the entity
  r_xml_snap(getId(), NameSet(getEntity(), "XmlSnapshot", "get"),
             ChannelDistribution::NO_OPINION, Bulk, &cb2),

  // a write token, for sending confirmation
  w_xml_snap(getId(), NameSet(getEntity(), "XmlSnapshot", "set"),
             ChannelDistribution::NO_OPINION, Bulk),

  // make an activity
  xml_snap_recv(getId(), "process xml snapshot events", &cb1, PrioritySpec(0, 0))
{
  //
}

RTWModule::~RTWModule()
{
  //
}

void RTWModule::receiveXmlSnapshot(const TimeSpec& ts)
{
  if (!r_xml_snap.isValid()) {
    /* DUSIME system.

       Received a snapshot with invalid xml. Fix the file with the
       snapshot. */
    W_MOD("cannot read xml snapshot channel");
    return;
  }

  // read the snapshot from the channel
  EventReader<XmlSnapshot> r(r_xml_snap, ts);
  DEB1(getId() << "Me: " << getNameSet().getEntity() << ' ' << getNameSet().getClass() << ' ' << getNameSet().getPart());
  DEB1(getId() << "Incoming packet: " << r.data().originator.getEntity() << ' ' << r.data().originator.getClass() << ' ' << r.data().originator.getPart());
  // check that it is for me
  if (r.data().originator != getNameSet()) {
    /* DUSIME system.

       Information message, Received a snapshot but for another module. */
    I_MOD(getId() << " xml snapshot ignored, not for me");
  }
  else if (r.data().data.size() > 1) {

    // get the descendant to load it
    loadXmlSnapshot(ts, r.data());
    DEB1(getId() << " restored state from xml snapshot at " << ts);
  } else if (r.data().data.size() == 1) {
    // The event is meant as a request for a new snapshot
    XmlSnapshot::XmlSnapshotCommand cmd;
    AmorphReStore a(r.data().accessData(), 1);
    unPackData(a, cmd);

    switch (cmd) {
      case XmlSnapshot::PrepareXmlSnapshot:
        xml_snap_state = SnapshotState::SnapPrepared;
        future_xml_snap_time  = r.getTick();
        break;

      case XmlSnapshot::SendXmlSnapshot:
        // allocate the snapshot
        XmlSnapshot* snap = new XmlSnapshot(getNameSet());

        // get the descendant to generate data for the xml snapshot
        fillXmlSnapshot(ts, *snap);

        // send it off
        if (w_xml_snap.isValid()) {
          wrapSendEvent(w_xml_snap, snap, ts.getValidityStart());
        }
        else {
          delete snap;
          /* DUSIME system.

             Snapshot write token not valid, cannot send snapshot,
             discarding it. */
          W_MOD(getId() << " XmlSnapshot event write token not valid");
        }

        // update the state
        future_xml_snap_time = MAX_TIMETICK;
        snap_state = SnapshotState::SnapSent;

        DEB1(getId() << " sent off xml snapshot at " << ts);
        break;
    }
  }
}

void RTWModule::initXmlChannels(const TimeSpec& ts)
{
  r_xml_snap.isValid();

  // specify that the activity should take place upon data reception
  // from the entity
  xml_snap_recv.setTrigger(r_xml_snap);
  xml_snap_recv.switchOn(ts);
}

bool RTWModule::XmlSnapshotNow(const TimeSpec& ts)
{
  if (xml_snap_state == SnapshotState::SnapPrepared &&
      future_xml_snap_time <= ts.getValidityStart())
    xml_snap_state = SnapshotState::SnapNow;

  return xml_snap_state == SnapshotState::SnapNow;
}

void RTWModule::fillXmlSnapshot(const TimeSpec& ts, XmlSnapshot& snap)
{
  // check one thing, if the fillXmlSnapshot was not overloaded, then
  // the snapshot contains random data. That is at least a warning!
  /* DUSIME system.

     The fillXmlSnapshot method has not been overridden, no data being
     sent */
  W_MOD("module " << getId() << " did not fill xml snapshot data");
}

void RTWModule::loadXmlSnapshot(const TimeSpec &ts, const XmlSnapshot &snap)
{
  // check one thing, if the loadXmlSnapshot was not overloaded, then
  // snapshot loading did not work
  /* DUSIME system.

     The loadXmlSnapshot method has not been overridden, snapshot data
     ignored. */
  W_MOD("module " << getId() << " did not restore from xml snapshot data");
}
DUECA_NS_END
