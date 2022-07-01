/* ------------------------------------------------------------------   */
/*      item            : ShmAccessor.cxx
        made by         : Rene' van Paassen
        date            : 010710
        category        : body file
        description     :
        changes         : 010710 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ShmAccessor_cxx
#include <dueca-conf.h>

#include "ShmAccessor.hxx"
#include <ReflectivePacker.hxx>
#include <ReflectiveUnpacker.hxx>
#include <ReflectiveFillPacker.hxx>
#include <ReflectiveFillUnpacker.hxx>
#include <ActivityManager.hxx>
#include <Ticker.hxx>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <inttypes.h>
#include <Environment.hxx>
#include "dueca_assert.h"

//#undef TEST_OPTIONS
#ifdef TEST_OPTIONS
static const double prob = 0.01/6.0;
#endif

//#define D_SHM
#define I_SHM
#define W_SHM
#define E_SHM
#include <debug.h>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include <ParameterTable.hxx>
#include <VarProbe.hxx>
#include <MemberCall.hxx>
#include <MemberCall2Way.hxx>
#include <debprint.h>

DUECA_NS_START

const ParameterTable* ShmAccessor::getParameterTable()
{
  static const ParameterTable table[] = {
    { "no-parties", new VarProbe<ShmAccessor,uint32_t>
      (REF_MEMBER(&ShmAccessor::no_parties)),
      "Number of nodes in shared memory set-up" },
    { "my-index", new VarProbe<ShmAccessor,int>
      (REF_MEMBER(&ShmAccessor::my_index)),
      "Index (must be unique), of this node in the shared memory set-up\n"
      "Note that the node with index 0 becomes timing master" },
    { "reflect-area-id", new VarProbe<ShmAccessor,string>
      (REF_MEMBER(&ShmAccessor::reflect_area_id)),
      "String id for the communication area" },
    { "key", new VarProbe<ShmAccessor,int>
      (REF_MEMBER(&ShmAccessor::ikey)),
      "Integer key for shared memory and message queues" },
    { "area-size", new VarProbe<ShmAccessor,uint32_t>
      (REF_MEMBER(&ShmAccessor::area_size)),
      "Size of the total communications area" },
    { "combuff-size", new VarProbe<ShmAccessor,uint32_t>
      (REF_MEMBER(&ShmAccessor::commbuf_size)),
      "Size of each node's sending buffer" },
    { "packer", new MemberCall2Way<ShmAccessor,ScriptCreatable>
      (&ShmAccessor::setPacker),
      "Helper that packs messages into the buffer" },
    { "unpacker", new MemberCall2Way<ShmAccessor,ScriptCreatable>
      (&ShmAccessor::setUnpacker),
      "Helper that unpacks messages from the buffers" },
    { "watchtime", new VarProbe<ShmAccessor,TimeSpec>
      (REF_MEMBER(&ShmAccessor::watchtime)),
      "Time interval for checking the synchronization" },
    { "clocktime", new VarProbe<ShmAccessor,TimeSpec>
      (REF_MEMBER(&ShmAccessor::clocktime)),
      "Time interval for writing synchronization ticks" },
    { "priority", new VarProbe<ShmAccessor,PrioritySpec>
      (REF_MEMBER(&ShmAccessor::prio)),
      "Priority of the communication. Note that this needs to be a\n"
      "priority exclusively for the shared memory communication" },
    { "fill-packer", new MemberCall2Way<ShmAccessor,ScriptCreatable>
      (&ShmAccessor::setFillPacker),
      "Helper that packs bulk messages into the buffer" },
    { "fill-unpacker", new MemberCall2Way<ShmAccessor,ScriptCreatable>
      (&ShmAccessor::setFillUnpacker),
      "Helper that unpacks bulk messages from the buffers" },
    { NULL, NULL, NULL }
  };
  return table;
}

ShmAccessor::ShmAccessor() :
  ReflectiveAccessor(),
  key(-1),
  shm_id(-1),
  msg_id(-1),
  stop_counter(10),
  prio(0, 0),
  my_activity_manager(NULL),
  cb1(this, &ShmAccessor::blockForData),
  cb2(this, &ShmAccessor::stopWork),
  block(getId(), "shared block", &cb1, prio),
  stopwork(getId(), "shared stopping", &cb2, prio),
  timing_check(block, 170000, 270000, 600)
{
  //
}


bool ShmAccessor::complete()
{
  key = ikey;

  if (!ReflectiveAccessor::complete()) {
    return false;
  }

  if (key == -1) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF("ShmAccessor, invalid key");
    return false;
  }
  if (!prio.getPriority()) {
    /* DUECA shared memory network.

       Configuration error. */
    E_CNF("ShmAccessor cannot run at 0 priority");
    return false;
  }

  my_activity_manager = CSE.getActivityManager(prio.getPriority());
  block.changePriority(prio);
  stopwork.changePriority(prio);

  // allocate shared memory. Size is given in four-byte words, so
  // multiply by correct size
  shm_id = shmget(key, area_size * sizeof(uint32_t),
                  (IPC_CREAT | (6 + 8*6 + 64*6)));
    /* DUECA shared memory network.

       Configuration information. */
  I_SHM("Shared mem id=" << shm_id << " for key " << key);

  // failure to get the shared memory is fatal, and since we are early
  // in running we simply stop
  if (shm_id == -1) {
    /* DUECA shared memory network.

       Memory segment access fails. */
    E_SHM("Cannot get shared memory segment with key " << key);
    return false;
  }

  // allocate message queue
  msg_id = msgget(key, IPC_CREAT | (6 + 8*6 + 64*6));
    /* DUECA shared memory network.

       Configuration information. */
  I_SHM("Message mem id=" << msg_id << " for key " << key);

  // failure is just as problematic as aboce
  if (shm_id == -1) {
    /* DUECA shared memory network.

      Message queue access fails. */
     E_SHM("Cannot get message queue with key " << key);
    return false;
  }

  // map the shared memory
  area_start = reinterpret_cast<volatile uint32_t*>
    (shmat(shm_id, reinterpret_cast<void*>
           (const_cast<uint32_t*>(area_start)), 0));
  if (area_start == reinterpret_cast<volatile uint32_t*>(-1)) {
    perror("Cannot attach shared memory segment");
    return false;
  }
    /* DUECA shared memory network.

       Configuration information. */
  I_SHM("Area start " << area_start);

  // set up the ticker to trigger me, slowly!
  // In this way the activity that blocks for data is renewed now and then
  block.setTrigger(*Ticker::single());
  block.setTimeSpec
    (PeriodicTimeSpec(0, int(0.1 / Ticker::single()->getTimeGranule())));
  block.switchOn(TimeSpec(0,0));

  // call the ReflectiveAccessor's code to initialise the areas and
  // packer/unpacker
  initialiseArea(area_start);

  return true;
}


