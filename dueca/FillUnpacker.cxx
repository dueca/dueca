/* ------------------------------------------------------------------   */
/*      item            : FillUnpacker.cxx
        made by         : Rene' van Paassen
        date            : 001024
        category        : body file
        description     :
        changes         : 001024 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define FillUnpacker_cc
#include "FillUnpacker.hxx"
#include <iostream>
#include "Unpacker.hxx"
#include "PrioritySpec.hxx"
#include "ObjectManager.hxx"
#include "UnifiedChannel.hxx"
#include "ChannelManager.hxx"
#include "ParameterTable.hxx"
#include "Accessor.hxx"
#include "CriticalActivity.hxx"
#include <iomanip>
#include <boost/lexical_cast.hpp>
//#define D_NET
#define E_NET
#include "debug.h"

#define DO_INSTANTIATE
#include <dueca/AsyncList.hxx>
#include <dueca/VarProbe.hxx>
#include <dueca/MemberCall.hxx>
#include <dueca/DAtomics.hxx>
#include <dueca/Callback.hxx>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

int FillUnpacker::unpacker_no = 0;

const ParameterTable* FillUnpacker::getParameterTable()
{
  static const ParameterTable table[] = {
    { "priority-spec", new MemberCall<FillUnpacker, PrioritySpec>
      (&FillUnpacker::setPrioritySpec),
      "priority at which the unpacker will run" },
    { "buffer-size", new VarProbe<FillUnpacker, unsigned>
      (REF_MEMBER(&FillUnpacker::buffer_size)),
      "size of unpack buffer, must be large enough to handle largest object.\n"
      "Match this size to the size you specify in the FillPackers. Size is\n"
      "in bytes"},
    { NULL, NULL,
      "A FillUnPacker unpacks the bulk channel data."}
  };
  return table;
}

FillUnpacker::FillUnpacker() :
  ScriptCreatable(),
  NamedObject(NameSet
              ("dueca", "fill-un-packer",
               ObjectManager::single()->getLocation()*1000+unpacker_no++)),
  TriggerPuller(std::string("FillUnpacker(") + getPart() + std::string(")")),
  /*Activity(getId(), "unpacking", PrioritySpec(0,0)),
  TriggerPuller(),*/
  //store(NULL),
  //store_status(NULL),
  buffer(NULL),
  amorph_store(),
  work(10, "FillUnpacker::work"),
  buffer_size(512U),
  accessor(NULL),
  cb(this, &FillUnpacker::despatch),
  run_unpack(getId(), "net unpack", &cb, PrioritySpec(0, 0))
{
  // state is completed after initialiseStores, so the stateGuard is
  // left on
}

bool FillUnpacker::complete()
{
  if (buffer_size < 64U) {
    /* DUECA network.

       Supply a large-enough buffer size for the fill unpacker. Note
       that the buffer size must be at least as large as the size for
       the largest fill buffer.
    */
    E_CNF("supply a buffer size > 64 for fill unpacker");
    return false;
  }
  return true;
}

const char* FillUnpacker::getTypeName()
{
  return "FillUnpacker";
}

bool FillUnpacker::setPrioritySpec(const PrioritySpec& ps)
{
  run_unpack.changePriority(ps);
  return true;
}


FillUnpacker::~FillUnpacker()
{
  for (int ii = no_of_senders; ii--; ) delete[] buffer[ii];
  delete[] amorph_store;
  delete[] buffer;
}

ObjectType FillUnpacker::getObjectType() const
{
  return O_Dueca;
}

void FillUnpacker::initialiseStores(int send_order, int no_of_senders)
{
  this->no_of_senders = no_of_senders;

  countcheck.resize(no_of_senders);  countcheck = 0.0;
#ifdef FILLPACKER_SEND_ID
  pkg_count.resize(no_of_senders);   pkg_count = 0.0;
  senderid.resize(no_of_senders);    senderid = -1.0;
#endif

  // reserve room for unpacking, one buffer for each other sender
  buffer = new char*[no_of_senders];
  amorph_store = new AmorphReStore[no_of_senders];
  for (int ii = no_of_senders; ii--; ) {
    if (ii != send_order) {
      buffer[ii] = new char[buffer_size];
      amorph_store[ii].acceptBuffer(buffer[ii], buffer_size);
      amorph_store[ii].reUse(0);
    }
  }

  // add myself as a target for myself. Complicated, but I schedule
  // myself in changeCurrentStore,
  run_unpack.setTrigger(*this);
  run_unpack.switchOn(TimeSpec(0,0));
}

