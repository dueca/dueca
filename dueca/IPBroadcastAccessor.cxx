/* ------------------------------------------------------------------   */
/*      item            : IPBroadcastAccessor.cxx
        made by         : Rene' van Paassen
        date            : 990611
        category        : body file
        description     :
        changes         : 990611 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define IPBroadcastAccessor_cc
#include "IPBroadcastAccessor.hxx"
#include "Packer.hxx"
#include "Unpacker.hxx"
#include "FillPacker.hxx"
#include "FillUnpacker.hxx"
#include "TimeSpec.hxx"
#include "TransportDelayEstimator.hxx"
#include "debug.h"

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
#endif /* __MINGW32__ */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif
#ifndef __MINGW32__
#include <cstring>
#endif
#include "dueca_assert.h"
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "MemberCall2Way.hxx"
#include "VarProbe.hxx"
#include "MemberCall.hxx"

DUECA_NS_START

const ParameterTable* IPBroadcastAccessor::getParameterTable()
{
  static const ParameterTable table[] = {
    { "packer", new MemberCall2Way<IPBroadcastAccessor,ScriptCreatable>
      (&IPBroadcastAccessor::setPacker),
      "packer that compacts to-be-transported data" },
    { "unpacker" , new MemberCall2Way<IPBroadcastAccessor,ScriptCreatable>
      (&IPBroadcastAccessor::setUnpacker),
      "unpacker that extracts data" },
    { "output-buffer-size", new VarProbe<IPBroadcastAccessor,int>
      (REF_MEMBER(&IPBroadcastAccessor::output_packet_size)),
      "maximum size, in bytes, of the send buffer" },
    { "no-output-buffers", new VarProbe<IPBroadcastAccessor,int>
      (REF_MEMBER(&IPBroadcastAccessor::n_output_stores)),
      "number of buffers for output. Minumum is 2, packer can fill one\n"
      "    and the accessor can send the other" },
    { "input-buffer-size", new VarProbe<IPBroadcastAccessor,int>
      (REF_MEMBER(&IPBroadcastAccessor::input_packet_size)),
      "maximum size, in bytes, of the send buffer" },
    { "no-input-buffers", new VarProbe<IPBroadcastAccessor,int>
      (REF_MEMBER(&IPBroadcastAccessor::n_input_stores)),
      "number of buffers for input. Note that you may need quite a few\n"
      "    e.g. with ten nodes, you fill nine buffers in one send cycle\n"
      "    if bulk unpacking takes place at a low priority, you might want\n"
      "    buffering for ten cycles, so 90 buffers" },
    { "mc-address", new VarProbe<IPBroadcastAccessor,vstring>
      (REF_MEMBER(&IPBroadcastAccessor::bc_address)),
      "Broadcast address to be used, e.g. 192.168.2.255" },
    { "port", new VarProbe<IPBroadcastAccessor,int>
      (REF_MEMBER(&IPBroadcastAccessor::base_port)),
      "base port number for the communication. Note that 2*n port numbers\n"
      "are needed, with n = number of parties" },
    { "if-address", new VarProbe<IPBroadcastAccessor,vstring>
      (REF_MEMBER(&IPBroadcastAccessor::if_address)),
      "IP address of the interface to use here" },
    { "timeout", new VarProbe<IPBroadcastAccessor,int>
      (REF_MEMBER(&IPBroadcastAccessor::timeout)),
      "timeout, in us, for the communication wait." },
    { "n-senders", new VarProbe<IPBroadcastAccessor,int>
      (REF_MEMBER(&IPBroadcastAccessor::no_of_senders)),
      "number of senders" },
    { "send-order", new VarProbe<IPBroadcastAccessor,int>
      (REF_MEMBER(&IPBroadcastAccessor::send_order)),
      "order of this node in the send cycle" },
    { "time-spec", new VarProbe<IPBroadcastAccessor,PeriodicTimeSpec>
      (REF_MEMBER(&IPBroadcastAccessor::time_spec)),
      "time specification for the send cycle" },
    { "priority", new MemberCall<IPBroadcastAccessor,PrioritySpec>
      (&IPBroadcastAccessor::adjustPriority),
      "Priority specification for the send/wait/receive activity" },
    { "delay-estimator",new MemberCall2Way<IPBroadcastAccessor,ScriptCreatable>
      (&IPBroadcastAccessor::setDelayEstimator),
      "supply an estimator for set-up and per-byte transmit delay" },
    { "fill-packer", new MemberCall2Way<IPBroadcastAccessor,ScriptCreatable>
      (&IPBroadcastAccessor::setFillPacker),
      "packer that compacts low-priority (excess bw) data" },
    { "fill-unpacker", new MemberCall2Way<IPBroadcastAccessor,ScriptCreatable>
      (&IPBroadcastAccessor::setFillUnpacker),
      "fill-unpacker that extracts low-prio data" },
#ifdef LOG_COMMUNICATIONS
    { "log-communications", new VarProbe<IPBroadcastAccessor,bool>
      (REF_MEMBER(&IPBroadcastAccessor::log_communications)),
      "Create log file with communication summary" },
#endif
#ifdef LOG_PACKING
    { "log-packing", new VarProbe<IPBroadcastAccessor,bool>
      (REF_MEMBER(&IPBroadcastAccessor::log_packing)),
      "Create log file with communication summary" },
#endif
    { NULL, NULL,
      "This is a DUECA communication media accessor, broadcast\n"
      "IP communication with several other DUECA nodes"}
  };
  return table;
}

