/* ------------------------------------------------------------------   */
/*      item            : ReflectivePacker.cxx
        made by         : Rene' van Paassen
        date            : 010404
        category        : body file
        description     :
        changes         : 010404 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


// build test option
#define REVISION 3

#define ReflectivePacker_cxx
#include "ReflectivePacker.hxx"
#include <dueca-conf.h>

//#define D_SHM
#define I_SHM
#define E_SHM
#include <debug.h>
#include <iomanip>

#ifdef TEST_OPTIONS
static const double prob = 0.01/6.0;
#endif

// compile this, depending on a configure switch
#include <AmorphStore.hxx>
#include <StoreInformation.hxx>
#include <TimeSpec.hxx>
#include <TransportNotification.hxx>
#include <UnifiedChannel.hxx>
#include <ReflectiveAccessor.hxx>
#include <dassert.h>
#include "dueca_assert.h"
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "VarProbe.hxx"
#include "MemberCall.hxx"
#include "NodeManager.hxx"
#include "AsyncList.hxx"

#define DEBPRINTLEVEL -1
#include <debprint.h>

/** Macros for calculating check number. */
#define ROTATE_RIGHT(c) \
if ((c) & 01) (c) = ((c) >>1) + 0x80000000; else (c) >>= 1;
#define AMALGI(d, I) \
{ ROTATE_RIGHT(I); I = (I ^ d); }

DUECA_NS_START

const ParameterTable* ReflectivePacker::getParameterTable()
{
  static const ParameterTable table[] = {
    { "buffer-size", new VarProbe<ReflectivePacker,int>
      (REF_MEMBER(&ReflectivePacker::object_buffer_size)),
      "size of buffer for packing objects into = max size of sent objects.\n"
      "size is in 4-byte words" },
    { "priority-spec", new MemberCall<ReflectivePacker,PrioritySpec>
      (&ReflectivePacker::setPrioritySpec),
      "priority at which the packing process will run" },
    { NULL, NULL,
      "The reflective-packer prepares channel data for sending over SCRAMNet\n"
      "or other devices with reflective/shared memory capability. Create\n"
      "one or more reflective packers and add these to a packer set to\n"
      "define which nodes communicate in what way, and supply the packers\n"
      "to their corresponding media accessor" }
  };
  return table;
}

ReflectivePacker::ReflectivePacker() :
  GenericPacker("reflective-packer"),
  TriggerPuller(),
  accessor(NULL),
  area_cb(NULL),
  area(NULL),
  area_write_idx(1),
  area_size(0),
  no_reflect_nodes(0),
  reflect_node_id(-1),
  object_buffer(NULL),
  object_buffer_size(1024),
  store(),
  cb1(this, &ReflectivePacker::packWorkR),
  id("dueca", "reflective-packer", static_node_id),
  pack_activity(id.getId(), "pack data to reflective mem",
                &cb1, PrioritySpec(0,0)),
  scheduled(true)   // this is a lie!
{
  pack_activity.setTrigger(*this);
}

bool ReflectivePacker::complete()
{
  assert(object_buffer_size > 0);
  object_buffer = new uint32_t[object_buffer_size];
  return true;
}

const char* ReflectivePacker::getTypeName()
{
  return "ReflectivePacker";
}

ReflectivePacker::~ReflectivePacker()
{
  delete[] (object_buffer);
}

bool ReflectivePacker::setPrioritySpec(const PrioritySpec& ps)
{
  pack_activity.changePriority(ps);
  return true;
}

void ReflectivePacker::initialiseStoresR(StoreInformation& i)
{
  /* DUECA shared memory network.

     Information on reflective packer initialisation. */
  I_SHM("Reflective packer initialisation");

  // The store information should be a reflective store information object
  ReflectiveStoreInformation *ri =
    dynamic_cast<ReflectiveStoreInformation*>(&i);

  if (ri == NULL) {
    cerr << "ReflectivePacker received wrong type of info" << endl;
    return;
  }

  // the stream and event areas are allocated by the reflective accessor,
  // and passed in the area parameter
  // accessor = ri->accessor;  // has to stay NULL, until start
  area = ri->area[ri->node_id];
  area_cb = ri->area_cb[ri->node_id];
  area_size = ri->area_size;
  for (int ii = area_size; ii--; ) area[ii] = 0;

  // now we have to find out who we are, and how many nodes are using the
  // shared memory communication
  no_reflect_nodes = ri->no_parties;
  reflect_node_id  = ri->node_id;

  // initialise the control buffers. I write all "my" control buffer
  // words to "0", meaning read index 0, write index "0"
  for (int ii = no_reflect_nodes; ii--; ) {
    area_cb[ii] = 0;
  }
  // but my write index should be one, so I have the sentinel word in
  area_cb[reflect_node_id] = 1;

  // write indexes need new length
  //area_write_idx.setLimit(area_size);
  area_write_idx = 1;

  // Initialise the store
  store.acceptBuffer(reinterpret_cast<char*>(object_buffer),
                     object_buffer_size);
  store.reUse(area_size - 1);
}

void ReflectivePacker::start(ReflectiveAccessor* acc)
{
  /* DUECA shared memory network.

     Information on start of reflective packer packing. */
  I_SHM(getId() << " will start packing");

  // set the accessor
  accessor = acc;
  pack_activity.switchOn(0);

  this->pull(0);
}

void ReflectivePacker::stopPacking()
{
  if (accessor != NULL) {

    // tell my accessor
    accessor->prepareToStop();

    // and tell myself
    accessor = NULL;
  }
}

