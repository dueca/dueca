/* ------------------------------------------------------------------   */
/*      item            : NetCommunicator.cxx
        made by         : Rene' van Paassen
        date            : 170912
        category        : body file
        description     :
        changes         : 170912 first version
        language        : C++
        copyright       : (c) 17 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define NetCommunicator_cxx
#include "NetCommunicator.hxx"
#include "NetCommunicatorExceptions.hxx"

#define I_NET
#include <debug.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <boost/lexical_cast.hpp>
#include <boost/swap.hpp>
#include <ifaddrs.h>
#include <net/if.h>
#include <exception>
#include <strings.h>
#include <dueca/Ticker.hxx>
#include "CRCcheck.hxx"
#include <dueca/CriticalActivity.hxx>
#include <dueca-conf.h>

#ifdef BUILD_TESTOPT
static const double test_failprob = 0.001;
#define DEBPRINTLEVEL 1
#else
#define DEBPRINTLEVEL -1
#endif

#include <debprint.h>

DUECA_NS_START;

const size_t NetCommunicator::control_size = 22;

NetCommunicator::NetCommunicator() :
  PacketCommunicatorSpecification(),
  ts_interval(Ticker::single()->getBaseIncrement()),
  master_port(7000),
  config_buffer_size(1024),
  data_comm(),
  peer_address(""),
  dataport(7001),
  group_magic(0U),
  peer_cycles(),
  //  message_cycle(uint32_t(4294957696U)), // rollover after 30 s
  message_cycle(uint32_t(0x10)),            // let the rollover, easier interp
  packed_cycle(uint32_t(0x00)),
  current_send_buffer(NULL),
  backup_send_buffer(NULL),
  failure(FailureSimulation::NoFailure),
  coalescing_backlog(2, "coalescing backlog"),
  coalescing_reserve(2, "coalescing reserve"),
  sendstate(Normal),
  trigger_recover(false),
  npeers(0U),
  node_id(uint16_t(0)),
  errorbit(uint16_t(0))
{

}

NetCommunicator::~NetCommunicator()
{

}

#ifdef BUILD_TESTOPT

// a preprocessor define to either keep the message or not send it
#define POSSIBLE_SEND_FAILURE(A)                   \
  if (failure >= FailureSimulation::Coalesce) {    \
    MessageBuffer::ptr_type tmpbuffer = NULL;      \
    if (coalescing_reserve.notEmpty()) {           \
      tmpbuffer = coalescing_reserve.front();      \
      coalescing_reserve.pop();                    \
    } else {                                       \
      tmpbuffer = new MessageBuffer(A ->capacity); \
    }                                              \
    *tmpbuffer = * A;                              \
    coalescing_backlog.push_back(tmpbuffer);       \
  }                                                \
                                                   \
  /* Test for normal sending */                    \
  if (failure < FailureSimulation::FailSend)

#else
// no failure
#define POSSIBLE_SEND_FAILURE(A)
#endif