ShmAccessor::~ShmAccessor()
{
  DEB1("Destructor ShmAccessor");

  // stop the message queue
  if (my_index == 0) {
    struct msqid_ds msgds;
    if (msgctl(msg_id, IPC_RMID, &msgds) == -1) {
      perror("Cannnot remove queue");
    }
  }

  // detach the shared memory area
  if (shmdt(const_cast<uint32_t*>(area_start)) == -1) {
    perror("detaching shared memory");
  }

  // try to remove the shared memory area
  if (my_index == 0) {
    struct shmid_ds shmds;
    if (shmctl(shm_id, IPC_RMID, &shmds) == -1) {
      perror("Cannnot remove shared memory area");
    }
  }
}

extern "C" {
  /** This is the message buffer as used here. The only message that
      is sent is the offset of an "active" write into the shared
      memory area. */
  struct msgbuf_offset
  {
    long mtype;
    uint32_t offset;
  };
}

void ShmAccessor::blockForData(const TimeSpec& ts)
{
  // messages come here
  struct msgbuf_offset message;

  // if not running multithread, do not wait, because this will block
  // all other activities
  int msgflag = 0;
  int nowait_tries = 5;
  if (!CSE.runningMultiThread()) {
    msgflag = IPC_NOWAIT;
    startContact();
  }

  while (!block.noScheduledBehind()) {

    // block on messages
    DEB1("Blocking on message queue");

    my_activity_manager->logBlockingWait();
    if (msgrcv(msg_id, &message, sizeof(uint32_t),
               my_index + 1, msgflag) == -1) {

      // tell that something has come in, and I am rolling again
      my_activity_manager->logBlockingWaitOver();

      // an error occurred. The only "normal" errors are EINTR and
      // EIDRM
      if (errno == EIDRM) {
        // stop myself
        perror("Stopping shared mem activity");
        block.switchOff(TimeSpec(0,0));
        return;
      } else if (errno == ENOMSG) {

        // log via timingcheck
        timing_check.userReportsAnomaly();

        if (--nowait_tries) {
          DEB1("No message, not running multi, sleeping");
          usleep(10000);
        }
        else {
          /*          // do check control writes for start-up phase
          if (!CSE.runningMultiThread() && my_index == 0) {
            // check all others
            for(int ii = no_parties; --ii; ) {
              handleControlWrite(ii, ts);
            }
            }*/

          DEB1("No message, not running multi, return");
          return;
        }
      }
      else if (errno != EINTR) {
        // print the error
        perror("while receiving message:");
        return;
      }
    }
    else {

      // tell that the data has come in, and I am rolling again
      my_activity_manager->logBlockingWaitOver();

      DEB1("Have message, type=" << message.mtype
            << ", offset=" << message.offset);

      // now we should have a message. It contains an offset into
      // the shared memory area where we are triggered. The
      // ReflectiveAccessor has code to deal with this
      handleControlWrite(message.offset, ts);
    }
  }

  DEB1("Leaving ShmAccessor::blockForData");
}

