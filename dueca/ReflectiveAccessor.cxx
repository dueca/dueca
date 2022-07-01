/* ------------------------------------------------------------------   */
/*      item            : ReflectiveAccessor.cxx
        made by         : Rene' van Paassen
        date            : 010420
        category        : body file
        description     :
        changes         : 010420 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ReflectiveAccessor_cxx

#include <dueca-conf.h>

// compile this, depending on a configure switch
#include "ReflectiveAccessor.hxx"
#include <StoreInformation.hxx>
#include <ReflectivePacker.hxx>
#include <ReflectiveUnpacker.hxx>
#include <ReflectiveFillPacker.hxx>
#include <ReflectiveFillUnpacker.hxx>
#include "CriticalActivity.hxx"
#include <Ticker.hxx>
#include <NameSet.hxx>
#include <ChannelManager.hxx>
#include <Environment.hxx>
#include <NodeManager.hxx>
#include <iomanip>

//#define D_SHM
#define I_SHM
#define E_SHM
#define W_SHM
#include <debug.h>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <debprint.h>

DUECA_NS_START

int ReflectiveAccessor::sequence = 0;

ReflectiveAccessor::
ReflectiveAccessor() :
  Accessor(NameSet("dueca", "ReflectiveAccessor", ++sequence)),
  cstate(Unknown),
  reflect_area_id(""),
  packer(NULL),
  unpacker(NULL),
  fill_packer(NULL),
  fill_unpacker(NULL),
  my_index(0),
  no_parties(0),
  commbuf_size(1024),
  area_start(NULL),
  area_cb(NULL),
  area_size(0),
  countdown(0),
  countdown_init(10),
  counts_in_second(int(1.0/Ticker::single()->getTimeGranule() + 0.5)),
  helpers_initialised(false),
  helpers_started(false),
  have_to_start_clock(false),
  timing_ok(true),
  direct_comm_allowed(true),
  write_index_copy(NULL),
  cb1(this, &ReflectiveAccessor::watchTheLine),
  cb2(this, &ReflectiveAccessor::writeClock),
  watchdog(getId(), "reflective watch", &cb1, PrioritySpec(0,0)),
  clockwrite(getId(), "clock write", &cb2,
             PrioritySpec(CSE.getHighestPrio(), 0)),
  line_ok(false),
  data_received(false)
{
  // nothing
}

bool ReflectiveAccessor::complete()
{
  bool res = true;

  // has to be set by calls from the creation script
  if (!reflect_area_id.size()) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' << "ReflectiveAccessor, no area id");
    res = false;
  }
  if (!packer) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' << "ReflectiveAccessor, no packer");
    res = false;
  }
  if (!unpacker) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' << "ReflectiveAccessor, no unpacker");
    res = false;
  }
  if (!fill_packer) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' << "ReflectiveAccessor, no fill packer");
    res = false;
  }
  if (!fill_unpacker) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' << "ReflectiveAccessor, no fill unpacker");
    res = false;
  }
  if (!no_parties) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' << "ReflectiveAccessor, no peers");
    res = false;
  }
  if (!commbuf_size) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' << " no communication buffer");
    res = false;
  }

  // check that we have correct sizes.
  if (no_parties * (commbuf_size + no_parties + 2) >= area_size) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF("Needed shared memory size" <<
          no_parties * (commbuf_size + no_parties + 2) <<
          " available " << area_size);
    res = false;
  }
  if (!watchtime.getValiditySpan()) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' << " invalid watch time");
    res = false;
  }
  if (!my_index && !clocktime.getValiditySpan()) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' <<
          " invalid tick writing time");
    res = false;
  }

  // bail out here, if errors found
  if (!res) return false;

  // some derived parameters
  cstate = my_index ? Unknown : Contact1;
  have_to_start_clock = my_index != 0;

  // configure and switch on the watchdog activity. This activity will
  // check the configuration of the lines
  watchdog.setTrigger(*Ticker::single());
  watchdog.setTimeSpec(watchtime);
  watchdog.switchOn(TimeSpec(0, 0));


  // set up the ticker to trigger me also for the clock writing
  // activity. It runs in the ticker thread, right after the ticker.
  clockwrite.setTrigger(*Ticker::single());
  clockwrite.setTimeSpec(clocktime);

  // copies of the write index, backup when control writes fail
  write_index_copy = new uint32_t[no_parties];
  for (int ii = no_parties; ii--; ) write_index_copy[ii] = 0xffffffff;

  // clockwrite will be switched on when the communication has been
  // initialised.
  return true;
}

void ReflectiveAccessor::initialiseArea(volatile uint32_t* area_start)
{
  // remember the area starrt
  this->area_start = area_start;

  // calculate the array with control areas
  area_cb = new volatile uint32_t*[no_parties];
  area = new volatile uint32_t*[no_parties];
  for (int ii = no_parties; ii--; ) {
    area_cb[ii] = &area_start[no_parties +
                              ii*(commbuf_size + no_parties + 1)];
    area[ii] = &area_start[2*no_parties + 1 +
                           ii*(commbuf_size + no_parties + 1)];
  }

  // set my node id into the proper location. THis is later used to
  // get sender information
  //area_cb[my_index][no_parties] = NodeManager::single()->getThisNodeNo();

  // Tell the channel thingy that reflective areas can be given out.
  // For now, the nodemask with reachable nodes is all nodes!
  size_t startfree =
    no_parties + no_parties*(commbuf_size + no_parties + 1);
  if (direct_comm_allowed && startfree < area_size) {
    /* DUECA shared memory network.

       Information on shared region use. */
    I_SHM("Shared region " << reflect_area_id <<
          " starting at " << &area_start[startfree] <<
          " size " << area_size - startfree);

    /*    ChannelManager::single()->offerSharedRegion
      (reflect_area_id.c_str(),
       const_cast<uint32_t*>(&area_start[startfree]),
       area_size - startfree, DESTINATION_MASK);
    */
  }
}