size_t NetCommunicator::codeAndSendUDPMessage(TimeTickType current_tick)
{

#ifdef BUILD_TESTOPT
#warning "test options selected, simulated message send/coalesce fail added"

  // act depending on the previous failure state
  switch (failure) {
  case FailureSimulation::Coalesce:
    while (coalescing_backlog.notEmpty()) {
      DEB("Sending message from the coalescing backlog");
      data_comm->send(coalescing_backlog.front());
      coalescing_reserve.push_back(coalescing_backlog.front());
      coalescing_backlog.pop();
    }
  case FailureSimulation::NoFailure:
  case FailureSimulation::FailSend:
  case FailureSimulation::ExtraDelay:
    // calculate the new case
    {
      int randresult = int(random()/test_failprob/0.1/(RAND_MAX+1.0));
      if (randresult == 0) {
        DEB("test failing to send message " << message_cycle <<
            " in state " << sendstate);
        failure = FailureSimulation::FailSend;
      }
      else if (randresult == 1) {
        useconds_t delay =
          useconds_t(round(double(random())/double(RAND_MAX)*100000));
        DEB("test additional delay in sending message" << message_cycle <<
            " in state " << sendstate << " delay " << delay << " [us]");
        usleep(delay);
        failure = FailureSimulation::ExtraDelay;
      }
      else if (randresult <= 3) {
        DEB("test single coalesce " << message_cycle <<
            " in state " << sendstate);
        failure = FailureSimulation::Coalesce;
      }
      else if (randresult <= 6) {
        DEB("test double coalesce " << message_cycle <<
            " in state " << sendstate);
        failure = FailureSimulation::CoalesceTwice;
      }
      else if (randresult <= 9) {
        DEB("test triple coalesce " << message_cycle <<
            " in state " << sendstate);
        failure = FailureSimulation::CoalesceTriple;
      }
      else {
        failure = FailureSimulation::NoFailure;
      }
    }
    break;
  case FailureSimulation::CoalesceTwice:
    failure = FailureSimulation::Coalesce;
    break;
  case FailureSimulation::CoalesceTriple:
    failure = FailureSimulation::CoalesceTwice;
    break;
  }
#endif

  // start with UDP sending/receiving
  switch(sendstate) {
  case Normal: {

    if (!packed_cycle.cycleIsNext(message_cycle)) {
      /* @todo

         This case occurs on a peer, in a SIMONA project, feb 2022, it
         triggers disconnect of the websocket connection, then a
         crash.

         The cause is strange. Sendstate is adapted here to Stasis
         (preventing sending the same buffer/count twice).

         The only spot where sendstate is adapted to Normal is in
         NetCommunicatorPeer. It only occurs when the above test is
         true; updating of packed_cycle is only in the code here
         below.

         The erroneous behaviour occurs on FlyingVLatDirHQ
      */
      /* DUECA network.

         Error with message ID's in the received messages. Possibly
         due to network issues that may re-order messages? */
      E_NET("Cycle confusion, last packed (or init)" << packed_cycle <<
            " now in " << message_cycle);
      CriticalActivity::criticalErrorNodeWide();
    }

    // normal progress, switch buffers
    boost::swap(current_send_buffer, backup_send_buffer);
    current_send_buffer->message_cycle = message_cycle.cycle_counter;
    packed_cycle = message_cycle;

    {
      ControlBlockWriter i_
        (current_send_buffer, group_magic,
         message_cycle, peer_id, npeers, current_tick, errorbit);

      current_send_buffer->fill = control_size;

      // pack the payload
      clientPackPayload(current_send_buffer);

      // add the timing information
      communicatorAddTiming(i_);

      // set the sendstate to stasis; will be adjusted after
      // master updates to a new cycle
      sendstate = AfterNormal;
    } // end of ControlBlockWriter, completes crc

    DEB2("peer " << peer_id <<
         " send message, cycle " << message_cycle <<
         " size " << current_send_buffer->fill <<
         (errorbit ? " +err" : ""));

    // send over broadcast if?
    // REP_DEBUG("udp send "); s.print(std::cerr); std::cerr << std::endl;
    POSSIBLE_SEND_FAILURE(current_send_buffer) {
      data_comm->send(current_send_buffer);
    }
  }
    return current_send_buffer->fill;

  case Recover: {

    // flags in cycle n, indicate that one of the peers did not
    // receive cycle n; re-send the backup buffer, and then progress
    // to sending the cycle n buffer
    /* DUECA network.

       Information message on recover attempts. */
    I_NET("Peer " << peer_id <<
          " UDP recover stage 1, size " << backup_send_buffer->fill <<
          " cycle " << message_cycle);

    if (!message_cycle.cycleIsCurrent(backup_send_buffer->message_cycle)) {
      /* DUECA network.

         Unrecoverable error in the network protocol, unexpected numbers
         received in the recover phase.
      */
      E_NET("Recover phase, message cycle " << message_cycle <<
            " buffer cycle " <<
            CycleCounter(backup_send_buffer->message_cycle));
      CriticalActivity::criticalErrorNodeWide();
    }

    // message cycle has been reset correctly?
    //assert(message_cycle.cycleIsCurrent(backup_send_buffer->message_cycle));
    // only peer 0 may have an errorbit in this case
    // assert(errorbit == 0 || peer_id == 0);

    // re-code the control buffer
    {
      ControlBlockWriter i_
        (backup_send_buffer, group_magic,
         message_cycle, peer_id, npeers, current_tick, uint16_t(0x0000));

      // add the timing information again
      communicatorAddTiming(i_);
    }

    POSSIBLE_SEND_FAILURE (backup_send_buffer) {
      data_comm->send(backup_send_buffer);
    }
  }
    return backup_send_buffer->fill;

  case AfterNormal:
    /* DUECA network.

       Warning on recovery status in communication. */
    W_NET("Peer " << peer_id << " AfterNormal stasis, cycle "
          << message_cycle);

    // intentional fall through
  case Stasis: {

    // re-send the current buffer
    /* DUECA network.

       Information what message is being re-sent. */
    I_NET("Peer " << peer_id <<
          " UDP recover stage 2, size " << current_send_buffer->fill <<
          " cycle " << message_cycle);

    if (!message_cycle.cycleIsCurrent(current_send_buffer->message_cycle) ||
        !(message_cycle.cycleIsCurrent(packed_cycle))) {
      /* DUECA network.

         Unrecoverable error in message numbers.
      */
      E_NET("Cycle issue in Stasis mode, message_cycle " << message_cycle <<
            " buffer_cycle " <<
            CycleCounter(current_send_buffer->message_cycle) <<
            " packed_cycle " << packed_cycle);
      CriticalActivity::criticalErrorNodeWide();
    }

    // re-code the control buffer
    {
      ControlBlockWriter i_
        (current_send_buffer, group_magic,
         message_cycle, peer_id, npeers, current_tick, errorbit);

      // add the timing information
      communicatorAddTiming(i_);
    }
    POSSIBLE_SEND_FAILURE (current_send_buffer) {
      data_comm->send(current_send_buffer);
    }
  }
    return current_send_buffer->fill;
  }

  // unreachable statement, but SLE_12 gcc does not know
  return 0U;
}

