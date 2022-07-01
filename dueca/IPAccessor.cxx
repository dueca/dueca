/* ------------------------------------------------------------------   */
/*      item            : IPAccessor.cxx
        made by         : Rene' van Paassen
        date            : 990611
        category        : body file
        description     :
        changes         : 990611 first version
                          040403 Logic overhaul
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define IPAccessor_cc
#include <dueca-conf.h>

//#define D_NET
//#define I_NET
#define W_NET
#define E_NET

#define packet_after(a, b) ((int32_t)(b) - (int32_t)(a) < 0)

#include <dueca.h>
#include <IPAccessor.hxx>
#include <Packer.hxx>
#include <Unpacker.hxx>
#include <FillPacker.hxx>
#include <FillUnpacker.hxx>
#include <Ticker.hxx>
#include <TimeSpec.hxx>
#include <ObjectManager.hxx>
#include <NameSet.hxx>
#include <TransportDelayEstimator.hxx>
#include <debug.h>
#include "CriticalActivity.hxx"
#include <TimingCheck.hxx>
#include "Environment.hxx"

#include <fstream>
#include <cerrno>
#include <iomanip>
#include <sys/types.h>
#ifndef __MINGW32__
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <sys/file.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define TIMING_BYTES 8
// layout timing info: time-tick; 4 bytes + microsecond offset 4 bytes
#define CONTROL_BYTES 10
// layout control info: send_order; 2 bytes, data_sending_phase 1
// bytes, i_missed_data 1 byte, current_packet_no 4 bytes, primary
// data size 2 bytes

#define DO_INSTANTIATE
#include "Callback.hxx"
#define DEBPRINTLEVEL -1
#include <debprint.h>
DUECA_NS_START


int IPAccessor::sequence = 0;
#undef TEST_OPTIONS
#ifdef TEST_OPTIONS
static const double prob = 0.001/6.0;
#endif
using namespace std;

IPAccessor::IPAccessor() :
  Accessor(NameSet("dueca", "IP-accessor",
                   1000*ObjectManager::single()->getLocation() +
                   sequence++)),
  TriggerPuller(std::string("IPAccessor(") + getPart() + std::string(")")),
  output_store(NULL),
  output_status(NULL),
  store_in_use(0, 1),
  store_to_send(-1),
  bytes_to_send(0),
  output_store_size(0),
  input_store_size(0),
  output_packet_size(0),
  n_input_stores(2),
  n_output_stores(2),
  current_packet_no(NULL),
  send_order(0),
  no_of_senders(1),
  sockfd_high(0),
  timeout(50),
  time_spec(1, 1),
  io_cycle_phase(0),
  i_missed_data(false),
  all_ok(true),
  running(true),
  comm_state(SyncOnZero),
  purge_extra_messages(true),

  time_info_store(),
  control_info_store(),
  time_info_restore(),
  control_info_restore(),
  tdelay_estimator(NULL),
  cb(this, &IPAccessor::runIO),
  net_io(getId(), "IP transport", &cb, PrioritySpec(0,0))
{
  //
}

bool IPAccessor::complete()
{
  bool res = Accessor::complete();

  // am using the presence of the output store pointer to detect
  // re-init;
  SCRIPTSTART_CHECK2(output_store == NULL, "Cannot re-initialize");

  // check that required parameters are filled in
  SCRIPTSTART_CHECK(packer != NULL);
  SCRIPTSTART_CHECK(unpacker != NULL);
  SCRIPTSTART_CHECK(n_output_stores > 1);
  SCRIPTSTART_CHECK(n_input_stores > 1);

  // store use counters need to be updated
  store_in_use.setLimit(n_output_stores);

  // useful store size is reduced by space for control bytes
  output_store_size = output_packet_size - (CONTROL_BYTES + TIMING_BYTES);
  input_store_size = input_packet_size - (CONTROL_BYTES + TIMING_BYTES);

  // check that there is a possiblility to send and receive data
  SCRIPTSTART_CHECK(output_store_size > 0);
  SCRIPTSTART_CHECK(input_store_size > 0 || no_of_senders == 1);

  // check number senders + order logic
  SCRIPTSTART_CHECK(no_of_senders > 0);
  SCRIPTSTART_CHECK(send_order >= 0 && send_order < no_of_senders);

  // timeout
  SCRIPTSTART_CHECK(timeout > 0);
  SCRIPTSTART_CHECK(time_spec.getValiditySpan() > 0);

  if (!res) return res;

  // allocate room for the send buffers
  output_store = new char*[n_output_stores];
  output_status = new int[n_output_stores];
  for (int ii = n_output_stores; ii--; ) {
    output_status[ii] = 0;
    output_store[ii] = new char[output_packet_size];
    memset(output_store[ii], '\000', output_packet_size);
  }

  // pass the send buffers to the packer. The packer will create
  // AmorphStore objects to handle the send buffers and further
  // initialise itself

  packer->initialiseStores(output_store, output_status,
                           n_output_stores, output_store_size);

  // write a number of message buffers into the available buffer queue
  for (unsigned ii = n_input_stores; ii--; ) {
    MessageBuffer::ptr_type nb = new MessageBuffer(input_packet_size);
    this->returnBuffer(nb);
  }

  // inform unpackers
  unpacker->initialiseStores();
  if (fill_unpacker) {
    fill_unpacker->initialiseStores(send_order, no_of_senders);
  }

  // initialise the packet no
  current_packet_no = new uint32_t[max(no_of_senders, 2)];
  for (int ii = max(no_of_senders,2); ii--; ) current_packet_no[ii] = 0;
  if (send_order == 0) current_packet_no[0] = 0x1000;

  // reserve room for the sockets
  sockfd = new int[2*no_of_senders];

  // specify the timing for activation, provisional, 1 on 1 during
  // initial contact
  net_io.setTimeSpec(time_spec);

  // specify activation by the time service channel, for send_order = 0,
  // otherwise auto_invoke
  net_io.switchOn(time_spec);
  net_io.setTrigger(*Ticker::single());

  if (send_order == 0) {
    // no 0 should round off within the period. Warning on excessive
    // use (0.9 times period)
    new TimingCheck(net_io, int(time_spec.getDtInSeconds() * 900000.0),
                    int(time_spec.getDtInSeconds() * 1500000.0),
                    int(60.0 / time_spec.getDtInSeconds()));
  }
  else {
    // others basically need to warn when doing twice the period
    new TimingCheck(net_io, int(time_spec.getDtInSeconds() * 2000000.0),
                    int(time_spec.getDtInSeconds() * 3500000.0),
                    int(60.0 / time_spec.getDtInSeconds()));
  }

  // calculate the timeout value for initial contact running
  initial_tv_wait.tv_sec =  1;
  initial_tv_wait.tv_usec = 0;

  // calculate the timeout value for real-time running
  realtime_tv_wait.tv_sec =  timeout / 1000000;
  realtime_tv_wait.tv_usec = timeout % 1000000;

  // initialise working paramters for running
  io_cycle_phase = 0;               // start with number 0
  i_missed_data = false;            // everything still ok

  // notify the ticker when our syncing depends on someone else
  if (send_order != 0) Ticker::single()->noImplicitSync();


  return res;
}

IPAccessor::~IPAccessor()
{
  // close the sockets
  if (sockfd) {
    for (int ii = no_of_senders; ii--; ) {
      close(sockfd[ii]);
    }
  }

  // delete socket and packet no counter
  delete [] sockfd;
  delete [] current_packet_no;


  // delete output store space and management
  for (int ii = store_in_use.getLimit(); ii--; ) {
    delete [] output_store[ii];
  }
  delete [] output_status;
  delete [] output_store;
}

void IPAccessor::runIO(const TimeSpec& ts)
{
  if (send_order)
    despatch1toN(ts);
  else
    despatch0(ts);
}

// Auxiliary routine, to wait for a sender
int IPAccessor::waitPacket(int socket)
{

#ifdef TEST_OPTIONS
  int try_error = int(random()/prob/(RAND_MAX+1.0));
  if (try_error == send_order) {
    DEB(getId() << " pretending to be delayed");
    usleep(10000);
  }
#endif

  // prepare the file descriptor set
  fd_set party_fds;
  FD_ZERO(&party_fds);
  FD_SET(socket, &party_fds);
  int result = 0;

  // prepare the timeval for the timeout time. Take a long timeout
  // when waiting on number 0, or in the initialization phase, a short
  // timeout when waiting on some other node.
  struct timeval tv_wait =
    (socket != sockfd[0] && CSE.initialisationComplete())
    ? realtime_tv_wait : initial_tv_wait;

  // inform blocking wait starts
  net_io.logBlockingWait();

  // check for data, with a timeout possibility
  if ((result =
       select(sockfd_high, &party_fds, NULL, NULL, &tv_wait)) == -1) {

    perror("Problem reading select");

    // Interrupted system call, happens when starting new OGRE on SLED
    // 11 SP1 (hmilab)
#ifndef ERESTART
#define ERESTART EINTR
#endif
    if (errno == EINTR || errno == ERESTART) return 0;

    // this should not happen, a real problem with the
    // sockets. Simply stop, it is irrepairable anyway
    net_io.switchOff(0);        // stop this activity
    CriticalActivity::criticalErrorNodeWide();
  }

  // blocking wait stopped
  net_io.logBlockingWaitOver();

#ifdef TEST_OPTIONS
  if (result > 0 && try_error == 2*send_order) {
    DEB(getId() << " pretending no data came");
    recv(socket, dummy_buffer, input_packet_size, 0);
    return 0;
  }
#endif

  return result;
}

// logic for nodes who are not mastering
void IPAccessor::despatch1toN(const TimeSpec& ts)
{
  assert(send_order != 0);

  // prevent a build up of activation instances
  if (net_io.noScheduledBehind()) {
    DEB("Lagging, so leaving");
    return;
  }

  while (running) {

    switch(comm_state) {

    case DataSend: {

      assert(io_cycle_phase == send_order);

      // send the data
      DEB(getId() << "IPAccessor, sending " <<
            current_packet_no[send_order]/0x1000 << '/' <<
            (current_packet_no[send_order]&0xfff));

      sendMyData(ts);

      // update logic. If this was last sender, return to wait on no
      // 0, otherwise go on with receiving
      io_cycle_phase++;
      if (io_cycle_phase == no_of_senders) {
        io_cycle_phase = 0;
        comm_state = SyncOnZero;
      }
      else {
        comm_state = DataReceive;
      }

      break;
    }

    case DataReceive: {

      assert(io_cycle_phase < no_of_senders);
      assert(io_cycle_phase != send_order);
      assert(io_cycle_phase != 0);

      MessageBuffer::ptr_type buf = getBuffer();
      char* buffer = buf->buffer;

      do {

        // wait for availability of data
        if (waitPacket(sockfd[io_cycle_phase]) == 0) {

          // report no data.
          /* DUECA network.

             Data from a peer did not arrive. Usually this condition
             is temporary, check timeout values, and network
             condition. */
          W_NET(getId() << "IPAccessor, no data from " << io_cycle_phase);

          // no data arrived, re-sync on 0's messages and remember to
          // clear out stuff
          io_cycle_phase = 0;
          comm_state = SyncOnZero;
          purge_extra_messages = true;
          net_io.getCheck()->userReportsAnomaly();