void ReflectiveAccessor::startContact()
{
  // and now node 0 starts the first write, to initialise
  // communication
  if (my_index == 0 && cstate == Contact1 && !countdown) {
    cstate = Contact1;
    write(area_start, cstate);
    if (countdown) {
      countdown--;
    }
    else {
      countdown = countdown_init;
    }
  }
}

ReflectiveAccessor::~ReflectiveAccessor()
{
  delete[] area_cb;
}


void ReflectiveAccessor::watchTheLine(const TimeSpec &ts)
{
  //DEB1("ReflectiveAccessor::watchTheLine, data_received=" << data_received);
  line_ok = data_received;
  data_received = false;

  // check, if running real-time, for a reasonable response from all
  // nodes
  if (CSE.runningMultiThread()) {

    // maximum and minimum counts from me
    unsigned int count_max = area_start[0];
    unsigned int count_min = count_max;

    // check the max and min from others
    for (int ii = no_parties; --ii; ) {
      if (area_start[ii] > count_max) count_max = area_start[ii];
      if (area_start[ii] < count_min) count_min = area_start[ii];
    }
    DEB1(getId() << " min " << count_min << " max " << count_max);

    // things are unacceptable, if "far" in real-time running, and
    // count differs too much.
    if (count_max > 120 * counts_in_second &&
        count_max - count_min > (counts_in_second + 3) / 3) {
      if (CriticalActivity::criticalErrorNodeWide()) {
        /* DUECA shared memory network.

           Discovered a large time difference between nodes in shared
           memory access; probably due to one of the DUECA processes
           failing or communication failure. Setting nodewide error,
           and stopping this node. */
        E_TIM(getId() << " detected time disparity " << count_min
              << " to " << count_max);
        for (unsigned int ii = 0; ii < no_parties; ii++)
          cerr << ii << ':' << area_start[ii] << ' ';
        cerr << endl;
      }
    }
  }
}