int ReflectivePacker::roundUpFree(volatile uint32_t *area, int size)
{
  /** The trick is to find the fullest read buffer. We start out with
      the emptiest possible read buffer, of size 1, with only the
      sentinel word in it. This is with the read index 1 lower than
      the write index, and if it drops below 0 it is the last word in
      the buffer. */
  int write_idx = area_write_idx; //area_cb[reflect_node_id];
  int fullest = write_idx - 1;
  if (fullest < 0) fullest += size;

  /** Now we look for fuller read buffers. Fuller is in three cases:
      <ol>
      <li> Above the write index, fullest is also above the write
      index but further than the current index
      <li> Above the write index, fullest is "still" below the write
      index
      <li> Below the write index, fullest is also below the write
      index, but higher than the current index.
      </ol> */
  for (int ii = no_reflect_nodes; ii--; ) {
    if (ii != reflect_node_id) {
      int idx = area_cb[ii];
      if ((idx > write_idx && (fullest > idx || fullest < write_idx)) ||
          (idx < write_idx && fullest > idx && fullest < write_idx)) {
        fullest = idx;
      }
    }
  }
  DEB("round up, write_idx=" << area_write_idx << " fullest=" << fullest);

  return fullest;
}

void ReflectivePacker::copyBuffer(int nwords)
{
  DEB("Copying buffer of " << nwords << " words, 1st=" <<
        hex << object_buffer[0] << dec);

#ifdef TEST_OPTIONS
#warning Building in test failures in writing
  static volatile uint32_t *failed_address = NULL;
  static uint32_t failed_data;

  if (failed_address) {
    // correct previous simulated failure
    *failed_address = failed_data;
    failed_address = NULL;
  }

  int try_error = int(random()/prob/(RAND_MAX+1.0));
#endif

  // may be called without data ...
  if (!nwords) {
    accessor->write(&area_cb[reflect_node_id], area_write_idx);
    return;
  }

  CyclicInt idx(area_write_idx, area_size);
  for (int ii = 0; ii < nwords; ii++) {

#ifdef TEST_OPTIONS
    // spoil the data writing once in a while
    if (try_error == idx) {
      failed_data = object_buffer[ii];
      failed_address = &area[idx];
      object_buffer[ii] = area[idx];
    }
#endif

    // write and calculate checksum
    area[idx] = object_buffer[ii];
    ++idx;
  }

  // update the control area data
  accessor->write(&area_cb[reflect_node_id], idx);
  area_write_idx = idx;
}

void ReflectivePacker::notification(UChannelEntry* entry,
                                    TimeTickType ts, unsigned idx)
{
  // first queue the notification
  GenericPacker::notification(entry, ts, idx);

  // schedule the activity, preferably once?
  if (!scheduled) {
    this->pull(ts);
    scheduled = true;
  }
}

void ReflectivePacker::packWorkR(const TimeSpec& ts)
{
  // flag that we are working
  scheduled = false;

  // calculate number of free words in buffer
  int free_size = 0;

  if (accessor == NULL) {
    return;
  }
  else {

    // Find out how much space there is
    int area_free_idx = roundUpFree(area_cb, area_size);
    free_size = (area_size - area_write_idx + area_free_idx) % area_size;
    DEB("Shared mem free_size=" << free_size);
  }

  // re-use the store
  store.reUse(min(object_buffer_size, free_size)*sizeof(uint32_t));

  // check whether that is enough to do pack something
  if (!store.checkForRoom(80)) {
    /* DUECA shared memory network.

       There is no room in the shared memory buffers, packing is
       delayed. */
    I_SHM("No room shared mem, not packing");
    copyBuffer(store.getSize() / sizeof(uint32_t));
    return;
  }

  // remember current index, for if unpacking fails.
  int store_idx = store.getSize();

  // work off the stack
  try {
    while (work_queue.notEmpty()) {

      unsigned int checksum_idx = store.getSize() / 4;
      work_queue.front().entry->codeHead(store);
      store.startMark();

      work_queue.front().entry->packData
        (store, work_queue.front().idx, work_queue.front().tick);
      store.endMark();

      unsigned int upto_idx = store.roundSize4() / 4;

      uint32_t checksum = 0;
      for (unsigned int ii = checksum_idx; ii < upto_idx; ii++) {
        AMALGI(object_buffer[ii], checksum);
      }
      DEB("0Checksum " << checksum << " at index " << store_idx);
      packData(store, checksum);

      // all packing done, remember new place in store
      store_idx = store.getSize();

      // mark this progress in the channel
      work_queue.front().entry->packComplete(work_queue.front().idx);

      work_queue.pop();
    }
  }
  catch(AmorphStoreBoundary& e) {

    /* DUECA shared memory network.

       While packing data, the shared memory buffer filled up, packing
       is reset and resumed later. */
    I_SHM("Full store with channel "
          << work_queue.front().entry->getChannelName());

    // reset the channel status
    work_queue.front().entry->packFailed(work_queue.front().idx);

    // clogged at this point. Reset the store index to what it was
    // just before, and stop further packing
    store.setSize(store_idx);
  }
  catch(NoDataAvailable& e) {

    /* DUECA shared memory network.

       The data that should be transmitted is no longer available in
       the local channel end. This condition should no longer happen
       with modern DUECA versions.
     */
    I_SHM("Data lost for pack, channel " <<
          work_queue.front().entry->getChannelName());

    store.setSize(store_idx);
    work_queue.pop();
  }

  // now copy the store into the shared memory. This also adjusts
  // the counter and writes into the control area
  copyBuffer(store.getSize() / sizeof(uint32_t));

  // and leave
  return;
}

int ReflectivePacker::changeCurrentStore(int& store_no)
{
  // this is useless for this kind of packer
  return 0;
}

DUECA_NS_END