#ifdef LOG_COMMUNICATIONS
          if (log_communications) {
            commlog << "- " << setw(3) << io_cycle_phase << endl;
          }
#endif
        }
        else {
          bytes = recv(sockfd[io_cycle_phase], buffer, input_packet_size,
                       MSG_TRUNC);
          if (bytes > input_packet_size) {
            /* DUECA network.

               A packet arrived from a peer that is too large for
               reading. Correct this in the DUECA configuration file.
            */
            E_NET(getId() << " cannot accomodate packet size " <<
                  bytes << ", check dueca.cnf");
            CriticalActivity::criticalErrorNodeWide();
            prepareToStop();
            returnBuffer(buf);
            return;
          }

          // check all OK
          if (!getControlData(io_cycle_phase, buffer, ts)) {

            io_cycle_phase = 0;
            comm_state = SyncOnZero;
            purge_extra_messages = true;
            net_io.getCheck()->userReportsAnomaly();
          }

          // report if there is a stuck packet found here
          else if (current_packet_no[send_order] !=
                   data_says_current_packet_no) {
            /* DUECA network.

               Received an old message from a previous cycle. Removing
               the packet.
            */
            I_NET(getId() << "Found stuck data from " << io_cycle_phase <<
                  " pack=" << data_says_current_packet_no/0x1000 << '/' <<
                  (data_says_current_packet_no&0xfff));
          }
        }
        DEB(getId() << "IPAccessor " << bytes <<
              " from " << io_cycle_phase << '=' << data_says_send_number);

        // do this until we find the packet number as no 0 commanded it
      } while (comm_state == DataReceive &&
               packet_after(current_packet_no[send_order],
                            data_says_current_packet_no));

      // stop further processing if we got out due to an anomaly
      if (comm_state != DataReceive) {
        returnBuffer(buf);
        break;
      }

      // at this point, we have reached target packet set by 0, unpack
      // and use in system if we have not seen this packet before.
      if (packet_after(data_says_current_packet_no & 0xfffff000,
                       current_packet_no[io_cycle_phase] & 0xfffff000)) {

        buf->fill = bytes;    // bytes has been adjusted subtracting control
        buf->regular = data_says_regular_bytes;
        buf->origin = io_cycle_phase; // in send order, not node id?
        if (getDuecaData(ts, buf)) {
          current_packet_no[io_cycle_phase] = data_says_current_packet_no;
        }
      }
      else {
        /* DUECA network.

           Receive repeated data. */
        I_NET(getId() << "IPAccessor, repeated data from " <<
              io_cycle_phase << " pack=" <<
              data_says_current_packet_no/0x1000 << '/' <<
              (data_says_current_packet_no & 0xfff));
      }

      returnBuffer(buf);

      // update io cycle stuff
      io_cycle_phase++;

      // it may be my turn to send data
      if (io_cycle_phase == send_order) {
        comm_state = DataSend;
        if (net_io.noScheduledBehind() ||
            !CSE.initialisationComplete()) return;
      }

      // it may be time to send confirm, or return to wait on 0
      if (io_cycle_phase == no_of_senders) {
        if (send_order == no_of_senders - 2) {
          comm_state = SendConfirm;
        }
        else {
          comm_state = WaitConfirm;
          io_cycle_phase = send_order + 1;
        }
      }
      break;
    }

    case WaitConfirm: {

      assert(send_order < no_of_senders - 2);
      assert(no_of_senders > 3);

      do {

        // wait for the preceding confirm packet
        if (waitPacket(sockfd[no_of_senders + send_order + 1]) == 0) {

          // no data arrived, re-sync on 0's messages and remember to
          // clear out stuff
          // no need to set i_missed_data
          io_cycle_phase = 0;
          comm_state = SyncOnZero;
          purge_extra_messages = true;
          net_io.getCheck()->userReportsAnomaly();
          /* DUECA network.

             Missing a confirm message from one of the nodes. */
          W_NET(getId() << "IPAccessor, no confirm from " << send_order+1);
          }
        else {
          char buffer[CONTROL_BYTES+TIMING_BYTES];
          bytes = recv(sockfd[no_of_senders + send_order + 1], buffer,
                       CONTROL_BYTES+TIMING_BYTES, 0);

          if (!getControlData(send_order+1, buffer, ts)) {
            io_cycle_phase = 0;
            comm_state = SyncOnZero;
            purge_extra_messages = true;
            net_io.getCheck()->userReportsAnomaly();
          }

          else if (current_packet_no[send_order] !=
                   data_says_current_packet_no) {
            /* DUECA network.

               Found an old confirm message from one of the nodes. */
            I_NET(getId() << "Found stuck confirm from " << send_order <<
                  " pack=" << data_says_current_packet_no/0x1000 << '/' <<
                  (data_says_current_packet_no&0xfff));
          }
        }
        DEB(getId() << "IPAccessor confirm from " <<
              io_cycle_phase << '=' << data_says_send_number);


      } while (comm_state == WaitConfirm &&
               packet_after(current_packet_no[send_order],
                            data_says_current_packet_no));

      if (comm_state != WaitConfirm) break;

      // intentional fall through, next step is sending confirm
    }

    case SendConfirm: {

      assert(no_of_senders > 2);
      assert(send_order < no_of_senders - 1);

      DEB(getId() << "IPAccessor, sending confirm " <<
            current_packet_no[send_order]/0x1000 << '/' <<
            (current_packet_no[send_order]&0xfff) << ", m=" <<
            i_missed_data);
      sendMyConfirm(ts);

      // now time to wait on 0 again
      io_cycle_phase = 0;

      comm_state = SyncOnZero;

      break;
    }

    case SyncOnZero: {

      assert(io_cycle_phase == 0);
      i_missed_data = false;        // begin with good hopes

      if (waitPacket(sockfd[0]) != 0) {

        MessageBuffer::ptr_type buf = getBuffer();
        char* buffer = buf->buffer;

        // receive the initial installment
        bytes = recv(sockfd[0], buffer, input_packet_size, MSG_TRUNC);
        if (bytes > input_packet_size) {
          /* DUECA network.

             A packet arrived from a peer that is too large for
             reading. Correct this in the DUECA configuration file.
          */
          E_NET(getId() << " cannot accomodate packet size " <<
                bytes << ", check dueca.cnf");
          CriticalActivity::criticalErrorNodeWide();
          prepareToStop();
          returnBuffer(buf);
          return;
        }
        if (!getControlData(0, buffer, ts)) {
          returnBuffer(buf);
          break;
        }

        // purge the master channel if this is hinted by the sender, or when
        // we came here by breaking off a sequence
        if ((purge_extra_messages || data_says_i_missed_data)
            && spinToLast(0, buffer)) {
          /* DUECA network.

             The master node initiated a new cycle, following that one
             now. */
          W_NET(getId() << "pre-empted from 0, pack=" <<
                data_says_current_packet_no/0x1000 << '/' <<
                (data_says_current_packet_no & 0xfff));
          if (!getControlData(0, buffer, ts)) {
            returnBuffer(buf);
            break;
          }
        }
        purge_extra_messages = false; // done purging

        DEB(getId() << "packet from 0, b=" << bytes << " pack=" <<
              data_says_current_packet_no/0x1000 << '/' <<
              (data_says_current_packet_no & 0xfff));

        // read data if this means a packet number increase
        if (packet_after(data_says_current_packet_no & 0xfffff000,
                         current_packet_no[0] & 0xfffff000)) {
          // no 0 starts new cycle, no re-sending old data
          bytes_to_send = 0;

          buf->fill = bytes;    // bytes has been adjusted subtracting control
          buf->regular = data_says_regular_bytes;
          buf->origin = io_cycle_phase; // in send order, not node id?
          if (getDuecaData(ts, buf)) {
            current_packet_no[0] = data_says_current_packet_no;
          }
        }
        else {
          /* DUECA network.

             Received an old message from a previous cycle. Removing
             the packet.
          */
          I_NET(getId() << "IPAccessor, repeated data from 0 pack=" <<
                (data_says_current_packet_no/0x1000) << '/' <<
                (data_says_current_packet_no & 0xfff));
        }

        returnBuffer(buf);

        // always get the current packet number of 0, as a reference
        // to stamp my own send packet
        current_packet_no[send_order] = data_says_current_packet_no;

        // go on with rest of work
        io_cycle_phase++;
        if (send_order == io_cycle_phase) {
          comm_state = DataSend;
          if (net_io.noScheduledBehind()||
              !CSE.initialisationComplete()) return;
        }
        else {
          comm_state = DataReceive;
        }
      }
      else {
        DEB(getId() << "IPAccessor, no data from comm master");
      }

      break;
    } // end SyncOnZero

    } // end of switch statement

  } // end of while loop


}

