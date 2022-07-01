/* ------------------------------------------------------------------   */
/*      item            : IPMulticastAccessor.cxx
        made by         : Rene' van Paassen
        date            : 990611
        category        : body file
        description     :
        changes         : 990611 first version
                          061006 changed to use CoreCreator
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define IPMulticastAccessor_cc

#include <dueca-conf.h>

#include "IPMulticastAccessor.hxx"
#include "Packer.hxx"
#include "Unpacker.hxx"
#include "FillPacker.hxx"
#include "FillUnpacker.hxx"
#include "TimeSpec.hxx"
#include "ObjectManager.hxx"
#include "NameSet.hxx"
#include "TransportDelayEstimator.hxx"
#include "Environment.hxx"
#include "ActivityManager.hxx"

#include <fstream>
#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock2.h>
#include "inet_aton.hxx"
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#include <netinet/tcp.h>
#endif
#include <netdb.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef __MINGW32__
#include <cstring>
#endif
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif
#include "dueca_assert.h"
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "MemberCall2Way.hxx"
#include "VarProbe.hxx"
#include "MemberCall.hxx"

#include <debug.h>


DUECA_NS_START


const ParameterTable* IPMulticastAccessor::getParameterTable()
{
  static const ParameterTable table[] = {
    { "packer", new MemberCall2Way<IPMulticastAccessor,ScriptCreatable>
      (&IPMulticastAccessor::setPacker),
      "packer that compacts to-be-transported data" },
    { "unpacker" , new MemberCall2Way<IPMulticastAccessor,ScriptCreatable>
      (&IPMulticastAccessor::setUnpacker),
      "unpacker that extracts data" },
    { "output-buffer-size", new VarProbe<IPMulticastAccessor,int>
      (REF_MEMBER(&IPMulticastAccessor::output_packet_size)),
      "maximum size, in bytes, of the send buffer" },
    { "no-output-buffers", new VarProbe<IPMulticastAccessor,int>
      (REF_MEMBER(&IPMulticastAccessor::n_output_stores)),
      "number of buffers for output. Minumum is 2, packer can fill one\n"
      "    and the accessor can send the other" },
    { "input-buffer-size", new VarProbe<IPMulticastAccessor,int>
      (REF_MEMBER(&IPMulticastAccessor::input_packet_size)),
      "maximum size, in bytes, of the send buffer" },
    { "no-input-buffers", new VarProbe<IPMulticastAccessor,int>
      (REF_MEMBER(&IPMulticastAccessor::n_input_stores)),
      "number of buffers for input. Note that you may need quite a few\n"
      "e.g. with ten nodes, you fill nine buffers in one send cycle\n"
      "if bulk unpacking takes place at a low priority, you might want\n"
      "buffering for ten cycles, so 90 buffers" },
    { "mc-address", new VarProbe<IPMulticastAccessor,vstring>
      (REF_MEMBER(&IPMulticastAccessor::mc_address)),
      "Multicast address to be used, e.g. 224.0.0.1" },
    { "port", new VarProbe<IPMulticastAccessor,int>
      (REF_MEMBER(&IPMulticastAccessor::base_port)),
      "base port number for the communication. Note that 2*n port numbers\n"
      "are needed, with n = number of parties" },
    { "if-address", new VarProbe<IPMulticastAccessor,vstring>
      (REF_MEMBER(&IPMulticastAccessor::if_address)),
      "IP address of the interface to use here" },
    { "timeout", new VarProbe<IPMulticastAccessor,int>
      (REF_MEMBER(&IPMulticastAccessor::timeout)),
      "timeout, in us, for the communication wait." },
    { "n-senders", new VarProbe<IPMulticastAccessor,int>
      (REF_MEMBER(&IPMulticastAccessor::no_of_senders)),
      "number of senders" },
    { "send-order", new VarProbe<IPMulticastAccessor,int>
      (REF_MEMBER(&IPMulticastAccessor::send_order)),
      "order of this node in the send cycle" },
    { "time-spec", new VarProbe<IPMulticastAccessor,PeriodicTimeSpec>
      (REF_MEMBER(&IPMulticastAccessor::time_spec)),
      "time specification for the send cycle" },
    { "priority", new MemberCall<IPMulticastAccessor,PrioritySpec>
      (&IPMulticastAccessor::adjustPriority),
      "Priority specification for the send/wait/receive activity" },
    { "delay-estimator",new MemberCall2Way<IPMulticastAccessor,ScriptCreatable>
      (&IPMulticastAccessor::setDelayEstimator),
      "supply an estimator for set-up and per-byte transmit delay" },
    { "fill-packer", new MemberCall2Way<IPMulticastAccessor,ScriptCreatable>
      (&IPMulticastAccessor::setFillPacker),
      "packer that compacts low-priority (excess bw) data" },
    { "fill-unpacker", new MemberCall2Way<IPMulticastAccessor,ScriptCreatable>
      (&IPMulticastAccessor::setFillUnpacker),
      "fill-unpacker that extracts low-prio data" },
    { "port-re-use", new VarProbe<IPMulticastAccessor,bool>
      (REF_MEMBER(&IPMulticastAccessor::port_re_use)),
      "Enable port re-use, only necessary in specific configurations where\n"
      "multiple DUECA nodes run on one physical computer" },
#ifdef LOG_COMMUNICATIONS
    { "log-communications", new VarProbe<IPMulticastAccessor,bool>
      (REF_MEMBER(&IPMulticastAccessor::log_communications)),
      "Create log file with communication summary" },
#endif
#ifdef LOG_PACKING
    { "log-packing", new VarProbe<IPMulticastAccessor,bool>
      (REF_MEMBER(&IPMulticastAccessor::log_packing)),
      "Create log file with communication summary" },
#endif
    { NULL, NULL,
      "This is a DUECA communication media accessor, multicast\n"
      "IP communication with several other DUECA nodes"}
  };
  return table;
}

IPMulticastAccessor::
IPMulticastAccessor() :
  IPAccessor(),
  base_port(-1),
#ifdef TEST_OPTIONS
  port_re_use(true)
#else
  port_re_use(false)
#endif
{
  //
}

bool IPMulticastAccessor::complete()
{
  // run complete method of parent
  bool res = IPAccessor::complete();

  // get host address
  in_addr host_address;
  SCRIPTSTART_CHECK2(inet_aton(if_address.c_str(), &host_address),
                     "own ip address not valid");

  // prepare the multicast reception request
  struct ip_mreq mreq;
  memset(&mreq, 0, sizeof(mreq));
  SCRIPTSTART_CHECK2(inet_aton(mc_address.c_str(),
                               &mreq.imr_multiaddr),
                     "multicast ip address not valid");
  mreq.imr_interface = host_address;

  // check that the base port number is OK
  SCRIPTSTART_CHECK2(base_port > 0 && base_port < 0x10000 - 2 * no_of_senders,
                     "base port range not valid");

  if (!res) return false;

  // prepare all sockets
  // socket layout:
  // 0           : sending socket for master
  // 1 .. n - 2  : data only sending for followers
  // n - 1       : data + confirm for last one in chain
  // n           : unused (no confirm master)
  // n+1 .. 2n-2 : confirm for followers 1 .. n-2
  // 2n-1        : unused (no separate confirm last one in chain)
  for (int ii = 0; ii < 2 * no_of_senders; ii++ ) {

    unsigned short portno = base_port + ii;

    if (ii == no_of_senders ||         // skip conf socket 0
        ii == 2*no_of_senders - 1) {   // and of last sender

      // this socket no's are totally unused, for sending and for receiving
      sockfd[ii] = -1;
    }
    else if (ii == send_order || ii == send_order + no_of_senders) {

      // sending sockets, data or confirmation

      // create the socket
      if ((sockfd[ii] = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Problem creating reception socket");
        res = false;
      }

      // prepare a source address. With this source address I can
      // select the interface to send this over
      union {
        struct sockaddr_in src_address;
        struct sockaddr    use_address;
      } addru;
      memset(&addru, 0, sizeof(addru));
      addru.src_address.sin_addr = host_address;
      addru.src_address.sin_family = AF_INET;
      addru.src_address.sin_port = 0;

      if (port_re_use) {
        // allow re-use of the socket
        /* DUECA network.

           For information. A socket/port for multicast has been
           flagged for re-use. This is usual only for testing, or for
           running multiple DUECA processes on a single computer. */
        I_NET("selecting re-use on the socket");
        int on = 1;
        if (setsockopt(sockfd[ii], SOL_SOCKET, SO_REUSEADDR,
                       (char*) &on, sizeof(on)) != 0) {
          perror("Cannot set re-use");
          res = false;
        }
      }

      // bind the socket to the source address
      if (::bind(sockfd[ii], &addru.use_address,
               sizeof(addru)) != 0) {
        perror("Cannot bind to source address");
        res = false;
      }

      if (ii >= no_of_senders) {

        // create a target multicast address for the confirm cycle
        memset(&target_address_confirm, 0, sizeof(target_address_confirm));
        inet_aton(mc_address.c_str(), &target_address_confirm.set.sin_addr);
        target_address_confirm.set.sin_family = AF_INET;
        target_address_confirm.set.sin_port = htons(portno);
      }
      else {

        // create a target address for the data cycle
        memset(&target_address_data, 0, sizeof(target_address_data));
        inet_aton(mc_address.c_str(), &target_address_data.set.sin_addr);
        target_address_data.set.sin_family = AF_INET;
        target_address_data.set.sin_port = htons(portno);
      }

      // request multicast, with this socket and interface
      if (setsockopt(sockfd[ii], IPPROTO_IP, IP_MULTICAST_IF,
                     &mreq.imr_interface, sizeof(in_addr)))
        perror("Problem selecting interface");

      if (!port_re_use) {
        // slightly more efficient if loopback on this address is avoided
        u_char loop = '\000';
        if (setsockopt(sockfd[ii], IPPROTO_IP, IP_MULTICAST_LOOP,
                       &loop, sizeof(loop))) {
          perror("Trying to disable loop");
        }
      }

    }
    else if ((ii < no_of_senders)    // receive all data sockets
             || (send_order == 0)    // 0 reads all used sockets
             || (ii - no_of_senders == send_order + 1)) {
      // receive socket for confirm just ahead of me

      // create a receiving socket
      if ((sockfd[ii] = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        perror("Problem creating reception socket");

      // remember which is the highest numbered socket
      if (sockfd[ii] > sockfd_high) sockfd_high = sockfd[ii];

      // prepare the reception addresses
      union {
        struct sockaddr_in recept_address;
        struct sockaddr use_address;
      } addru;
      memset(&addru, 0, sizeof(addru));
      inet_aton(mc_address.c_str(), &addru.recept_address.sin_addr);
      addru.recept_address.sin_family = AF_INET;
      addru.recept_address.sin_port = htons(portno);

      if (port_re_use) {
        // allow re-use of the socket
        /* DUECA network.

           For information. A socket/port for multicast has been
           flagged for re-use. This is usual only for testing, or for
           running multiple DUECA processes on a single computer. */
        I_NET("selecting re-use on the socket");
        int on = 1;
        if (setsockopt(sockfd[ii], SOL_SOCKET, SO_REUSEADDR,
                       (char*) &on, sizeof(on)) != 0)
          perror("Cannot set re-use");
      }

      // add multicast membership
      if (setsockopt(sockfd[ii], IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     (char*) &mreq, sizeof(mreq)) != 0)
        perror("Cannot configure multicast membership");

      // bind the socket to the reception address
      if (::bind(sockfd[ii], &addru.use_address, sizeof(addru)) != 0)
        perror("Cannot bind to reception address");

    }
  }
  sockfd_high++;      // has to be one higher than highest found

  return res;
}

IPMulticastAccessor::~IPMulticastAccessor()
{
  //
}


DUECA_NS_END