IPBroadcastAccessor::
IPBroadcastAccessor() :
  IPAccessor()
{
  //
}

bool IPBroadcastAccessor::complete()
{
  // run complete method of parent
  bool res = IPAccessor::complete();

  // get host address
  in_addr host_address;
  SCRIPTSTART_CHECK2(inet_aton(if_address.c_str(), &host_address),
                     "own ip address not valid");

  if (!res) return false;

  // prepare receiving sockets
  unsigned short portno = base_port;
  for (int ii = 0; ii < 2 * no_of_senders; ii++ ) {

    if (ii == no_of_senders || ii == 2*no_of_senders - 1) {
      // no 0 and the last sender do not have separate confirmation
      // sockets
      sockfd[ii] = sockfd[ii - no_of_senders];
    }
    else if (ii == send_order || ii == send_order + no_of_senders) {
      // sending socket, either for data or confirmation

      // create the socket
      if ((sockfd[ii] = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        perror("Problem creating reception socket");

      // make it broadcast capable
      int on = 1;
      if (setsockopt(sockfd[ii], SOL_SOCKET,
                     SO_BROADCAST, &on, sizeof(on)) == -1)
        perror("Cannot add broadcast capability to the send socket");

      // prepare a source address. With this source address I can
      // select the interface to send this over
      union {
        struct sockaddr_in src_address;
        struct sockaddr    use_address;
      } addru;
      struct sockaddr_in src_address;
      memset(&addru, 0, sizeof(src_address));
      addru.src_address.sin_addr = host_address;
      addru.src_address.sin_family = AF_INET;
      addru.src_address.sin_port = 0;

      // bind the socket to the source address
      if (::bind(sockfd[ii], &addru.use_address, sizeof(addru)) != 0)
        perror("Cannot bind to source address");

      if (ii >= no_of_senders) {

        // the confirm target address is a broadcast address
        memset(&target_address_confirm, 0, sizeof(target_address_confirm));
        inet_aton(bc_address.c_str(), &target_address_confirm.set.sin_addr);
        target_address_confirm.set.sin_family = AF_INET;
        target_address_confirm.set.sin_port = htons(portno++);
      }
      else {

        // the data target address is a broadcast address
        memset(&target_address_data, 0, sizeof(target_address_data));
        inet_aton(bc_address.c_str(), &target_address_data.set.sin_addr);
        target_address_data.set.sin_family = AF_INET;
        target_address_data.set.sin_port = htons(portno++);
      }
    }
    else {
      // receiving socket

      // create the socket
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
      addru.recept_address.sin_addr.s_addr = INADDR_ANY;
      addru.recept_address.sin_family = AF_INET;
      addru.recept_address.sin_port = htons(portno++);

#ifdef TEST_OPTIONS
      // allow re-use of the socket
      int on = 1;
      if (setsockopt(sockfd[ii], SOL_SOCKET, SO_REUSEADDR,
                     (char*) &on, sizeof(on)) != 0)
        perror("Cannot set re-use");
#endif

      // bind the socket to the reception address
      if (::bind(sockfd[ii], &addru.use_address, sizeof(addru)) != 0)
        perror("Cannot bind to reception address");
    }
  }
  sockfd_high++;     // has to be one higher than highest found

  return res;
}

IPBroadcastAccessor::~IPBroadcastAccessor()
{
  //
}

DUECA_NS_END