void ReflectiveAccessor::writeClock(const TimeSpec &ts)
{
  // write the current spec
  DEB1("Writing tick " << ts.getValidityStart());
  if (my_index) {
    // others just write
    DEB1("clock write, offset=" << my_index << " value=" <<
          ts.getValidityStart());
    area_start[my_index] = ts.getValidityStart();
  }
  else {
    // no 0 triggering write
    write(&area_start[0], ts.getValidityStart());
  }
}

void ReflectiveAccessor::initialiseHelpers()
{
  // As soon as it is clear that we have communication working,
  // initialise the stores to be empty.
  ReflectiveStoreInformation
    ri(this, area_cb, area, commbuf_size, no_parties, my_index);

  // initialise packer stores. This sets the control memory to good
  // values.
  packer->initialiseStoresR(ri);
}

void ReflectiveAccessor::startHelpers()
{
  // packers and unpackers have a single interface to initialisation
  // data about the stores. This interface accepts a StoreInformation
  // object
  ReflectiveStoreInformation
    ri(this, area_cb, area, commbuf_size, no_parties, my_index);

  /* DUECA shared memory network.

     Information on the layout of the communication memory. */
  I_SHM("Reflective information: " << ri);

  // notify the packers and unpackers of these locations
  // packer->initialiseStores(ri);
  packer->start(this);
  unpacker->initialiseStoresR(ri);

  if (fill_packer != NULL) fill_packer->initialise(ri);
  if (fill_unpacker != NULL) fill_unpacker->initialise(ri);
}

