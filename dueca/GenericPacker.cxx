/* ------------------------------------------------------------------   */
/*      item            : GenericPacker.cxx
        made by         : Rene' van Paassen
        date            : 980611
        category        : header file
        description     : The classes that send data from one host to
                          one or more others
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "GenericPacker.hxx"
#include <ObjectManager.hxx>
#include <Accessor.hxx>
#include "CriticalActivity.hxx"
#define E_CNF
#include <debug.h>
DUECA_NS_START

int GenericPacker::unique = 0;

GenericPacker::GenericPacker(const char* tname) :
  NamedObject(NameSet("dueca", tname, ++unique +
                      ObjectManager::single()->getLocation() * 1000)),
  accessor(NULL)
{
  //
}

GenericPacker::~GenericPacker()
{
  // nothing for now
}

static const char* message =
"Attempt to use an auxiliary packer main packer!";

int GenericPacker::changeCurrentStore(int & store)
{
  if (CriticalActivity::criticalErrorNodeWide()) {
    /* DUECA network.

       You are attempting to use this packer type as the main packer,
       while it is an auxiliary (fill) packer. Check and correct the
       configuration of your network communication. */
    E_CNF(message);
  }
  return 0;
}

void GenericPacker::initialiseStores(char** area, int* store_status,
                                     int n_stores, int store_size)
{
  changeCurrentStore(n_stores);
}

const char* GenericPacker::getTypeName()
{
  return "GenericPacker";
}

void GenericPacker::stopPacking()
{
  if (accessor != NULL) {
    accessor->prepareToStop();
    accessor = NULL;
  }
}

void GenericPacker::notification(UChannelEntry* entry,
                                 TimeTickType ts, unsigned idx)
{
  {
    AsyncQueueWriter<PackUnit> w(work_queue);
    w.data().entry = entry;
    w.data().idx = idx;
    w.data().tick = ts;
  }
}

void GenericPacker::packWork()
{
  assert(0);
}

DUECA_NS_END