NetCommunicator::ControlBlockWriter::ControlBlockWriter
(MessageBuffer::ptr_type buffer, uint32_t group_magic,
 const dueca::CycleCounter& cycle, uint16_t peer_id, uint16_t npeers,
 TimeTickType tick, bool error) :
  buffer(buffer),
  s(buffer->buffer, NetCommunicator::control_size),
  checksum_mark(s.createMark(uint16_t())),
  usecsoffset_mark(s.createMark(int32_t())),
  tick(tick)
{
  s.packData(group_magic);
  s.packData(cycle);
  s.packData(uint16_t(peer_id | (error? uint16_t(0x8000) : uint16_t(0x0000))));
  s.packData(npeers);
  s.packData(tick);
}

template<class D>
D limit(D low, D val, D hig)
{ return low < val ? val : ( hig > val ? hig : val); }

void NetCommunicator::ControlBlockWriter::markTimeOffset
  (double net_permessage, double net_perbyte)
{
  int64_t usecpast = Ticker::single()->getUsecsSinceTick(tick);
  int32_t usecpred =
    int32_t(limit(double(numeric_limits<int32_t>::min()),
                  double(usecpast +
                         net_permessage +
                         net_perbyte * buffer->fill),
                  double(numeric_limits<int32_t>::max()) ));
  s.finishMark(usecsoffset_mark, usecpred);
}

void NetCommunicator::ControlBlockWriter::markSendTime()
{
  int64_t usecpast = Ticker::single()->getUsecsSinceTick(tick);
  s.finishMark(usecsoffset_mark, int32_t(usecpast));
}

NetCommunicator::ControlBlockWriter::~ControlBlockWriter()
{
  uint16_t crc = crc16_ccitt(&(buffer->buffer[2]), buffer->fill - 2);
  s.finishMark(checksum_mark, crc);
}

NetCommunicator::ControlBlockReader::ControlBlockReader
(MessageBuffer::ptr_type buffer) :
  r(buffer->buffer, buffer->fill),
  crcvalue(r),
  usecs_offset(r),
  group_magic(r),
  cycle(r),
  peer_id(r),
  npeers(r),
  peertick(r),
  errorflag((peer_id & 0x8000) != 0),
  crcgood(crcvalue == crc16_ccitt(&(buffer->buffer[2]), buffer->fill - 2))
{
  peer_id &= uint16_t(0x7fff);
}

uint16_t NetCommunicator::ControlBlockReader::decodePeerId(MessageBuffer::ptr_type buffer)
{
  AmorphReStore r(&(buffer->buffer[14]), 2);
  return uint16_t(r) & uint16_t(0x7fff);
}

void NetCommunicator::communicatorAddTiming(ControlBlockWriter &cb)
{
  cb.markSendTime();
}

DUECA_NS_END;

std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::NetCommunicator::SendState& x)
{
  const char* names[] = { "Normal", "Stasis", "Recover", "AfterNormal" };
  return os << names[unsigned(x)];
}