void ReflectiveAccessor::handleControlWrite(uint32_t offset,
                                            const TimeSpec& ts)
{
  // for checking
  data_received = true;

  if (cstate == Operational) {

    DEB1("Operation message, offset=" << dec << offset <<
         " value there=" << area_start[offset]);

    // do all the normal stuff
    if (offset == 0 && my_index != 0) {

      // clock triggering. Disregarding myself, have to inform my
      // ticker
      DEB1("Tick from master " << area_start[0]);
      uint32_t tickval = area_start[0];
      if (tickval && timing_ok)
        Ticker::single()->tickFromMaster(tickval);

      if (have_to_start_clock) {
        clockwrite.switchOn(ts);
        have_to_start_clock = false;
      }
    }
    else if (offset >= uint32_t(no_parties)) {

      // this landed somewhere in the communication areas, find out
      // which one
      int sending_node = (offset - no_parties) /
        (no_parties + commbuf_size + 2);

      // it should be exact, so with n nodes, there are n area_cb's,
      // and node 0 writes at location 0, i.e. right after the n
      // syncing words, and node 1 writes at location n +
      // (commbuf_size + no_parties + 1 + 1), etc. So I could
      // just as well insert an assertion:
      if (sending_node * (no_parties + commbuf_size + 2) +
          no_parties != offset || sending_node >= int(no_parties)) {

        // is there a correction possible?
        bool corrected = false;
        for (uint32_t trynode = no_parties; trynode--; ) {
          uint32_t tryoff = no_parties +
            trynode*(no_parties + commbuf_size + 2);
          if (area_start[tryoff] != write_index_copy[trynode] &&
              write_index_copy[trynode] != 0xffffffff) {
            /* DUECA shared memory network.

               Detected an irregular message, trying to interpret as a write.
            */
            W_SHM("Guessing write from " << trynode <<
                  " offset received " <<
                  hex << offset << " calc " << tryoff << dec);
            offset = tryoff;
            sending_node = trynode;
            corrected = true;
            break;
          }
        }
        if (!corrected) {
          /* DUECA shared memory network.

             Detected a control location write that cannot be
             correct. Probably due to a (transient?) hardware
             error. */
          W_SHM("Erroneous location control write: " << hex << offset << dec);
          return;
        }
      }

      // remember index for later corrections?
      write_index_copy[sending_node] = area_start[offset];

      // so, there is more data. Despatch the unpacker to disseminate
      // it in this node
      DEB1("Data buffer control write from " << sending_node);

      unpacker->distribute(sending_node);

    }
    else {
          /* DUECA shared memory network.

             Ignoring an operational message at a location where none
             should occur. */
      I_SHM("Ignored operational message at " << offset << " data " <<
            hex << area_start[offset] << dec);
    }
  }
  else {

    // first check that this comm is about the control area
    if (offset >= no_parties) {
      /* DUECA shared memory network.

         Ignoring an operational message that does not conform to the
         DUECA protocol. Is another program accessing shared or
         reflective memory? */
      I_SHM("Rogue communication at offset " << offset
            << " on the shared mem channel " << reflect_area_id);
      return;
    }

    if (my_index != 0) {

      // check for commands from node 0
      if (offset == 0) {
        /* DUECA shared memory network.

           Node 0 issues a starting value. */
        I_SHM("Start write from 0, value=" << ComState(area_start[0]));
        switch(area_start[0]) {
        case uint32_t(Contact1):
          // confirm contact
          write(&area_start[my_index], Contact1);
          cstate = Contact1;

          // and write my node id in the appropriate spot
          area_cb[my_index][no_parties] =
            NodeManager::single()->getThisNodeNo();
          break;

        case uint32_t(Contact2):
          // confirm contact 2 by my
          write(&area_start[my_index], Contact2);
          cstate = Contact2;

          // initialise the packer and area.
          if (!helpers_initialised) {
            initialiseHelpers();
            helpers_initialised = true;
          }

          // and write my node id in the appropriate spot
          area_cb[my_index][no_parties] =
            NodeManager::single()->getThisNodeNo();
          break;

        case uint32_t(Contact3):
          if (cstate == Contact2 || cstate == Contact3) {
            // confirm contact 3 by me
            write(&area_start[my_index], Contact3);

            // and we are operational
            cstate = Operational;

            // now start packers and unpackers
            if (!helpers_started) {
              startHelpers();

              helpers_started = true;
            }
          }
          else {
            /* DUECA shared memory network.

               Incorrect start sequence detected. Communication error
               or is another program accessing the memory?
             */
            E_SHM("Missed previous state changes on area "
                  << reflect_area_id << "\ncannot confirm");
          }
          break;
        case uint32_t(Unknown):
          // do nothing, this is OK
          break;
        default:
            /* DUECA shared memory network.

               Unforeseen error in the start-up state machine.
             */
          E_SHM("Contact state machine error, got "
                << area_start[0]);
        }
      }
    }
    else {

      // as node 0, check the control writes from others
      static ComState next_com_state[5] =
      { Operational, Contact2, Contact3, Operational, Contact1 };

      // check the state of the  other nodes
      bool all_same = true;
      for (int ii = no_parties; --ii; ) {
        all_same &= (cstate == ComState(area_start[ii]));
      }

      if (all_same) {
        cstate = next_com_state[int(cstate)];
        /* DUECA shared memory network.

           Received a correct message, advancing to next state.
        */
        I_SHM("advancing to com state " << cstate);
        write(&area_start[0], cstate);

        if (cstate == Contact2) {
          // and write my node id in the appropriate spot
          area_cb[my_index][no_parties] =
            NodeManager::single()->getThisNodeNo();
        }
      }
      else {
        /* DUECA shared memory network.

           Confirmation not complete, not advancing.
         */
        I_SHM("confirm not complete, not advancing com state");
#ifdef I_SHM_ACTIVE
        for (unsigned int ii = 0; ii < no_parties; ii++ )
          cerr << ii << ':' << area_start[ii] << ' ';
        cerr << endl;
#endif
        // not all the same, may mean the others are already writing clocks
        if (cstate == Contact3) {
            bool oper_test = true;
          for (int ii = no_parties; --ii; ) {
            oper_test = oper_test &&
              (ComState(area_start[ii]) == Operational ||
               area_start[ii] > 5);
          }
          if (oper_test) cstate = Operational;
        }
      }

      if (cstate == Contact3 && !helpers_initialised) {
        // this initialises uses of the comm area
        initialiseHelpers();
        helpers_initialised = true;
      }

      if (cstate == Operational) {

        // tell packer/unpacker where to put/find the data
        startHelpers();

        // start writing clock pulses into the start-up area
        clockwrite.switchOn(ts);
      }
    }
  }
}

