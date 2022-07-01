/* ------------------------------------------------------------------   */
/*      item            : ScramNetAccessor.cxx
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


#define ScramNetAccessor_cxx

#include <dueca-conf.h>

#define USE_WAIT_IOCTL
#define SIMONA
#define SCRAMNET_TIMEOUT 16

// compile this, depending on a configure switch and the presence of
// the scramnet headers

#include "ScramNetAccessor.hxx"
#include <ReflectivePacker.hxx>
#include <ReflectiveUnpacker.hxx>
#include <ReflectiveFillPacker.hxx>
#include <ReflectiveFillUnpacker.hxx>
#include <ActivityManager.hxx>
#include <Ticker.hxx>
#include <Environment.hxx>
#include <cstdio>

extern "C" {
  // the following define is needed to prevent the symbol
  // Scramnet_config from being defined twice
# define CONFIG_EXTERN extern
#include <scramnet/scrplus.h>
  void scr_acr_write(unsigned long, unsigned char);
  extern int scr_csr_fd;
  extern int scr_registers_mapped;
  int scr_int_set( int flag )
  {
    if( ioctl( scr_csr_fd, INTSIG, flag ) == -1 )
      return -1;
     return 0;
  }
}
# include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <Environment.hxx>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>

//#define D_SHM
//#define I_SHM
#define W_SHM
#define E_SHM
#include <debug.h>
#include <iomanip>

#define DO_INSTANTIATE
#include <Callback.hxx>
#include "dueca_assert.h"
#include <ParameterTable.hxx>
#include <VarProbe.hxx>
#include <MemberCall.hxx>
#include <MemberCall2Way.hxx>

DUECA_NS_START

#define SCRAMSIG SIGUSR1

const ParameterTable* ScramNetAccessor::getParameterTable()
{
  static const ParameterTable table[] = {
    { "no-parties", new VarProbe<ScramNetAccessor,uint32_t>
      (REF_MEMBER(&ScramNetAccessor::no_parties)),
      "Number of nodes in shared memory set-up" },
    { "my-index", new VarProbe<ScramNetAccessor,int>
      (REF_MEMBER(&ScramNetAccessor::my_index)),
      "Index (must be unique), of this node in the shared memory set-up\n"
      "Note that the node with index 0 becomes timing master" },
    { "reflect-area-id", new VarProbe<ScramNetAccessor,string>
      (REF_MEMBER(&ScramNetAccessor::reflect_area_id)),
      "String id for the communication area" },
    { "key", new VarProbe<ScramNetAccessor,int>
      (REF_MEMBER(&ScramNetAccessor::ikey)),
      "Integer key for shared memory and message queues" },
    { "area-size", new VarProbe<ScramNetAccessor,uint32_t>
      (REF_MEMBER(&ScramNetAccessor::area_size)),
      "Size of the total communications area" },
    { "combuff-size", new VarProbe<ScramNetAccessor,uint32_t>
      (REF_MEMBER(&ScramNetAccessor::commbuf_size)),
      "Size of each node's sending buffer" },
    { "packer", new MemberCall2Way<ScramNetAccessor,ScriptCreatable>
      (&ScramNetAccessor::setPacker),
      "Helper that packs messages into the buffer" },
    { "unpacker", new MemberCall2Way<ScramNetAccessor,ScriptCreatable>
      (&ScramNetAccessor::setUnpacker),
      "Helper that unpacks messages from the buffers" },
    { "watchtime", new VarProbe<ScramNetAccessor,TimeSpec>
      (REF_MEMBER(&ScramNetAccessor::watchtime)),
      "Time interval for checking the synchronization" },
    { "clocktime", new VarProbe<ScramNetAccessor,TimeSpec>
      (REF_MEMBER(&ScramNetAccessor::clocktime)),
      "Time interval for writing synchronization ticks" },
    { "priority", new VarProbe<ScramNetAccessor,PrioritySpec>
      (REF_MEMBER(&ScramNetAccessor::prio)),
      "Priority of the communication. Note that this needs to be a\n"
      "priority exclusively for the shared memory communication" },
    { "fill-packer", new MemberCall2Way<ScramNetAccessor,ScriptCreatable>
      (&ScramNetAccessor::setFillPacker),
      "Helper that packs bulk messages into the buffer" },
    { "fill-unpacker", new MemberCall2Way<ScramNetAccessor,ScriptCreatable>
      (&ScramNetAccessor::setFillUnpacker),
      "Helper that unpacks bulk messages from the buffers" },
    { "direct-comm", new VarProbe<ScramNetAccessor,bool>
      (REF_MEMBER(&ScramNetAccessor::direct_comm_allowed)),
      "Give out remainder of scramnet memory for direct communication of\n"
      "stream channel contents. May produce problems when struct packing is\n"
      "not identical in all nodes. Default = true." },
    { "scramnet-mode", new VarProbe<ScramNetAccessor,uint32_t>
      (REF_MEMBER(&ScramNetAccessor::csr2_mode)),
      "Mode for scramnet. Under linux, this is written directly to CSR2\n"
      "0x0604: platinum plus mode, 0x4040: platinum mode, 0xC040 burst mode\n"
      "Under QNX: 0=burst, 1=platinum, 2=plus" },
    { NULL, NULL, NULL }
  };
  return table;
}

ScramNetAccessor::ScramNetAccessor() :
  ReflectiveAccessor(),
  int_read_guard("scramnet int"),
  waiting_interrupt(0xffffffff),
#ifdef HAVE_SCRAMNET_H
  key(0xff),
#else
  key(-1),
#endif
  have_to_attach(true),
  not_transferred(true),
  scr_int_fd(-1),
  worked(true),
  stop_counter(100),
  prio(0, 0),
  csr2_mode(0x4040),
  stopping(false),
  my_activity_manager(NULL),
  cb1(this, &ScramNetAccessor::checkData),
  cb2(this, &ScramNetAccessor::blockForData),
  cb3(this, &ScramNetAccessor::checkAndTickle),
  cb4(this, &ScramNetAccessor::stopWork),
  check(getId(), "scram check", &cb1, prio),
  block(getId(), "scram block", &cb2, prio),
  tickle(getId(), "scram watch", &cb3,
         PrioritySpec(CSE.getHighestPrio(), 0)),
  stopwork(getId(), "scram stop", &cb4, prio),
  // block works on a 0.1 second base. Do the timing check for one
  // minute, and check on 0.17 and 0.27 s late
  timing_check(block, 170000, 270000, 600)
{
  //
}

static const char* classname = "ScramNetAccessor";

bool ScramNetAccessor::complete()
{
  key = ikey;
  if (!ReflectiveAccessor::complete()) {
    return false;
  }
  bool res = true;

  if (key < 0 || key > 255) {
    /* DUECA scramnet access.

       An error. The ScramNetAccessor is obsolete and no longer in
       use. Error messages have not been further elaborated. */
    E_SHM(getId() << '/' << classname << " invalid key");
    res = false;
  }
  if (!prio.getPriority()) {
    E_SHM(getId() << '/' << classname <<
          " cannot run at 0 priority");
    res = false;
  }
  my_activity_manager = CSE.getActivityManager(prio.getPriority());
  check.changePriority(prio);
  block.changePriority(prio);
  stopwork.changePriority(prio);

  // get out if parameters not OK
  if (!res) return false;

  // initialise scramnet. This also defines the scr_int_fd, the file
  // descriptor for getting interrupts from scramnet
  int initres;
  if ((initres = sp_scram_init(0)) < 0) {
    E_SHM(getId() << " error sp_scram_init, " << initres);
    return false;
  }
  if (!scr_registers_mapped) {
    E_SHM(getId() << " scram regs not mapped");
    return false;
  }

  // check memory size
  if (sp_mem_size() < commbuf_size*4) {
    E_SHM(getId() << " this scramnet card does not have " <<
          commbuf_size*4 << " bytes");
    return false;
  }

  // reset the board
  scr_reset_mm();

  // obtain the scramnet base memory pointer
  area_start = get_base_mem();

  // clear acr memory
  scr_mclr_mm(ACR);

  // set read/write sensitivity on the comm areas, first the initial
  // area
  if (my_index == 0) {

    // no 0 wants to send on area_start[0]
    scr_acr_write(0, ACR_TIE);
  }
  else {

    // others want to receive from 0
    scr_acr_write(0, ACR_RIE);
  }

  // now the control areas of the communication buffers. Here you want
  // to receive from the sender.
  for (int ii = no_parties; ii--; ) {
    if (my_index == ii) {
      scr_acr_write(no_parties + ii*(no_parties+commbuf_size + 2), ACR_TIE);
      I_SHM(getId() << " setting transmit on " <<
            no_parties + ii*(no_parties+commbuf_size + 2));
    }
    else {
      scr_acr_write(no_parties + ii*(no_parties+commbuf_size + 2), ACR_RIE);
      I_SHM(getId() << " setting receive on " <<
            no_parties + ii*(no_parties+commbuf_size + 2));
    }
  }

  // set timeout to the approx no of nodes in SRS
  // \todo Make this variable
  sp_net_to(SCRAMNET_TIMEOUT);

  // insert the scramnet node in the ring.
  // enable transmit, receive, the receive and transmit interrupts
  // according to ACR bits, and insert the node in the ring.
  scr_csr_write( SCR_CSR0, 0xF000); // fifo reset
  scr_csr_write( SCR_CSR0, 0x8003); // normal insert, fifo complete
  scr_csr_write( SCR_CSR2, csr2_mode /* 0x6040 */); // platinum+ mode

  unsigned short estat = scr_csr_read( SCR_CSR1 );
  if (estat != 0) I_SHM("Error bits step 1 " << hex << estat << dec);
  estat = scr_csr_read( SCR_CSR1 );
  if (estat != 0) I_SHM("Error bits step 2 " << hex << estat << dec);
  if (estat & 1<<6) {
    E_SHM("CDC failure, check fiber wiring\n");
    return false;
  }

  I_SHM("Node inserted CSR0=" << hex << scr_csr_read(SCR_CSR0) << dec);

