/* ------------------------------------------------------------------   */
/*      item            : ReflectiveFillUnpacker.cxx
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


#define ReflectiveFillUnpacker_cxx
#include "ReflectiveFillUnpacker.hxx"
#include <NameSet.hxx>
#include <ObjectManager.hxx>
#include <ChannelManager.hxx>
#include <CriticalActivity.hxx>
#include <DataReader.hxx>
#include <UnifiedChannel.hxx>
#include <StoreInformation.hxx>
#include <CallbackWithId.hxx>
#include <dassert.h>
//#define I_SHM
//#define D_SHM
#define E_SHM
#define W_SHM
#include <debug.h>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <CallbackWithId.hxx>
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "VarProbe.hxx"
#include <debprint.h>
DUECA_NS_START

int ReflectiveFillUnpacker::unique = 0;

const ParameterTable* ReflectiveFillUnpacker::getParameterTable()
{
  static const ParameterTable table[] = {
    { "buffer-size", new VarProbe<ReflectiveFillUnpacker, int>
      (REF_MEMBER(&ReflectiveFillUnpacker::buffer_size)),
      "size of unpack buffer, must be large enough to handle largest object.\n"
      "Size is given in bytes" },
    { "priority-spec", new VarProbe<ReflectiveFillUnpacker,PrioritySpec>
      (REF_MEMBER(&ReflectiveFillUnpacker::unpack_prio)) ,
      "priority at which the unpacker will run"},
    { NULL, NULL,
      "Unpacks bulk data sent over reflective/shared memory accessors." }
  };
  return table;
}


ReflectiveFillUnpacker::ReflectiveFillUnpacker() :
  NamedObject(NameSet("dueca", "ReflectiveFillUnpacker", ++unique +
                      ObjectManager::single()->getLocation() * 1000)),
  buffer(NULL),
  buffer_size(512),
  store(NULL),
  channel_manager(ChannelManager::single()),
  token_valid(this, &ReflectiveFillUnpacker::tokenValid),
  unpack_prio(0, -50),
  all_receive_stuff()
{

}

bool ReflectiveFillUnpacker::complete()
{
  if (buffer_size < 64) {
    /* DUECA shared memory network.

       Configuration error ReflectiveFillUnpacker. */
    E_CNF("supply a buffer size > 64 for fill unpacker");
    return false;
  }
  return true;
}

const char* ReflectiveFillUnpacker::getTypeName()
{
  return "ReflectiveFillUnpacker";
}

void ReflectiveFillUnpacker::tokenValid(const TimeSpec& ts)
{
  for (list<ChannelReadToken*>::iterator ii =
         all_tokens.begin(); ii != all_tokens.end(); ii++) {
    (*ii)->isValid();
  }
}

ReflectiveFillUnpacker::~ReflectiveFillUnpacker()
{
  delete[] buffer;
}

void ReflectiveFillUnpacker::
initialise(const ReflectiveStoreInformation& i)
{
  // first make the amorphous storage objects and the buffers. For
  // programming efficiency, I waste one of the AmorphReStores and a
  // buffer pointer.
  buffer = new char*[i.no_parties];
  store = new AmorphReStore[i.no_parties];

  for (int ii = i.no_parties; ii--; ) {
    if (ii != i.node_id) {

      // fill these, i.e. allocate the buffers and associate each buffer
      // with an AmorphReStore object.
      buffer[ii] = new char[buffer_size];
      store[ii].acceptBuffer(buffer[ii], buffer_size);
      // tell the store that there is no data in the buffer
      store[ii].reUse(0);

      // open a channel
      ChannelReadToken *token =
        new ChannelReadToken
        (getId(), NameSet("dueca", "FillSet", ii),
         getclassname<FillSet>(), 0,
	 Channel::Events, Channel::OnlyOneEntry,
	 Channel::AdaptEventStream, 0.0, &token_valid);

      // make a callback object, which takes a pointer to the channel
      CallbackWithId<ReflectiveFillUnpacker,SenderInfo>
        *cb = new
        CallbackWithId<ReflectiveFillUnpacker,SenderInfo>
        (this, &ReflectiveFillUnpacker::receiveAPiece, SenderInfo(ii,token));

      // and an activity that uses this callback
      ActivityCallback* receive_stuff = new
        ActivityCallback(getId(), "receive fill", cb, unpack_prio);

      // specify the trigger for the activity, and switch it on
      receive_stuff->setTrigger(*token);
      receive_stuff->switchOn(TimeSpec(0, 0));

      // remember, for if I want to clean up things later
      all_receive_stuff.push_back(receive_stuff);
    }
    else {
      buffer[ii] = NULL;
    }
  }
}