void IPAccessor::setForCleaningRound()
{
  // some anomaly occurred, cannot increase packet counter, and have
  // to indicate cleaning out double packets
  net_io.getCheck()->userReportsAnomaly();
  int repeat = (current_packet_no[0] & 0xfff) + 1;
  current_packet_no[0] = (current_packet_no[0] & 0xfffff000) |
    (repeat & 0xfff);
  // reset the i_missed_data flag. We also get here when there were no
  // buffers to unpack the data
  i_missed_data = false;
}


// logic for nodes 0, timing master
void IPAccessor::despatch0(const TimeSpec& ts)
{
  assert(send_order == 0);

  if (net_io.noScheduledBehind()) {
    DEB("Lagging, so leaving");
    return;
  }

  // send data
  sendMyData(ts);

  // initialize checks
  all_ok = true;

  // receive data
  for (io_cycle_phase = 1; io_cycle_phase < no_of_senders; io_cycle_phase++) {

    MessageBuffer::ptr_type buf = getBuffer();
    char* buffer = buf->buffer;

    // this loop repeats until the reported data packet/repeat number
    // is in sync with the numbers sent, or until a timeout
    do {
      if (waitPacket(sockfd[io_cycle_phase]) == 0) {

        setForCleaningRound();
        returnBuffer(buf);

        /* DUECA network.

           Data from a peer did not arrive. Usually this condition
           is temporary, check timeout values, and network
           condition. */
        W_NET(getId() << "IPAccessor no data from " << io_cycle_phase);
        return;
      }

      // read the data
      bytes = recv(sockfd[io_cycle_phase], buffer, input_packet_size,
                   MSG_TRUNC);
      if (bytes > input_packet_size) {
        /* DUECA network.

           A packet arrived from a peer that is too large for
           reading. Correct this in the DUECA configuration file.
        */
        E_NET(getId() << " cannot accomodate packet size " <<
              bytes << ", check dueca.cnf");
        CriticalActivity::criticalErrorNodeWide();
        prepareToStop();
        returnBuffer(buf);
        return;
      }
      if (!getControlData(io_cycle_phase, buffer, ts)) {
        setForCleaningRound();
        returnBuffer(buf);
        /* DUECA network.

           A packet arrived that could not be correctly decoded. Are
           there other programs sending data on the same address?
        */
        W_NET(getId() << "IPAccessor invalid data");
        return;
      }

      DEB(getId() << "IPAccessor " << bytes <<
            " from " << io_cycle_phase << '=' << data_says_send_number <<
            " p=" << data_says_current_packet_no/0x1000 << '/' <<
            (data_says_current_packet_no & 0xfff) << " m=" <<
            data_says_i_missed_data << " ok=" << all_ok);

    } while (current_packet_no[0] != data_says_current_packet_no);

    // read the data, if indeed we advanced the true packet count
    if (packet_after(data_says_current_packet_no & 0xfffff000,
                     current_packet_no[io_cycle_phase] & 0xfffff000)) {
      buf->fill = bytes;    // bytes has been adjusted subtracting control
      buf->regular = data_says_regular_bytes;
      buf->origin = io_cycle_phase; // in send order, not node id?
      if (getDuecaData(ts, buf)) {
        current_packet_no[io_cycle_phase] = data_says_current_packet_no;
      }
    }

    returnBuffer(buf);
    // prevent packet counter increasing if data was lost
    all_ok &= !data_says_i_missed_data;
  }


  // receive confirms
  for (io_cycle_phase = no_of_senders - 2; io_cycle_phase > 0;
       io_cycle_phase--) {
    int sockno = io_cycle_phase + no_of_senders;

    do {
      if (waitPacket(sockfd[sockno]) == 0) {

        setForCleaningRound();
        /* DUECA network.

           Missing a confirm message.
        */
        W_NET(getId() << "IPAccessor no confirm from " << io_cycle_phase);
        return;
      }

      // read the data
      char dummy_buffer[CONTROL_BYTES];
      bytes = recv(sockfd[sockno], dummy_buffer, CONTROL_BYTES, 0);

      if (!getControlData(io_cycle_phase, dummy_buffer, ts)) {
        setForCleaningRound();
        /* DUECA network.

           Invalid confirm message.
        */
        I_NET(getId() << "IPAccessor invalid confirm from " << io_cycle_phase);
        return;
      }

      DEB(getId() << "IPAccessor confirm from " <<
            sockno - no_of_senders << '=' << data_says_send_number <<
            " p=" << data_says_current_packet_no/0x1000 << '/' <<
            (data_says_current_packet_no & 0xfff) << " m=" <<
            data_says_i_missed_data << " ok=" << all_ok);

    } while (current_packet_no[0] != data_says_current_packet_no);

    // prevent packet counter increasing if data was lost in confirm round
    all_ok &= !data_says_i_missed_data;
  }

  // when here, a complete cycle has been sent and received, increase
  // data counter
  if (all_ok) {
    current_packet_no[0] = (current_packet_no[0] & 0xfffff000) + 0x1000;
    i_missed_data = false; bytes_to_send = 0;
  }
  else {
    setForCleaningRound();
  }
}