#ifndef USE_WAIT_IOCTL
  // block sigusr1 in this and all other threads
  sigset_t mask; sigaddset(&mask, SCRAMSIG);
  sigprocmask(SIG_BLOCK, &mask, NULL);
#endif

  // clear the scramnet memory (at least the first 256 words)
  if (my_index == 0) {
    for (int ii = 256; ii--; ) area_start[ii] = 0;
  }

  // set up the ticker to trigger me, slowly!
  // In this way the activity that blocks for data is renewed now and then
  check.setTrigger(*Ticker::single());
  block.setTimeSpec
    (PeriodicTimeSpec(0, int(0.1 / Ticker::single()->getTimeGranule())));
  check.setTimeSpec
    (PeriodicTimeSpec(0, int(0.1 / Ticker::single()->getTimeGranule())));
  check.switchOn(TimeSpec(0,0));

#ifndef USE_WAIT_IOCTL
  tickle.setTimeSpec
    (PeriodicTimeSpec(0, int(max(0.02, Ticker::single()->getDT()+0.001) /
                             Ticker::single()->getTimeGranule()) )) ;
  tickle.setTrigger(*Ticker::single());
#endif

  // call the ReflectiveAccessor's code to initialise the areas and
  // packer/unpacker
  initialiseArea(area_start);

  int_read_guard.leaveState();
  return true;
}

