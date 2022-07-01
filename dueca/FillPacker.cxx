/* ------------------------------------------------------------------   */
/*      item            : FillPacker.cxx
        made by         : Rene' van Paassen
        date            : 001023
        category        : body file
        description     :
        changes         : 001023 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define FillPacker_cc
#include "FillPacker.hxx"
#include <dueca-conf.h>
#include <iomanip>
#include "AmorphStore.hxx"
#include "TimeSpec.hxx"
#include <CriticalActivity.hxx>
#include "Trigger.hxx"
#include <TransportNotification.hxx>
#include <UnifiedChannel.hxx>
//#define D_NET
//#define I_NET
#include <dassert.h>
#define MIN_FILL 50
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "VarProbe.hxx"
#include "AsyncList.hxx"
#include "Accessor.hxx"

#define DEBPRINTLEVEL -1
#include <debprint.h>
#define E_CNF
#include "debug.h"

DUECA_NS_START

const int FillPacker::no_of_stores = 2;

const ParameterTable* FillPacker::getParameterTable()
{
  static const ParameterTable table[] = {
    { "buffer-size", new VarProbe<FillPacker,int>
      (REF_MEMBER(&FillPacker::buffer_size)),
      "size of buffer for packing objects into = max size of sent objects"  },
    { NULL, NULL,
      "A FillPacker packs the bulk channel data, and offers these to an\n"
      "IP accessor for transmission whenever there is spare capacity."}
  };
  return table;
}

FillPacker::FillPacker() :
  GenericPacker("fill-packer"),
  store(NULL),
  store_to_fill(0),
  store_to_send(0),
  bytes_to_send(0),
  buffer_size(512)
#ifdef FILLPACKER_SEND_ID
  ,pkg_count(0)
#endif
{
  store = new AmorphStore[2];
}

const char* FillPacker::getTypeName()
{
  return "FillPacker";
}

bool FillPacker::complete()
{
  if (buffer_size < 256) {
    /* DUECA network.

       Configuration error for fill packer, the packer needs a
       reasonable buffer size. */
    E_CNF("Fill packer needs reasonable buffer");
    return false;
  }
  DEB("Initialising fill packer, 2 buffers of size " << buffer_size);

  // give the stores some data
  for (int ii = 2; ii--; ) {
    store[ii].renewBuffer(buffer_size);
    store[ii].reUse();
  }
  return true;
}

FillPacker::~FillPacker()
{
  delete[] store;
}

bool FillPacker::packOneSet(AmorphStore& store,
                            const PackUnit& c)
{
  // 3 bytes, the channel id and this send no
  packData(store, c.entry->getChannelId());

  // 4 more bytes, the start mark, gives length of data
  store.startBigMark();

#ifdef LOG_PACKING
  // get current size
  unsigned before = store.getSize();
#endif

  // unknown no of bytes, data in channel
  c.entry->packData(store, c.idx, c.tick);

  // write the length of the data in the mark
  store.endBigMark();

#ifdef LOG_PACKING
  if (accessor->getLogPacking()) {
    accessor->getPackLog() << "FP " << setw(9) << c.tick
                           << "  i,"
                           << setw(3) << c.entry->getChannelId().getObjectId()
                                 << setw(6) << store.getSize() - before
                           << " s" << setw(4) << c.idx << endl;
  }
#endif

  return true;
}

void FillPacker::packWork()
{
  // remember how much data was in the store
  int old_level = store[store_to_fill].getSize();
  //DEB1("fp store " << store_to_fill << " at " << old_level);

  try {

    while (work_queue.notEmpty() && !store[store_to_fill].isChoked()) {

      // store one of the waiting data sets
      packOneSet(store[store_to_fill], work_queue.front());

      // assure that the pack is complete, progresses channel reading
      work_queue.front().entry->packComplete(work_queue.front().idx);

      // if we get here, there were no exceptions thrown at us. nice.
      old_level = store[store_to_fill].getSize();
      //DEB1("fp store packed one, now at " << old_level);
      work_queue.pop();
    }
  }
  catch(AmorphStoreBoundary& e) {

    // in case something did not fit
    if (old_level == 0) {
      /* DUECA network.

         A bulk object is too large for packing in the fill
         store. Increase the fill store size. */
      W_NET("Object too large for fill store, channel "
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
    DEB2("Object did not fit, resetting to " << old_level);
    store[store_to_fill].setSize(old_level);
  }
}


int FillPacker::stuffMessage(char* buff, int size,
                             MessageBuffer::ptr_type buffer)
{
  packWork();

  // if there is currently nothing left to send, see whether there is
  // a new store ready
  if (!bytes_to_send) {

    if (store[store_to_fill].getSize()) {

      // Yes, start on this store now
      bytes_to_send = store[store_to_fill].getSize();
      index_to_send = 0;
      store_to_send = store_to_fill;
      if (++store_to_fill == no_of_stores) store_to_fill = 0;
      store[store_to_fill].reUse();
    }

    // can now leave the state, filling can continue in parallel with
    // sending of another store
  }

  // early return if nothing to send or too little room available
  if (bytes_to_send == 0 || size < MIN_FILL) {
    return 0;
  }


#ifdef FILLPACKER_SEND_ID

  // send a tag with send node ID and count
  char tag_data[5];
  AmorphStore tag(tag_data, 5);
  tag.packData(getId().getLocationId());
  tag.packData(pkg_count);
  std::memcpy(buff, tag.getToData(), 5);

  // determine how much can be sent
  int send_size = min(size - 5, bytes_to_send);
  std::memcpy(&buff[5], &(store[store_to_send].getToData())[index_to_send],
              send_size);
  DEB("Fill pack node " << int(getId().getLocationId()) <<
      " count " << pkg_count << " to message no " <<
      (buffer? (buffer->message_cycle >> 4): -1));
  pkg_count++;

#else

  // no tag, assume stuff is correctly assembled
  int send_size = min(size, bytes_to_send);
  std::memcpy(buff, &(store[store_to_send].getToData())[index_to_send],
              send_size);
#endif

  DEB1("Fill send, size=" << send_size << " from idx " << index_to_send <<
       " first byte=" <<
       int((store[store_to_send].getToData())[index_to_send]) <<
       " last byte=" <<
       int((store[store_to_send].getToData())[index_to_send+send_size-1]));

  // remember status of buffer here
  index_to_send += send_size;
  bytes_to_send -= send_size;

  // return number of added bytes to send buffer
#ifdef FILLPACKER_SEND_ID
  return send_size + 5;
#else
  return send_size;
#endif
}

DUECA_NS_END