bool ReflectiveAccessor::setPacker(ScriptCreatable& p, bool in)
{
  // only valid for specifying stuff, not for getting it out again
  if (!in) return false;
  if (packer) {
    /* DUECA shared memory network.

       Configuration error.
     */
    E_SHM(getId() << '/' <<
          "ReflectiveAccessor, already have a packer");
    return false;
  }

  ReflectivePacker *pnew = dynamic_cast<ReflectivePacker*> (&p);
  if (pnew == NULL) {
    /* DUECA shared memory network.

       Configuration error.
     */
    E_CNF(getId() << '/' <<
          "ReflectiveAccessor, object is not a ReflectivePacker");
    return false;
  }

  // protect from Scheme garbage collection, and add to the sets.
  //scheme_id.addReferred(&p);
  packer = pnew;

  // signal success
  return true;
}

bool ReflectiveAccessor::setUnpacker(ScriptCreatable& p, bool in)
{
  // only valid for specifying stuff, not for getting it out again
  if (!in) return false;
  if (unpacker) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' <<
          "ReflectiveAccessor, already have an unpacker");
    return false;
  }

  ReflectiveUnpacker *pnew = dynamic_cast<ReflectiveUnpacker*> (&p);
  if (pnew == NULL) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' <<
          "ReflectiveAccessor, object is not a ReflectiveUnpacker");
    return false;
  }

  // protect from Scheme garbage collection, and add to the sets.
  //scheme_id.addReferred(&p);
  unpacker = pnew;

  // signal success
  return true;
}

bool ReflectiveAccessor::setFillPacker(ScriptCreatable& p, bool in)
{
  // only valid for specifying stuff, not for getting it out again
  if (!in) return false;
  if (fill_packer) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' <<
          "ReflectiveAccessor, already have a fill packer");
    return false;
  }

  ReflectiveFillPacker *pnew = dynamic_cast<ReflectiveFillPacker*> (&p);
  if (pnew == NULL) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' <<
          "ReflectiveAccessor, object is not a ReflectiveFillPacker");
    return false;
  }

  // protect from Scheme garbage collection, and remember
  //scheme_id.addReferred(&p);
  fill_packer = pnew;

  // signal success
  return true;
}

bool ReflectiveAccessor::setFillUnpacker(ScriptCreatable& p, bool in)
{
  // only valid for specifying stuff, not for getting it out again
  if (!in) return false;
  if (fill_unpacker) {
    /* DUECA shared memory network.

       Configuration error. */
    E_SHM(getId() << '/' <<
          "ReflectiveAccessor, already have a fill unpacker");
    return false;
  }

  ReflectiveFillUnpacker *pnew = dynamic_cast<ReflectiveFillUnpacker*> (&p);
  if (pnew == NULL) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF(getId() << '/' <<
          "ReflectiveAccessor, object is not a ReflectiveFillUnpacker");
    return false;
  }

  // protect from Scheme garbage collection, and add to the sets.
  //scheme_id.addReferred(&p);
  fill_unpacker = pnew;

  // signal success
  return true;
}


ostream& operator << (ostream& os, const ReflectiveAccessor& r)
{
  return os << "ReflectiveAccessor(reflect_area_id="
            << r.reflect_area_id << ')';
}

ostream& operator << (ostream& os,
                      const ReflectiveAccessor::ComState& o)
{
  static const char* names[] =
  { "Operational",
    "Contact1",
    "Contact2",
    "Contact3",
    "Unknown",
    "out of bounds="};
  if (int(o) >= 0 && int(o) <= 4)
    return os << names[int(o)];
  else
    return os << names[5] << int(o);
}


DUECA_NS_END