void ShmAccessor::stopWork(const TimeSpec& ts)
{
  if (stop_counter-- > 0) {

    // write 0 to the control/timing area
    write(&(area_start[my_index]), 0);

    // check whether other nodes are also writing zeros
    bool others_stopping = true;
    for (int ii = no_parties; ii--; )
      others_stopping &= area[ii] == 0;

    // stop scramnet when others also stopped, or after countdown complete
    if (others_stopping || !stop_counter) {

      // make sure we dont do any work any more
      stop_counter = 0;
      stopwork.switchOff(TimeSpec(0, 0));

      return;
    }

    // siphon off any messages over the control channels
    struct msgbuf_offset message;
    while (msgrcv(msg_id, &message, sizeof(uint32_t),
               my_index + 1, IPC_NOWAIT) != -1) {
      DEB1("Have message, type=" << message.mtype
            << ", offset=" << message.offset);
      handleControlWrite(message.offset, ts);
    }
    if (errno != ENOMSG && errno != EINTR) {
      perror("while checking for message:");
    }
  }
}

void ShmAccessor::write(volatile uint32_t* address, uint32_t value)
{
#ifdef TEST_OPTIONS
  int try_error = int(random()/prob/(RAND_MAX+1.0));
  if (try_error == my_index) {
    DEB(getId() << "pretending failed interrupt write");
    return;
  }
#endif

  int offset = address - area_start;
  *address = value;

#ifdef TEST_OPTIONS
  try_error = int(random()/prob/(RAND_MAX+1.0));
  if (try_error == my_index) {
    offset = offset + int(random()/prob/(RAND_MAX+1.0))+1;
    DEB(getId() << " pretending interrupt location error");
  }
#endif

  DEB1("ShmAccessor explicit write, offset=" << offset
        << " value=" << value);

  // this is SysV messaging. Have to send a message to all the others,
  // with the message id equal to send_no + 1
  for (int ii = no_parties; ii--; ) {

    if (ii != my_index) {


      // make a message
      struct msgbuf_offset message = { ii + 1, uint32_t(offset) };
      DEB1("sending message type=" << message.mtype
            << ", data=" << message.offset);

      // send it
      if (msgsnd(msg_id, &message, sizeof(uint32_t), 0) == -1) {
        perror("In message send");
      }
    }
  }
}

void ShmAccessor::prepareToStop()
{
  // switch off the blocking mode
  block.switchOff(TimeSpec(0,0));

  // turn off clock writing (in ReflectiveAccessor)
  if (my_index == 0) {
    clockwrite.switchOff(TimeSpec(0,0));
  }

  // go over to another activity, stopwork
  stopwork.setTrigger(*Ticker::single());
  stopwork.switchOn(TimeSpec(0,0));
}

DUECA_NS_END