void FillUnpacker::acceptBuffer(MessageBuffer* buffer, const TimeSpec& ts)
{
  buffer->claim();
  work.push_back(buffer);
  pull(ts);
}

void FillUnpacker::despatch(const TimeSpec& t)
{
  // there must by now be a work description for me
  assert(work.notEmpty());

  // buffer to unpack
  MessageBuffer* to_unpack = work.front();

  // number of bytes in this package for the fill unpack
  size_t nbytes = to_unpack->fill -
    to_unpack->regular - to_unpack->offset;
  assert(nbytes);

  // origin of the data, in send order
  int sender = to_unpack->origin;

  unsigned mcycle = (to_unpack->message_cycle);
  if (countcheck[sender] >= mcycle) {
    DEB("FillUnpacker message from " << sender << " cycle jump: expect " <<
        countcheck[sender] << " got " << mcycle);
  }
  countcheck[sender] = mcycle;

#if 0
  // for our troubles, check the crc
  NetCommunicator::ControlBlockReader i_(to_unpack);
  if (!i_.crcgood) {
    DEB("CRC error on buffer from " << sender << " cycle " << mcycle);
  }
#endif

  // check that there is room....
  if (nbytes + amorph_store[sender].getFillLevel() > buffer_size) {
    std::cerr << "Fill unpacker, buffer is too small " << buffer_size
              << " check match with other nodes " << std::endl;
    CriticalActivity::criticalErrorNodeWide();
  }

#ifdef FILLPACKER_SEND_ID
  // verify sender and message count
  AmorphReStore s
    (&to_unpack->buffer[to_unpack->regular+to_unpack->offset], 5);
  uint8_t sendnodeid(s);
  uint32_t count(s);
  DEB("Fill from sender " << sender << " node " << int(sendnodeid) <<
      " count " << count << " in cycle " << to_unpack->message_cycle);
  if (int(sendnodeid) != senderid[sender]) {
    if (senderid[sender] == -1) {
      // initial message
      senderid[sender] = sendnodeid;
    }
    else {
      std::cerr << "Fill packet from sender " << sender << " node "
                << senderid[sender] << " now from different node "
                << int(sendnodeid) << std::endl;
      CriticalActivity::criticalErrorNodeWide();
    }
  }
  if (count != pkg_count[sender]) {
    std::cerr << "Fill packet count from sender " << sender
              << " does not match, expected "
              << pkg_count[sender] << " got " << count << std::endl;
    CriticalActivity::criticalErrorNodeWide();
  }
  pkg_count[sender]++;
  nbytes -= 5;

  // copy the data to the buffer
  std::memcpy(&buffer[sender][amorph_store[sender].getFillLevel()],
              &to_unpack->buffer[to_unpack->regular+to_unpack->offset+5],
              nbytes);
#else
  // copy the data to the buffer
  std::memcpy(&buffer[sender][amorph_store[sender].getFillLevel()],
              &to_unpack->buffer[to_unpack->regular+to_unpack->offset],
              nbytes);
#endif

  DEB("Fill assembling " << nbytes << " sender=" << sender << " offset " <<
      to_unpack->regular+to_unpack->offset <<
      " to buffer at " << amorph_store[sender].getFillLevel());

  // done with buffer, return access
  accessor->returnBuffer(to_unpack);

  // tell the store that it has more data now
  amorph_store[sender].extend(nbytes);

  // done with the work description
  work.pop();

  // continue extracting data from the buffer, fillers use big marks
  // current_idx remembers the state of the restore object. If we
  // can't completely restore an object, we reset the restore buffer
  // to that state
  int current_idx = amorph_store[sender].getIndex();
  GlobalId idx;

  DEB("Store from " << sender << " status " <<
        amorph_store[sender].isExhausted() << " bytes "
        << amorph_store[sender].getSize());
  while (!amorph_store[sender].isExhausted() &&
         amorph_store[sender].getSize() > 3+4+1) {

    // 3 bytes, index of the channel
    unPackData(amorph_store[sender], idx);

    // get a peek at the start mark
    unsigned next_size = amorph_store[sender].peekBigMark();

    // check whether the object (next_size) and the mark (4 bytes)
    // are completely present in the store
    if (next_size + 4 <= amorph_store[sender].getSize()) {

      // yes, we can unpack a complete message
      DEB1("Fill unpacking " << next_size << " index " << current_idx
           << " for channel " << idx.getObjectId());

      // find the associated channel
      UnifiedChannel *c =
        ChannelManager::single()->getChannel(idx.getObjectId());

      // if the channel end is not here, we don't need the data
      if (c == NULL) {
        DEB1("Fill skipping " << next_size
              << " bytes for channel " << idx.getObjectId());

#ifdef LOG_PACKING
        if (accessor->getLogPacking()) {
          accessor->getPackLog()
            << "FX" << idx
            << setw(13) << int(idx.getLocationId())  << ','
            << setw(3) << idx.getObjectId()
            << setw(6) << next_size << endl;
        }
#endif

        // gobble the message, it is not meant to be used here
        amorph_store[sender].gobbleBig();
      }
      else {

        try {
          // gobble the 4 byte mark, it was left here in the peek action
          amorph_store[sender].gobbleBigMark();
          int old_size = amorph_store[sender].getSize();

          // unpack the data
          c->unPackData(amorph_store[sender], idx.getLocationId(), next_size);
          DEB1("unpacked data for channel (fill) " << idx);

#ifdef LOG_PACKING
          if (accessor->getLogPacking()) {
            accessor->getPackLog()
              << "FU"
              << setw(13) << int(idx.getLocationId())  << ','
              << setw(3) << idx.getObjectId()
              << setw(6) << next_size << endl;
          }
#endif

          // the size should match!
          if (next_size != old_size - amorph_store[sender].getSize()) {
            /* DUECA network.

               The declared size of the packed data for a channel does
               not match the number of unpacked bytes by the
               channel. This might indicate a mis-match between DUECA
               versions. */
            E_NET("Fill unpacker, wrong data size channel " << idx
		  << " from " << sender << " bcount " << countcheck[sender]
		  << " expected size " << next_size
		  << " real size " <<
		  old_size - amorph_store[sender].getSize() 
		  << " at index " << current_idx);
            CriticalActivity::criticalErrorNodeWide();
          }
        }
        catch (const AmorphReStoreEmpty& e) {
	  /* DUECA network.
	     
	     Failed to unpack the store by the fill unpacker. Verify that
	     the DUECA versions you use are compatible.
	   */
          E_NET("from sender " << sender <<
                " unpack failure ch " << idx.getObjectId() << " index "
                << current_idx << " decl size " << next_size <<
                 " fill " << amorph_store[sender].getFillLevel());
          throw(e);
        }
        //assert(next_size == old_size - amorph_store[sender].getSize());
      }

      // remember the state of the store, so we won't unpack this
      // again
      current_idx = amorph_store[sender].getIndex();
    }
    else {
      DEB("Not enough data for object of size " << next_size <<
          " from " << sender << " bcount " << countcheck[sender] <<
          " index " << current_idx <<
          " left " << amorph_store[sender].getSize());
      amorph_store[sender].setExhausted();
    }
  }

  // now, the previous loop kept unpacking until there was not enough
  // data left in the store for the next object. If there is no data
  // left at all, we "empty" the old stuff in the store, and start
  // fresh (so a complete filler buffer has been sent, or we have been
  // lucky, and a message ended just with a complete object)

  // on the other hand, if the unpacking left halfway an object, we
  // have got to rewind back to the control information located before
  // the object.

  if (amorph_store[sender].getSize()) {
    DEB("Store from " << sender << ", " << amorph_store[sender].getSize() << " remaining ");
    // incomplete unpack, rewind
    amorph_store[sender].setIndex(current_idx);
  }
  else {
    DEB("Store from " << sender << ", resetting");
    // complete unpack
    amorph_store[sender].reUse(0);
  }
}

ostream& operator << (ostream& os, const FillUnpacker& p)
{
  return os << "FillUnpacker(" <<
    reinterpret_cast<void*>(const_cast<FillUnpacker*>(&p)) << ')';
}

DUECA_NS_END