ScramNetAccessor::~ScramNetAccessor()
{
  W_SHM("Destructor ScramNetAccessor");

  // switch off the scramnet board (assumes only one process -- me)
  scr_csr_write(SCR_CSR0, 0);
  scr_csr_write(SCR_CSR1, 0);

  // clear the interrupt capability and close the interrupt fd
  scr_int_set(CLR_SIG);

  // clear the acr ram
  scr_mclr_mm(ACR);
  scr_mclr_mm(MEM);

  // unmap the scramnet memory from this process
  scr_mem_mm(UNMAP);

  // unmap the control registers
  scr_reg_mm(UNMAP);
}

void ScramNetAccessor::checkData(const TimeSpec& ts)
{
  if (not_transferred) {

    // start comm sequence
    startContact();

    if (my_index > 0) {
      // check 0 writing
      handleControlWrite(0, ts);
    }
    else if (cstate != Operational) {
      // check all others
      for(int ii = no_parties; --ii; ) {
        handleControlWrite(ii, ts);
      }
    }

    // check data comm
    if (cstate == Operational) {
      for(int ii = no_parties; ii--; ) {
        if (ii != my_index) {
          handleControlWrite(no_parties + ii*
                             (2 + no_parties + commbuf_size), ts);
        }
      }
    }

    // check for a transition to real-time running
    if (CSE.runningMultiThread()) {

      W_SHM(getId() << "transfer to multithread, " << ts);

      // stop this activity
      check.switchOff(ts);
      //check.removeTrigger();

      // and start the other
      block.setTrigger(*Ticker::single());
      block.switchOn(ts);

      not_transferred = false;
    }
  }
  else {
    I_SHM(getId() << " already transferred at " << ts);
  }
}