void IPAccessor::sendMyData(const TimeSpec& t)
{
#ifdef TEST_OPTIONS
  int try_error = int(random()/prob/(RAND_MAX+1.0));

  if (try_error == send_order) {
    int delay = 100000 + 10000*(random() % 8) +
      (send_order == 0 ? 900000 : 0);
    DEB(getId() << " extra delay " << delay/1000 << "ms, testing mode");
    usleep(delay);
  }
#endif

  // get new data from the packer, but only if the old data has
  // been accepted (bytes_to_send = 0)
  if (bytes_to_send == 0) {
    packer->packWork();
    regular_bytes = packer->changeCurrentStore(store_to_send);

    // now if we have a filler ...
    if (fill_packer != NULL) {

      fill_packer->packWork();

      bytes_to_send = regular_bytes + fill_packer->
        stuffMessage(&output_store[store_to_send][regular_bytes],
                     output_store_size - regular_bytes);
      DEB("stuff with " << bytes_to_send - regular_bytes <<
            " first byte=" <<
            int(output_store[store_to_send][regular_bytes]) <<
            " last byte=" <<
            int(output_store[store_to_send][bytes_to_send - 1]));
    }
    else {
      bytes_to_send = regular_bytes;
    }
  }
  else {

    // using the old data, but replacing the control data
    bytes_to_send -= CONTROL_BYTES;
    if (send_order == 0) bytes_to_send -= TIMING_BYTES;
  }

#ifdef LOG_COMMUNICATIONS
  // number of regular packed bytes
  // number of fill bytes
  if (log_communications) {
    commlog << "O "
            << setw(9) << t.getValidityStart()
            << setw(3) << send_order
            << setw(6) << regular_bytes
            << setw(6) << bytes_to_send - regular_bytes;
  }
#endif

  // at this point, there may be some data in the send buffer. If
  // there is none there is no data to send. Add my control information
  control_info_store.acceptBuffer
    (&output_store[store_to_send][bytes_to_send], CONTROL_BYTES);
  bytes_to_send += CONTROL_BYTES;
  packData(control_info_store, uint16_t(send_order));
  packData(control_info_store, bool(true)); // data sending phase
  packData(control_info_store, i_missed_data);
  packData(control_info_store, current_packet_no[send_order]);
  packData(control_info_store, regular_bytes);
  if (i_missed_data)
    /* DUECA network.

       Not all data arrive, sending a missed data flag in the confirm
       message. */
    W_NET(getId() << " sending missed data flag in data pack " <<
          (current_packet_no[send_order]/0x1000) << '/' <<
          (current_packet_no[send_order]&0xfff));

#ifdef LOG_COMMUNICATIONS
  if (log_communications) {
    commlog << setw(2) << false
            << setw(2) << i_missed_data
            << setw(7) << current_packet_no[send_order] / 0x1000 << ':'
            << setfill('0') << setw(4) << (current_packet_no[send_order] & 0xfff)
            << setfill(' ');
  }
#endif

  // If I am sender number 0, add timing information
  if (send_order == 0) {
    time_info_store.acceptBuffer
      (&output_store[store_to_send][bytes_to_send], TIMING_BYTES);
    bytes_to_send += TIMING_BYTES;

    // find out at which (nominal) time the tick took place
    int32_t offset_usecs = 0;

    // find out the time now, so we know the offset into the time tick
    offset_usecs = Ticker::single()->
      getUsecsSinceTick(t.getValidityStart());

    // add the estimated no of microsecs for transport to the offset
    int32_t tdelay = tdelay_estimator != NULL ? (*tdelay_estimator)(bytes_to_send) : 0;

    //offset_usecs += 100;
    DEB("offset sending " << offset_usecs);

    // store the time spec and offset as extra data in the packet
    packData(time_info_store, t.getValidityStart());
    packData(time_info_store, offset_usecs + tdelay);

#ifdef LOG_COMMUNICATIONS
  if (log_communications) {
    commlog << setw(6) << offset_usecs
            << setw(6) << tdelay << endl;
  }
#endif
  }
#ifdef LOG_COMMUNICATIONS
  else if (log_communications) {
    commlog << endl;
  }
#endif

   DEB("Sending packet no " << (current_packet_no[send_order]/0x1000) <<
        '/' << (current_packet_no[send_order]&0xfff) << " sz=" <<
        bytes_to_send);

#ifdef TEST_OPTIONS
  if (try_error == 2*send_order) {
    DEB(getId() << " not sending");
    return;
  }
#endif

  // OK, everything filled up. send the data
  assert(store_to_send >= 0 && store_to_send < n_output_stores);
  if (sendto(sockfd[send_order], output_store[store_to_send],
             bytes_to_send, 0, &(target_address_data.get),
             sizeof(target_address_data)) == -1) {
    perror("send");
  }
}

