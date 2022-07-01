/* ------------------------------------------------------------------   */
/*      item            : Packer.cxx
        made by         : Rene' van Paassen
        date            : 990611
        category        : body file
        description     :
        changes         : 990611 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define Packer_cc
#include "Packer.hxx"
#include <iomanip>
#include "Trigger.hxx"
#include "AmorphStore.hxx"
#include "debug.h"
#include <TransportNotification.hxx>
#include "ParameterTable.hxx"
#include "Accessor.hxx"
#include <dueca-conf.h>
#include <dassert.h>
//#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

const ParameterTable* Packer::getParameterTable()
{
  static ParameterTable table[] = {
    {NULL, NULL,
     "A Packer assembles data from channels that have ends on other nodes\n"
     "and offers this data to the IP accessor serving those nodes. Enter it\n"
     "in the appropriate PackerSets to indicate how data is routed, and\n"
     "supply it to an IP Accessor"} };
  return table;
}

Packer::Packer() :
  GenericPacker("packer"),
  store(NULL), current_store(0)
{
  // state is complete after initialiseStores, so the StateGuard is
  // left on
  DEB("Packer constructor " << reinterpret_cast<void*>(this));
}

bool Packer::complete()
{
  return true;
}

const char* Packer::getTypeName()
{
  return "Packer";
}

Packer::~Packer()
{
  //
}

void Packer::
initialiseStores(char** area, int* store_status,
                 int n_stores, int store_size)
{
  store = new AmorphStore[n_stores];
  no_of_stores = n_stores;
  assert(n_stores >= 3);
  for (int ii = n_stores; ii--; ) {
    store[ii].acceptBuffer(area[ii], store_size);
  }
}

void Packer::stopPacking()
{
  // my own work
  delete [] store;
  store = NULL;

  // my parent's work
  GenericPacker::stopPacking();
}

bool Packer::packOneSet(AmorphStore& store, const PackUnit& c)
{
  // 3 bytes, the channel id and this send no
  packData(store, c.entry->getChannelId());

  // 2 more bytes, the start mark, gives length of data
  StoreMark<uint16_t> setsize = store.createMark(uint16_t());

#ifdef LOG_PACKING
  // get current size
  unsigned before = store.getSize();
#endif

  // unknown no of bytes, data in channel
  c.entry->packData(store, c.idx, c.tick);

  // write the length of the data in the mark
  store.finishMark<uint16_t>(setsize);

#ifdef LOG_PACKING
  if (accessor->getLogPacking()) {
    accessor->getPackLog() << "PP " << setw(9) << c.tick
                           << "  i,"
                           << setw(3) << c.entry->getChannelId().getObjectId()
                           << setw(6) << store.getSize() - before
                           << " s" << setw(4) << c.idx << endl;
  }
#endif

  return true;
}

void Packer::packWork()
{
  if (store == NULL || store[current_store].isChoked()) return;
  packWork(store[current_store]);
}

void Packer::packWork(AmorphStore& store)
{
  /* Run through all work in the work_queue, to pack these

     If packing does not succeed due to AmorphStoreBoundary, the store
     is reset to the previous state. Packing actions should NOT modify
     channel state before all data has been packed.

     NoDataAvailable is an old error, should no longer happen.
  */
  int old_state;
  try {
    while (work_queue.notEmpty()) {

      old_state = store.getSize();
      packOneSet(store, work_queue.front());

      // assure that the pack is complete, progresses channel reading
      work_queue.front().entry->packComplete(work_queue.front().idx);
      work_queue.pop();
    }
  }
  catch(const AmorphStoreBoundary& e) {
    // this store is full again, don't destroy the reference to the data
    cerr << "send store full, will try again later" << endl;
    work_queue.front().entry->packFailed(work_queue.front().idx);
    store.setSize(old_state);
  }
  catch(const entryinvalid& e) {
    // entry has disappeared in the meantime
    cerr << "failed packing for now invalid entry" << endl;
    work_queue.pop();
    store.setSize(old_state);
  }
}

int Packer::changeCurrentStore(int& store_no)
{
  if (store == NULL) return 0;

  int fill_level = store[current_store].getSize();
  DEB1("store for transport: " << current_store);
  store_no = current_store;
  if (++current_store == no_of_stores) current_store = 0;
  store[current_store].reUse();

  return fill_level;
}


DUECA_NS_END
