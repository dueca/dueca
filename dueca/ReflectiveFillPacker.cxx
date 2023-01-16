/* ------------------------------------------------------------------   */
/*      item            : ReflectiveFillPacker.cxx
        made by         : Rene' van Paassen
        date            : 010814
        category        : body file
        description     :
        changes         : 010814 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ReflectiveFillPacker_cxx

#include "ReflectiveFillPacker.hxx"
#include <AmorphStore.hxx>
#include <NameSet.hxx>
#include <TimeSpec.hxx>
#include <Trigger.hxx>
#include <UnifiedChannel.hxx>
#include <TransportNotification.hxx>
//#define D_SHM
//#define I_SHM
#define W_SHM
#define E_SHM
#include "debug.h"
#include <NameSet.hxx>
#include <Ticker.hxx>
#include <CriticalActivity.hxx>
#include <StoreInformation.hxx>
#include "ParameterTable.hxx"
#include <dueca/DataWriter.hxx>
#include <boost/lexical_cast.hpp>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <AsyncList.hxx>
#include "dueca_assert.h"
#include "VarProbe.hxx"
#include <debprint.h>

DUECA_NS_START

const int ReflectiveFillPacker::no_of_stores = 2;

const ParameterTable* ReflectiveFillPacker::getParameterTable()
{
  static const ParameterTable table[] = {
    { "buffer-size", new VarProbe<ReflectiveFillPacker,int>
      (REF_MEMBER(&ReflectiveFillPacker::store_size)),
      "size of buffer for packing objects into = max size of sent objects,\n"
      "size is in bytes."},
    { "packet-size", new VarProbe<ReflectiveFillPacker,int>
      (REF_MEMBER(&ReflectiveFillPacker::packet_size)),
      "size of chunks in which information is sent" },
    { "set-timing" , new VarProbe<ReflectiveFillPacker,PeriodicTimeSpec>
      (REF_MEMBER(&ReflectiveFillPacker::time_spec)),
      set_timing_description },
    { NULL, NULL,
      "Packs bulk priority data for sending over reflective/shared memory\n"
      "accessors."}
  };
  return table;
}

ReflectiveFillPacker::ReflectiveFillPacker() :
  GenericPacker("reflective-fill-packer"),
  store(new AmorphStore[no_of_stores]),
  store_to_fill(0),
  store_to_send(1),
  bytes_to_send(0),
  index_to_send(0),
  packet_size(64),
  time_spec(0, 100),
  store_size(1024),
  wait_period(20),
  out(NULL),
  cb1(this, &ReflectiveFillPacker::sendAPiece),
  send_stuff(getId(), "refl fill packer send", &cb1, PrioritySpec(0, -50))
{
  //
}

bool ReflectiveFillPacker::complete()
{
  if (store_size <= 0 || packet_size <= 0
      || time_spec.getValiditySpan() <= 0) {
    /* DUECA shared memory network.

       parameters ReflectiveFillPacker incorrect. */
    E_CNF("fill packer parameters error");
    return false;
  }

  // let the stores allocate memory for sending
  for (int ii = no_of_stores; ii--; ) {
    store[ii].renewBuffer(store_size);
    store[ii].reUse();
  }

  // trigger sending from the ticker, specify the timing and switch on
  send_stuff.setTimeSpec(time_spec);

  return true;
}

const char* ReflectiveFillPacker::getTypeName()
{
  return "ReflectiveFillPacker";
}

ReflectiveFillPacker::~ReflectiveFillPacker()
{
  delete[] store;
}

void ReflectiveFillPacker::initialise(const ReflectiveStoreInformation& i)
{
  // open the channel
  /* DUECA shared memory network.

     Opening a channel for bulk information collecting. */
  I_SHM(getId() << "Opening output channel");

  // first time entry, open the channel
  out = new ChannelWriteToken
    (getId(), NameSet("dueca", "FillSet", i.node_id),
     getclassname<FillSet>(), std::string("fill pack ") +
     boost::lexical_cast<std::string>(unsigned(i.node_id)),
     Channel::Events, Channel::OnlyOneEntry);


  // adjust the wait period to be 2 s, but at least 10 cycles
  wait_period = max(10, int(2.0/time_spec.getDtInSeconds()));

  send_stuff.setTrigger(*Ticker::single());
  send_stuff.switchOn(TimeSpec(0,0));
}