void IPAccessor::sendMyConfirm(const TimeSpec& ts)
{
#ifdef TEST_OPTIONS
  int try_error = int(random()/prob/(RAND_MAX+1.0));

  if (try_error == send_order) {
    int delay = 100000 + 10000*(random() % 8) +
      (send_order == 0 ? 900000 : 0);
    DEB(getId() << " extra confirm delay " << delay/1000
          << "ms, testing mode");
    usleep(delay);
  }
#endif

  // confirm buffer size
  char confirm_buffer[CONTROL_BYTES];

  control_info_store.acceptBuffer
    (confirm_buffer, CONTROL_BYTES);
  packData(control_info_store, uint16_t(send_order));
  packData(control_info_store, bool(false));   // confirmation phase
  packData(control_info_store, i_missed_data);
  packData(control_info_store, current_packet_no[send_order]);
  packData(control_info_store, uint16_t(0));

  if (i_missed_data) {
    /* DUECA network.

       Not all data arrive, sending a missed data flag in the confirm
       message. */
    W_NET(getId() << " sending missed data flag in confirm pack " <<
          (current_packet_no[send_order]/0x1000) << '/' <<
          (current_packet_no[send_order]&0xfff));
  }

  DEB("Sending confirm no " << (current_packet_no[send_order]/0x1000) <<
        '/' << (current_packet_no[send_order]&0xfff) << " sz=" <<
        CONTROL_BYTES);

#ifdef TEST_OPTIONS
  if (try_error == 2*send_order) {
    DEB(getId() << " not sending confirm");
    return;
  }
#endif

#ifdef LOG_COMMUNICATIONS
  if (log_communications) {
    commlog << "C "
            << setw(9) << ts.getValidityStart()
            << setw(3) << send_order <<  "            "
            << setw(3) << send_order
            << setw(2) << false
            << setw(2) << i_missed_data
            << setw(7) << (data_says_current_packet_no / 0x1000) << ':'
            << setfill('0') << setw(4) << (data_says_current_packet_no & 0xfff)
            << setfill(' ') << endl;
  }
#endif

  if (sendto(sockfd[send_order + no_of_senders],
             confirm_buffer, CONTROL_BYTES, 0,
             &(target_address_confirm.get),
             sizeof(target_address_confirm)) == -1)
    perror("send confirm");
}

