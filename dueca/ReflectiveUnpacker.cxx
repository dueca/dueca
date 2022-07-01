/* ------------------------------------------------------------------   */
/*      item            : ReflectiveUnpacker.cxx
        made by         : Rene' van Paassen
        date            : 010711
        category        : body file
        description     :
        changes         : 010711 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ReflectiveUnpacker_cxx


#include <dueca-conf.h>
// compile this, depending on a configure switch
#include "ReflectiveUnpacker.hxx"
#include <ObjectManager.hxx>
#include <StoreInformation.hxx>
#include <SimTime.hxx>
#include <UnifiedChannel.hxx>
#include <ChannelManager.hxx>
#include <CyclicInt.hxx>
#include "CriticalActivity.hxx"
#include "dueca_assert.h"

#define W_SHM
#define E_SHM
#include <debug.h>
#include "ParameterTable.hxx"

#define DO_INSTANTIATE
#include "MemberCall.hxx"
#include "VarProbe.hxx"
#include "AsyncList.hxx"
#include <dueca/Callback.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

/** Macros for calculating check number. */
#define ROTATE_RIGHT(c) \
if ((c) & 01) (c) = ((c) >>1) + 0x80000000; else (c) >>= 1;
#define AMALGI(d, I) \
{ ROTATE_RIGHT(I); I = (I ^ d); }

DUECA_NS_START

int ReflectiveUnpacker::unpacker_no;

const ParameterTable* ReflectiveUnpacker::getParameterTable()
{
  static const ParameterTable table[] = {
    { "buffer-size", new VarProbe<ReflectiveUnpacker,int>
      (REF_MEMBER(&ReflectiveUnpacker::object_buffer_size)),
      "size of buffer for unpacking objects that \"wrap-around\" the\n"
      "communication buffer. Default is the size of the comm buffer itself.\n"
      "size is given in 4-byte words."},
    { "priority-spec", new MemberCall<ReflectiveUnpacker,PrioritySpec>
      (&ReflectiveUnpacker::setPrioritySpec) ,
      "priority at which the unpacker will run"},
    { NULL, NULL,
      "A DUECA helper object that will unpack and distribute the channel\n"
      "data that comes in over a memory interface, currently SCRAMNet or\n"
      "shared memory"}
  };
  return table;
}


ReflectiveUnpacker::ReflectiveUnpacker() :
  NamedObject(NameSet
              ("dueca", "ReflectiveUnpacker",
               ObjectManager::single()->getLocation()*1000+unpacker_no++)),
  //  Activity(getId(), "r unpack", PrioritySpec(0, 0)),
  //  TriggerPuller(),
  work(50,"RUnpacker::work"),
  work_done(30, "RUnpacker::work_done"),
  area_cb(NULL),
  area(NULL),
  last_write_idx(NULL),
  no_parties(0),
  reflect_node_id(0),
  area_size(0),
  object_buffer(NULL),
  object_buffer_size(0),
  cb(this, &ReflectiveUnpacker::despatch),
  run_unpack(getId(), "reflective unpack", &cb, PrioritySpec(0, 0)),
  waker(std::string("ReflectivUnpacker(") + getPart() + std::string(")"))
{
  // all actions in initialisation list
}

bool ReflectiveUnpacker::complete()
{
  if (object_buffer_size > 0) {
    object_buffer = new uint32_t[object_buffer_size];
  }
  return true;
}

const char* ReflectiveUnpacker::getTypeName()
{
  return "ReflectiveUnpacker";
}

bool ReflectiveUnpacker::setPrioritySpec(const PrioritySpec& ps)
{
  run_unpack.changePriority(ps);
  return true;
}

ReflectiveUnpacker::~ReflectiveUnpacker()
{
  delete [] object_buffer;
  delete [] last_write_idx;
}