/** This is a utility function to read out the first word of a
    transfer command in the shared memory. This word may contain an
    update notification for a stream channel (which has its data in
    shared memory, only the act of changing data needs to be sent) or
    it may be the start of a block of data for an event channel, and
    in this case the length of data (in bytes) if given. */
static inline void decodeHead(uint16_t head, unsigned int& channel_id,
                              bool& outflowing)
{
  DEB1("Decoding head " << hex << head << dec);

  channel_id = head & 0x3fff;              // bits 0 - 13 for channel id
  outflowing = (head & 0x8000) == 0;       // bit 15 indicates inflowing
}

void ReflectiveFillUnpacker::
receiveAPiece(const TimeSpec& ts,
              SenderInfo& info)
{
  DataReader<FillSet> i(*info.second, ts);

  int storeno = info.first;

  if (i.data().data.size() > store[storeno].getFree()) {
    /* DUECA shared memory network.

       No room for copying data. */
    W_SHM(getId() << "Cannot copy FillSet to store " << storeno
          << "\nfillset size " << i.data().data.size() <<
          " free in buffer " << store[storeno].getFree());

    // flag this as a critical error
    CriticalActivity::criticalErrorNodeWide();

    // stop myself from accepting more data -- and
    // corrupting/generating lots of error
    //all_receive_stuff.switchOff(TimeSpec(0,0));

    return;
  }

  DEB1(getId() << " received " << i.data().data.size());

  // copy the data into the unpacking store
  std::memcpy(&buffer[storeno][store[storeno].getFillLevel()],
              i.data().data.ptr(), i.data().data.size());
  store[storeno].extend(i.data().data.size());

  // unpack as much as possible from the store
  while(!store[storeno].isExhausted() && store[storeno].getSize() > 6) {

    // remember the fill level of this store, because it might be that
    // the object is still incomplete, and we have to reset the state
    // of this store
    int old_level = store[storeno].getIndex();
    assert(old_level >= 0);

    // Let's see. Channel data has a 16 bit head, containing the
    // channel id, and the data direction, and a 32 bit size.
    uint16_t head; unPackData(store[storeno], head);
    unsigned size = store[storeno].peekBigMark();

    // only go on if the information in the store is complete
    // the 4 bytes are for the big mark, which we only peeked at, but
    // did not remove from the store.
    if (size + 4 <= store[storeno].getSize()) {

      // yes, we can unpack or skip the meassage
      unsigned int channel_id; bool outflowing;
      decodeHead(head, channel_id, outflowing);

      // check that there is a local end
      if (channel_manager->channelHasLocalEnd(channel_id)) {

        // get a pointer to the local channel end
        UnifiedChannel *c = channel_manager->getChannel(channel_id);

        // outflowing data is accepted by everyone but the master,
        // inflowing data is accepted by only the master (and
        // subsequently send out again)
          DEB1("RFill unpacking " << size
                << " bytes for channel " << channel_id);

          // gobble the mark
          store[storeno].gobbleBigMark();
          int old_size = store[storeno].getSize();
          c->unPackData(store[storeno], i.origin().getLocationId(), size);

          // check the consumption of bytes
          if (size != old_size - store[storeno].getSize()) {
            /* DUECA shared memory network.

               Data unpack used wrong data size */
            W_SHM("Fill unpacker, wrong data size channel " << channel_id
                  << " expected size " << size
                  << " real size " << old_size -
                  store[storeno].getSize());
            CriticalActivity::criticalErrorNodeWide();
          }
      }
      else {

        DEB1("RFill skipping " << size << " bytes for unknown channel "
              << channel_id);
        // no local end, cannot use this, gobble the data
        store[storeno].gobbleBig();
      }
    }
    else {

      // if we get here, we found an object that is still
      // incomplete. reset the reading pointer in the store, and set
      // the store to exhausted
      DEB1("data incomplete, resetting " << storeno
            << " to level " << old_level);
      store[storeno].setIndex(old_level);
      store[storeno].setExhausted();
    }
  }

  // if the store has been completely emptied, then start again from zero
  if (store[storeno].getSize() == 0) {
    store[storeno].reUse(0);
    DEB1("completely cleaned no " << storeno);
  }
}

ostream& operator << (ostream& os, const ReflectiveFillUnpacker& o)
{
  return os << "ReflectiveFillUnpacker(" << o.getId() << ')';
}


DUECA_NS_END