bool IPAccessor::spinToLast(int sockno, char* buffer)
{
  DEB("Checking double messages from " << sockno);
  fd_set fdset; FD_ZERO(&fdset); FD_SET(sockfd[sockno], &fdset);
  int res;
#ifdef __QNXNTO__
  // on QNX (at least the 6.0 version), select does not quite do what
  // expected with very small (0) timeout values. It simply returns on
  // the timout, whether or not there is data, probably because of the
  // calls used to implement it. Therefore a slightly larger timeout
  // is used here.
  struct timeval timeout = {0, 1000};
#else
  struct timeval timeout = {0, 0};
#endif

  int newdata = 0;

  while ((res = select(sockfd[sockno] + 1, &fdset,
                       NULL, NULL, &timeout))) {
    assert(res >= 0);

    // there appears to be more data sent, read out this instead
    bytes = recv(sockfd[sockno], buffer, input_packet_size, MSG_TRUNC);
    if (bytes > input_packet_size) {
      /* DUECA network.

         A packet arrived from a peer that is too large for
         reading. Correct this in the DUECA configuration file.
      */
      E_NET(getId() << " cannot accomodate packet size " <<
            bytes << ", check dueca.cnf");
      CriticalActivity::criticalErrorNodeWide();
      prepareToStop();
      return false;
    }
    /* DUECA network.

       An additional message arrived. */
    W_NET("extra message " << ++newdata << " from " << sockno
          << ",bytes " << bytes);

    // reset the timeout and fd set, because they can be changed in
    // the select
#ifdef __QNXNTO__
    timeout.tv_sec = 0; timeout.tv_usec = 1000;
#else
    timeout.tv_sec = 0; timeout.tv_usec = 0;
#endif
    FD_ZERO(&fdset); FD_SET(sockfd[sockno], &fdset);
  }

  return newdata > 0;
}