void ReflectiveUnpacker::initialiseStoresR(StoreInformation& i)
{
  // The store information should be a reflective store information object
  ReflectiveStoreInformation *ri =
    dynamic_cast<ReflectiveStoreInformation*>(&i);

  if (ri == NULL) {
    cerr << "ReflectiveUnpacker received wrong type of info" << endl;
    return;
  }

  // copy the necessary information from the store information object
  area = ri->area;
  area_cb = ri->area_cb;
  area_size = ri->area_size;
  no_parties = ri->no_parties;
  reflect_node_id = ri->node_id;

  // if object buffer not initialized, do that now
  if (object_buffer == NULL) {
    object_buffer_size = ri->area_size;
    object_buffer = new uint32_t[object_buffer_size];
  }

  last_write_idx = new uint32_t[ri->no_parties];
  for (int ii = ri->no_parties; ii--; ) {
    last_write_idx[ii] = 0xffffffff;
  }

  // initialize indexing
  indexes = new Indexes[ri->no_parties];

  // specify that I trigger myself ( my own activity ) and start this
  // activity
  run_unpack.setTrigger(waker);
  run_unpack.switchOn(TimeSpec(0,0));
}

void ReflectiveUnpacker::distribute(int sender)
{
  /* This routine is called from the ReflectiveAccessor's thread.
     it communicates with the unpacking and distributing (normally
     sim-priority), via two AsyncList and by triggering. */

  // figure out whether our work has been done
  while (work_done.size()) {
    indexes[work_done.front().sender].stage2_done = work_done.front().index;
    work_done.pop();
  }

  // determine up to where the buffer has been filled
  int new_write_idx = area_cb[sender][sender];

  // this to remember whether we commanded work at this stage
  uint32_t stage2_commanded0 = 0xffffffff;

  // to remember whether anything was commanded at all
  bool commissioned = false;

  // What was commanded to stage 2, until now
  uint32_t stage2_commanded = indexes[sender].stage2_commanded;

  // sender id
  int sender_id = area_cb[sender][no_parties];

  // try decoding, up to this index
  CyclicInt idx(indexes[sender].stage1, area_size);

  while (++idx != new_write_idx) {

    // get the head
    HeadInfo hi(area[sender], idx, area_size, sender, sender_id);
    DEB("channel head " << hi);

    // The checksum, includes the head
    uint32_t checksum = 0;
    AMALGI(area[sender][idx], checksum);
    for (int ii = (hi.getSize()+3) / 4; ii--; ) {
      if (++idx == new_write_idx) {
        /* DUECA shared memory network.

           Corrupted data while trying to unpack data. */
        W_SHM(getId() << "reflective-unpacker, content mismatch s=" <<
              sender << " idx=" << idx);
        goto faileddata;
      }
      AMALGI(area[sender][idx], checksum);
    }

    // it should be identical to the next word
    if (++idx == new_write_idx) {
        /* DUECA shared memory network.

           Corrupted data while trying to unpack data, checksum failure. */
      W_SHM(getId() << "reflective-unpacker, checksum word missing s=" <<
            sender << " idx=" << idx);
      goto faileddata;
    }
    if (ntohl(area[sender][idx]) != checksum) {

      // Trouble. Checksum does not match, memory writes will be
      // corrected, but we don't wait for it. Report and leave
      /* DUECA shared memory network.

         Corrupted data while trying to unpack data, checksum failure. */
      W_SHM(getId() << "reflective-unpacker, checksum fail, s=" <<
            sender << " idx=" << idx);
      DEB("checksum=" << checksum << " head=" << hi);
      goto faileddata;
    }

    // now, handle the different cases
    if (hi.skipThisMessage()) {
      // no local end, can skip work on this

    }
    else if (hi.isNotification()) {
      // don't need the data, but request the unpack
      work.push_back(hi);
      commissioned = true;

    }
    else {
      // data needs to be unpacked too.
      if (stage2_commanded0 == 0xffffffff) {
        stage2_commanded0 = hi.getIndex();
      }
      stage2_commanded = hi.getIndex();
      work.push_back(hi);
      commissioned = true;
    }

    // remember how far we got in stage 1
    indexes[sender].stage1 = idx;
  }

  // any (hopefully temporary) failure in reading the data from the
  // media (which includes autocorrecting scramnet mode), ends up
  // here. the stage1 (reading media) index is not updated. Unpacking
  // relies on auto-correction of the data, and triggering of reading
  // at a later time
 faileddata:

  // at this point, all work has been commissioned, or we failed due
  // to checksum failure, or there was no work. Now work out whether
  // the read_idx can be updated, so the writing end has more room to
  // write data.
  if (indexes[sender].stage2_done == indexes[sender].stage2_commanded) {

    // the old unpacking jobs were all done. Buffer has been processed
    // up to the point where new jobs were commanded, or we might have
    // simply cleaned all
    if (stage2_commanded0 == 0xffffffff) {
      // no new jobs to read from area, cleaned all
      area_cb[sender][reflect_node_id] = indexes[sender].stage1;
    }
    else {
      // commissioned new jobs, can release until first commissioned job
      area_cb[sender][reflect_node_id] = stage2_commanded0;
      indexes[sender].stage2_commanded = stage2_commanded;
    }

  }
  else {

    // old unpacking was not finished.
    area_cb[sender][reflect_node_id] = indexes[sender].stage2_done;

    // remember up to where
    if (stage2_commanded0 != 0xffffffff) {
      indexes[sender].stage2_commanded = stage2_commanded;
    }
  }

  if (commissioned) {
    DEB(getId() << "triggering further unpack");
    // trigger myself
    waker.requestAlarm(SimTime::getTimeTick());
  }
}

