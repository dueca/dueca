/* ------------------------------------------------------------------   */
/*      item            : Unpacker.cxx
        made by         : Rene' van Paassen
        date            : 990615
        category        : body file
        description     :
        changes         : 990615 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Unpacker_cc

#include <boost/lexical_cast.hpp>
#include <dueca-conf.h>
#include "Unpacker.hxx"
#include "ChannelManager.hxx"
#include "ObjectManager.hxx"
#include "UnifiedChannel.hxx"
#include "Accessor.hxx"
#include <iomanip>
//#include "dstypes.h"
#include "AmorphStore.hxx"
#include "TimeSpec.hxx"
#define E_NET
#include "debug.h"
#include "dueca_assert.h"
#include "ParameterTable.hxx"

#define DO_INSTANTIATE
#include "MemberCall.hxx"
#include "AsyncList.hxx"
#include "Callback.hxx"
#include <debprint.h>

DUECA_NS_START

int Unpacker::unpacker_no = 0;

const ParameterTable* Unpacker::getParameterTable()
{
  static const ParameterTable table[] = {
    { "priority-spec", new MemberCall<Unpacker,PrioritySpec>
      (&Unpacker::setPrioritySpec) ,
      "priority at which the unpacker will run"},
    { NULL, NULL,
      "Unpacks data sent over IP accessors."}
  };
  return table;
}

Unpacker::Unpacker() :
  ScriptCreatable(),
  NamedObject(NameSet
              ("dueca", "Unpacker",
               ObjectManager::single()->getLocation()*1000+unpacker_no++)),
  TriggerPuller(std::string("Unpacker(") + getPart() + std::string(")")),
  /* Activity(getId(), "unpacking", PrioritySpec(0, 0)), */
  store(NULL), store_status(NULL),
  no_of_stores(0),
  work(20, "Unpacker::work"),
  cb(this, &Unpacker::despatch),
  run_unpack(getId(), "net unpack", &cb, PrioritySpec(0, 0))
{
  //
}

bool Unpacker::complete()
{
  return true;
}

const char* Unpacker::getTypeName()
{
  return "Unpacker";
}

bool Unpacker::setPrioritySpec(const PrioritySpec& ps)
{
  run_unpack.changePriority(ps);
  return true;
}


Unpacker::~Unpacker()
{
  //
}

ObjectType Unpacker::getObjectType() const
{
  return O_Dueca;
}

void Unpacker::
initialiseStores()
{
  run_unpack.setTrigger(*this);
  run_unpack.switchOn(TimeSpec(0,0));
}

void Unpacker::acceptBuffer(MessageBuffer* buffer, const TimeSpec& ts)
{
  buffer->claim();
  work.push_back(buffer);
  this->pull(ts);
}

void Unpacker::despatch(const TimeSpec& t)
{
  DEB1("unpacker works" << t);

  // there must by now be a work description for me
  assert(work.notEmpty());

  // buffer to unpack
  MessageBuffer* to_unpack = work.front();

  // create a restore object with the buffer
  AmorphReStore store(&(to_unpack->buffer[to_unpack->offset]),
                      to_unpack->regular);

  // a bunch of working variables
  uint16_t len;
  UnifiedChannel* c;
  GlobalId idx;

  // start running over the store
  while (store.getSize() && !store.isExhausted()) {

    // index of the channel
    unPackData(store, idx);

    // see if there is a local end here
    c = ChannelManager::single()->getChannel(idx.getObjectId());

    if (c == NULL) {
#ifdef LOG_PACKING
      if (accessor->getLogPacking()) {
        accessor->getPackLog()
          << "PX"
          << setw(13) << int(idx.getLocationId())  << ','
          << setw(3) << idx.getObjectId()
          << setw(6) << store.peekMark() << endl;
      }
#endif
      store.gobble();
      DEB1("Skipping data for channel " << idx);
    }
    else {

      // data size, read away
      unPackData(store, len);
      int storesize = store.getSize();

      // unpack the data
      c->unPackData(store, idx.getLocationId(), len);
      DEB1("unpacked data for channel " << idx);
#ifdef LOG_PACKING
      if (accessor->getLogPacking()) {
        accessor->getPackLog()
          << "PU"
          << setw(13) << int(idx.getLocationId())  << ','
          << setw(3) << idx.getObjectId()
          << setw(6) << len << endl;
      }
#endif

      // check the size. Very important, because a lot of things can go wrong
      if (storesize - store.getSize() != len) {
        /* DUECA network.

           Serious error; unpacking of data for a channel used the
           wrong data size from the network buffer. 
        */
        E_NET("Wrong size data for channel " << idx);
      }
      assert(storesize - store.getSize() == len);
    }
  }

  // done with this buffer. The following atomically reduces use count and
  // returns the buffer when everyone is done with it
  accessor->returnBuffer(to_unpack);

  // done with the work description
  work.pop();
}

DUECA_NS_END