bool IPAccessor::getControlData(int sender, char* buffer, const TimeSpec& ts)
{
  // check for the proper no of bytes
  if ((sender == 0 && bytes < TIMING_BYTES + CONTROL_BYTES) ||
      (sender != 0 && bytes < CONTROL_BYTES)) {

    // was there a receive error?
    if (bytes == -1) {
      /* DUECA network.

         Receive error on the network. */
      W_NET(getId() << "IPAccessor receive error, sender=" << sender);
      perror("error");
    }
    else {
      /* DUECA network.

         Small packet, is there another application sending on this
         address/port? */
      W_NET(getId() << "IPAccessor, packet too small, sender=" <<
            sender << " bytes=" << bytes);
    }

#ifdef LOG_COMMUNICATIONS
    if (log_communications) {
      commlog << "? "
              << setw(9) << ts.getValidityStart()
              << setw(3) << io_cycle_phase
              << setw(6) << bytes << endl;
    }
#endif
    return false;
  }


  // if sender == 0, get also the timing information
  if (sender == 0) {
    bytes -= TIMING_BYTES;
    time_info_restore.acceptBuffer(&buffer[bytes],
                                   TIMING_BYTES);
    time_info_restore.reUse(TIMING_BYTES);
    TimeTickType master_time(time_info_restore);
    int32_t offset_usecs(time_info_restore);

    // only process at my regular scheduled rate
    if (master_time % ts.getValiditySpan() == 0) {
      Ticker::single()->dataFromMaster(master_time, offset_usecs);
      DEB(getId() << "IPAccessor, time " << master_time << '+'
            << offset_usecs);
    }
  }

  // now the normal control information
  bytes -= CONTROL_BYTES;
  control_info_restore.acceptBuffer(&buffer[bytes],
                                    CONTROL_BYTES);
  control_info_restore.reUse(CONTROL_BYTES);
  unPackData(control_info_restore, data_says_send_number);
  if (data_says_send_number != io_cycle_phase) {
    /* DUECA network.

       Inconsistency between data origin and ip address */
    W_NET("Data says sender =" << data_says_send_number <<
          " while ipsource=" << sender <<
          " and cycle=" << io_cycle_phase);
  }
  unPackData(control_info_restore, data_says_data_sending_phase);
  unPackData(control_info_restore, data_says_i_missed_data);
  unPackData(control_info_restore, data_says_current_packet_no);
  unPackData(control_info_restore, data_says_regular_bytes);

#ifdef LOG_COMMUNICATIONS
  if (log_communications) {
    commlog << "R "
            << setw(9) << ts.getValidityStart()
            << setw(3) << sender
            << setw(6) << data_says_regular_bytes
            << setw(6) << bytes - data_says_regular_bytes
            << setw(2) << data_says_data_sending_phase
            << setw(2) << data_says_i_missed_data
            << setw(7) << (data_says_current_packet_no / 0x1000) << ':'
            << setfill('0') << setw(4) << (data_says_current_packet_no & 0xfff)
            << setfill(' ') << endl;
  }
#endif

  DEB("Sender " << data_says_send_number << " packet "
        << (data_says_current_packet_no/0x1000) << '/'
        << (data_says_current_packet_no&0xfff));
  return true;
}