void ReflectiveUnpacker::despatch(const TimeSpec& t)
{
  /* This method runs after triggering by the distribute routine. It
     runs in a priority specified at unpacker creation, not the same
     priority as the reflective accessor and distribute routine. */
  while (work.size()) {

    // get sender id
    int sender_id = area_cb[work.front().getSender()][no_parties];

    if (work.front().isNotification()) {

      // only notification necessary, no need to read the buffer
      DEB("Notification for channel " << work.front().getChannel()->getId());
      // work.front().getChannel()->notifyArrival(work.front().getEntry());
    }
    else {

      // need to access the buffer, since it has the necessary data
      DEB("Unpacking " << work.front().getSize() << " for channel "
            << work.front().getChannel()->getId());

      // can the AmorphReStore object be created directly on top of
      // the communications area?
      if (work.front().getIndex() +
          (work.front().getSize() + 3U)/4U <= area_size) {

        // create AmorphReStore here
        AmorphReStore s
          (reinterpret_cast<const char*>
           (const_cast<uint32_t*>
            (&area[work.front().getSender()][work.front().getIndex()])),
           work.front().getSize());
        work.front().getChannel()->unPackData(s, sender_id,
                                              work.front().getSize());
      }
      else {

        // copy the data
        DEB("End of buffer, at " << work.front().getIndex()
              << " size " << work.front().getSize());
        for (unsigned int ii = (work.front().getSize() + 3)/4; ii--; ) {
          object_buffer[ii] = area[work.front().getSender()]
            [(work.front().getIndex() + ii) % area_size];
        }
        AmorphReStore s(reinterpret_cast<const char*>(object_buffer),
                        work.front().getSize());
        work.front().getChannel()->unPackData(s, sender_id,
                                              work.front().getSize());
      }

      // remember up to where unpacking was done.
      work_done.push_back
        (WorkDone(work.front().getSender(), work.front().getIndex()));
    }

    // this job is done
    work.pop();
  }
}

// helper class that comprises the control information from a message
// in the communication area.
HeadInfo::HeadInfo(volatile uint32_t* area, uint32_t index,
                   unsigned int area_size, int sender_no, int sender_id) :
  // pointer to the channel, initially NULL
  channel_ptr(NULL),
  // index of possible data in the channel
  index((index + 1) % area_size),
  sender(sender_no)
{
  unsigned short w1 = ntohs(area[index] & 0xffff);

  // channel id in 15 bits of w1
  int channel_id = w1 & 0x7fff;

  // flag to see whether data follows, in that case, mangle index
  if (w1 & 0x8000) {

    // message with data following
    size = ntohs(area[index] >> 16);
    entry = 0;
  }
  else {

    // message self contained notification
    entry = ntohs(area[index] >> 16);
    size = 0;
  }

  // check whether channel end available here
  if (ChannelManager::single()->channelHasLocalEnd(channel_id)) {

    // get pointer to end
    channel_ptr = ChannelManager::single()->getChannel(channel_id);
  }
}

HeadInfo::HeadInfo() :
  channel_ptr(NULL),
  index(0),
  size(0),
  entry(0),
  sender(0)
{
  //
}

ostream& HeadInfo::print(ostream& os) const
{
  os << "HeadInfo(channel=" << reinterpret_cast<void*>(channel_ptr)
     << " index=" << index << " size=" << size << " entry=" << entry
     << " sender=" << sender << ")";
  return os;
}

DUECA_NS_END