bool ScramNetAccessor::readInterrupt(uint32_t& location)
{
#ifndef USE_WAIT_IOCTL
  int_read_guard.accessState();

  // if there is a previously read interrupt, return it
  if (waiting_interrupt != 0xffffffff) {
    location = waiting_interrupt;
    waiting_interrupt = 0xffffffff;

    int_read_guard.leaveState();
    return true;
  }

  // if not, try to read from csr 5 and 4
  // read the csr 5 register, and check whether interrupt FIFO has values
  uint16_t csr5 = scr_csr_read(SCR_CSR5);
  if (csr5 & CSR5_IFE) {

    // also read csr4, and store in waiting interrupt variable
    uint16_t csr4 = scr_csr_read(SCR_CSR4);
    location = uint32_t(csr5 & ~(CSR5_RB | CSR5_IFE)) << 16 | csr4;
    int_read_guard.leaveState();
    return true;
  }

  int_read_guard.leaveState();
  return false;
#else
  return scr_read_int_fifo(&location) ? true: false;
#endif
}

bool ScramNetAccessor::checkInterrupt()
{
  int_read_guard.accessState();

  if (waiting_interrupt != 0xffffffff) {

    // still an interrupt read in and to be processed. True therefore
    int_read_guard.leaveState();
    return true;
  }

  // read the csr 5 register, and check whether interrupt FIFO has values
  uint16_t csr5 = scr_csr_read(SCR_CSR5);
  if (csr5 & CSR5_IFE) {

    // also read csr4, and store in waiting interrupt variable
    uint16_t csr4 = scr_csr_read(SCR_CSR4);
    waiting_interrupt = uint32_t(csr5 & ~(CSR5_RB | CSR5_IFE)) << 16 | csr4;

    int_read_guard.leaveState();
    return true;
  }

  int_read_guard.leaveState();
  return false;
}


void ScramNetAccessor::prepareToStop()
{
  W_SHM(getId() << " transferring to stop work");

  stopping = true;

  // switch off the blocking mode
  block.switchOff(TimeSpec(0,0));
  //block.removeTrigger();

  // turn off clock writing
  if (my_index == 0) {
    clockwrite.switchOff(TimeSpec(0,0));
  }

  // and go over to another activity, stopwork
  stopwork.setTrigger(*Ticker::single());
  stopwork.switchOn(TimeSpec(0,0));
}

void ScramNetAccessor::stopWork(const TimeSpec& ts)
{
  if (stop_counter-- > 0) {

    I_SHM(getId() << " stopwork " << stop_counter);

    // write 0 to the control/timing area
    area[my_index] = 0;

    // check whether other nodes are also writing zeros
    bool others_stopping = true;
    for (int ii = no_parties; ii--; )
      others_stopping &= (area[ii] == 0);

    // if node 0, simply keep on writing until counter runs out
    // stop scramnet when others also stopped, or after count
    if ((others_stopping && my_index != 0) || !stop_counter) {

      // make sure we dont do any work any more
      stop_counter = 0;
      stopwork.switchOff(TimeSpec(0, 0));
      //stopwork.removeTrigger();

      // closing scramnet works from the destructor, i.e. in general
      // from another thread
    }
    return;
  }
}