bool IPAccessor::getDuecaData(const TimeSpec& time, MessageBuffer* buffer)
{
  // data_says_regular_bytes was already determined in code above?
  if (buffer->regular) {

    // offer the packet to the unpacker for further distribution
    DEB(getId() << " offering " << buffer->regular
          << " to unpacker");

    unpacker->acceptBuffer(buffer, time);
  }

  // excess bytes collected by fill packer
  if (fill_unpacker && buffer->regular < buffer->fill) {
    fill_unpacker->acceptBuffer(buffer, time);
  }

  return true;
}

void IPAccessor::prepareToStop()
{
  /* DUECA network.

     Planned stop of communication. */
  I_NET(getId() << " stopping communication");

  net_io.switchOff(TimeSpec(0,0));

  running = false;
}


bool IPAccessor::setDelayEstimator(ScriptCreatable &p, bool in)
{
  bool res = true;
  assert(in);
  TransportDelayEstimator *pnew =
    dynamic_cast<TransportDelayEstimator*> (&p);

  SCRIPTSTART_CHECK2(pnew != NULL, "object is not a delay estimator");
  SCRIPTSTART_CHECK2(tdelay_estimator == NULL,
                     "you already have a delay estimator");
  if (res) {
    tdelay_estimator = pnew;
    //  scheme_id.addReferred(&p);
  }
  return res;
}

bool IPAccessor::adjustPriority(const PrioritySpec &p)
{
  net_io.changePriority(p);
  return true;
}


DUECA_NS_END