void ReflectiveFillPacker::stopPacking()
{
  send_stuff.switchOff(TimeSpec(0, 0));
}

bool ReflectiveFillPacker::packOneSet(AmorphStore& s, const PackUnit& c)
{
  // 2 bytes, the channel id, with information about direction, and
  // the data-follows bit
  c.entry->codeHead(s);

  // 4 more bytes, the big start mark, gives length of data
  s.startBigMark();

  // unknown no of bytes, data in channel
  c.entry->packData(s, c.idx, c.tick);

  // write the length of the data in the mark
  s.endBigMark();
  return true;
}

void ReflectiveFillPacker::packWork()
{
    // remember how much data was in the store
  int old_level = store[store_to_fill].getSize();
  DEB1("rfp store " << store_to_fill << " at " << old_level);

  // two pieces of code, with and without exceptions
  try {

    while (work_queue.notEmpty() && !store[store_to_fill].isChoked()) {

      // store one of the waiting data sets
      packOneSet(store[store_to_fill], work_queue.front());

      // assure that the pack is complete, progresses channel reading
      work_queue.front().entry->packComplete(work_queue.front().idx);

      // if we get here, there were no exceptions thrown at us. nice.
      old_level = store[store_to_fill].getSize();
      DEB1("fp store packed one, now at " << old_level);
      work_queue.pop();
    }
  }
  catch(AmorphStoreBoundary& e) {

    // in case something did not fit
    if (old_level == 0) {
      /* DUECA shared memory network.

         Channel data was to large to fit into the Bulk
         store. Increase the store size for the reflective packer, of
         make your DCO objects smaller.
      */
      W_SHM("Object too large for fill store, channel "
            << work_queue.front().entry->getChannelName());

      // throw this out. This means the event never gets sent. Not
      // good! But keep on assure that the pack is complete,
      // progresses channel reading
      work_queue.front().entry->packComplete(work_queue.front().idx);

      work_queue.pop();

      // this is serious enough to stop critical activities
      CriticalActivity::criticalErrorNodeWide();
    }
    else {
      work_queue.front().entry->packFailed(work_queue.front().idx);
    }

    // and reset the size to what we had before
    DEB1("Object did not fit, resetting to " << old_level);
    store[store_to_fill].setSize(old_level);
  }
  catch(NoDataAvailable& e) {

    // data from the backlog queue has been lost
    /* DUECA shared memory network.

       Have lost data for packing. Should be impossible with new DUECA
       versions. */
    W_SHM("data lost for " << work_queue.front().entry->getChannelName());

    // it failed, pop it away
    work_queue.pop();

    // and reset the size to what we had before
    store[store_to_fill].setSize(old_level);
  }
}

void ReflectiveFillPacker::sendAPiece(const TimeSpec& ts)
{
  // pack all data
  packWork();

  // check that the channel is valid, there is a small waiting period,
  // 10 cycles, after first isValid() to be reasonably sure that the
  // channel is also opened by the readers.
  if (wait_period) {
    if (out->isValid()) wait_period--;
    return;
  }

  // if there is nothing to send (bytes_to_send == 0), then try to
  // access the state, don't block, and try to grab the other store
  if (!bytes_to_send) {

    // tryState accessed the state, apparently no block
    if (store[store_to_fill].getSize()) {

      // Yes, start on this store now, switch stores
      DEB1(getId() << " switching to store " << store_to_fill
            << " with " << store[store_to_fill].getSize());
      bytes_to_send = store[store_to_fill].getSize();
      index_to_send = 0;
      store_to_send = store_to_fill;
      if (++store_to_fill == no_of_stores) store_to_fill = 0;
      store[store_to_fill].reUse();
    }
  }

  if (bytes_to_send != 0) {

    int send_size = min(packet_size, bytes_to_send);

    {
      DataWriter<FillSet> fs(*out, ts, send_size);

      // pack in the data
      std::memcpy(fs.data().data.ptr(),
                  &(store[store_to_send].getToData())[index_to_send],
                  send_size);
    }
    DEB1("sending a fill packet of " << send_size << " bytes");

    index_to_send += send_size;
    bytes_to_send -= send_size;
  }
}

ostream& operator << (ostream& os, const ReflectiveFillPacker& o)
{
  return os << "ReflectiveFillPacker(" << o.getId() << ')';
}

DUECA_NS_END