void ScramNetAccessor::blockForData(const TimeSpec& ts)
{

  // attaching to the scramnet interrupt is moved to here, because the
  // threading model of Linux is not quite POSIX compliant. In Linux,
  // each thread has a separate PID, so we have to set up the signal
  // catching and throwing with the PID of the thread that blocks on
  // scramnet IO.
  if (have_to_attach) {


#ifndef USE_WAIT_IOCTL
    I_SHM(getId() << " attaching to interrupt");

    // open the scramnet interrupt device
    if ((scr_int_fd = open("/dev/scr", 0)) == -1) {
      perror("Opening /dev/scr");
      exit(1);           // init related
    }

    // block the SCRAMSIG signal for this thread
    sigset_t mask;
    sigemptyset(&mask); sigaddset(&mask, SCRAMSIG);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    // add this process to the interested parties for a scramnet
    // interrupt
    hand_pid = getpid();
    ioctl( scr_int_fd, FLUSHSIG, NULL); // flush to be sure
    if (ioctl( scr_int_fd, INTSIG, (char*) SET_SIG) != 0) {
      perror("Cannot request SCRAMSIG from scramnet driver");
      std::exit(1);        // init related
    }
#endif

    // flush fifos
    scr_csr_write(SCR_CSR0, 0xf000);

    // re-insert the node with interrupts enabled
    scr_csr_write(SCR_CSR0, 0x812b);

    // arm interrupts, and check
    I_SHM(getId() << " arming interrupt");
    scr_csr_write(SCR_CSR1, 0);
    if (!(scr_csr_read(SCR_CSR1) & CSR1_IARM) ) {
      E_SHM(getId() << " cannot arm interrupts");
      exit(1);            // init related
    }

    have_to_attach = false;

    // this is a hack, hope to get it out someday. "tickle" is a
    // checking activity, used because the interrupts now sometimes
    // get lost
#ifndef USE_WAIT_IOCTL
    tickle.switchOn(ts);
#endif
  }


  // keep doing this until there is another invocation
  // waiting. Getting out once in a while (5 times per second), keeps
  // the stuff reacting
  while ( !(block.noScheduledBehind() || stopping)) {


    // read out the interrupt FIFO
    uint32_t memory_offset;
    int read_this_cycle = 0;
    if (readInterrupt(memory_offset)) {  // normally while

      // and handle it
      read_this_cycle++;
      worked = true;
      handleControlWrite(memory_offset >> 2, ts);
    }

    if (!read_this_cycle) {
      unsigned short errbits = scr_csr_read(SCR_CSR1);
      if (errbits & ~CSR1_IARM)
        W_SHM("error bits" << hex << errbits << dec);
    } else {
      DEB1("cycle of " << read_this_cycle);
    }

    // re-arm interrupts
    scr_csr_write(SCR_CSR1, 0);

    my_activity_manager->logBlockingWait();
#ifdef USE_WAIT_IOCTL
    // at this point the interrupts should be armed. now wait for the
    // signal
    unsigned int numint;
    if (ioctl(scr_csr_fd, WAIT_INTERRUPT, &numint) != 0) {
      perror("cannot wait");
    }
#else
    sigset_t mask; int the_signal;
    sigemptyset(&mask); sigaddset(&mask, SCRAMSIG);
    if (sigwait(&mask, &the_signal) != 0) {
      perror("cannot wait");
    }
#endif
    my_activity_manager->logBlockingWaitOver();
  }
}

void ScramNetAccessor::checkAndTickle(const TimeSpec& ts)
{
  if (!worked && checkInterrupt()) {
    I_SHM("tickle at time " << ts);

    // log a tickle
    timing_check.userReportsAnomaly();

    // flag that any clock pulses may be out of sync
    timing_ok = false;

    // and wake up the waiting thread
    kill (hand_pid, SCRAMSIG);
  }
  else if (block.noScheduledBehind() > 5) {
    // QNX driver does a timeout, Linux driver does not. do a kill when
    // enough instances are built up in the manager, to prevent full
    // managers when communication stops
    kill (hand_pid, SCRAMSIG);
  }
  else {

    // timing should be ok then
    timing_ok = true;
  }

  worked = false;
}

void ScramNetAccessor::write(volatile uint32_t* address, uint32_t value)
{
  DEB1("Write in ScramNetAccessor, offset="
        << address - area_start << " value=" << value);
  *address = value;
}


DUECA_NS_END

