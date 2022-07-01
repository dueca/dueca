/* ------------------------------------------------------------------   */
/*      item            : IPTwoWay.cxx
        made by         : Rene' van Paassen
        date            : 001102
        category        : body file
        description     :
        changes         : 001102 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include "GenericPacker.hxx"
#include "Unpacker.hxx"
#include "FillPacker.hxx"
#include "FillUnpacker.hxx"
#include "TransportDelayEstimator.hxx"

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

#define IPTwoWay_cc
#include "IPTwoWay.hxx"
#include "dueca_assert.h"
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "MemberCall2Way.hxx"
#include "VarProbe.hxx"
#include "MemberCall.hxx"

DUECA_NS_START

const ParameterTable* IPTwoWay::getParameterTable()
{
  static const ParameterTable table[] = {
    { "packer", new MemberCall2Way<IPTwoWay,ScriptCreatable>
      (&IPTwoWay::setPacker),
      "packer that compacts to-be-transported data" },
    { "unpacker" , new MemberCall2Way<IPTwoWay,ScriptCreatable>
      (&IPTwoWay::setUnpacker),
      "unpacker that extracts data" },
    { "output-buffer-size", new VarProbe<IPTwoWay,int>
      (REF_MEMBER(&IPTwoWay::output_packet_size)),
      "maximum size, in bytes, of the send buffer" },
    { "no-output-buffers", new VarProbe<IPTwoWay,int>
      (REF_MEMBER(&IPTwoWay::n_output_stores)),
      "number of buffers for output. Minumum is 2, packer can fill one\n"
      "    and the accessor can send the other" },
    { "input-buffer-size", new VarProbe<IPTwoWay,int>
      (REF_MEMBER(&IPTwoWay::input_packet_size)),
      "maximum size, in bytes, of the send buffer" },
    { "no-input-buffers", new VarProbe<IPTwoWay,int>
      (REF_MEMBER(&IPTwoWay::n_input_stores)),
      "number of buffers for input. Note that you may need quite a few\n"
      "    e.g. with ten nodes, you fill nine buffers in one send cycle\n"
      "    if bulk unpacking takes place at a low priority, you might want\n"
      "    buffering for ten cycles, so 90 buffers" },
    { "ip-address", new VarProbe<IPTwoWay,vstring>
      (REF_MEMBER(&IPTwoWay::ip_address)),
      "IP address of the host at the other end" },
    { "port", new VarProbe<IPTwoWay,int>
      (REF_MEMBER(&IPTwoWay::base_port)),
      "base port number for the communication. Note that 2 ports are needed" },
    { "if-address", new VarProbe<IPTwoWay,vstring>
      (REF_MEMBER(&IPTwoWay::if_address)),
      "IP address of the interface to use here" },
    { "timeout", new VarProbe<IPTwoWay,int>
      (REF_MEMBER(&IPTwoWay::timeout)),
      "timeout, in us, for the communication wait." },
    { "send-first", new MemberCall<IPTwoWay,bool>
      (&IPTwoWay::selectSendFirst),
      "if true, this node sends first" },
    { "one-way", new MemberCall<IPTwoWay,bool>
      (&IPTwoWay::selectOneWay),
      "if true, only sending, not receiving" },
    { "time-spec", new VarProbe<IPTwoWay,PeriodicTimeSpec>
      (REF_MEMBER(&IPTwoWay::time_spec)),
      "time specification for the send cycle" },
    { "priority", new MemberCall<IPTwoWay,PrioritySpec>
      (&IPTwoWay::adjustPriority),
      "Priority specification for the send/wait/receive activity" },
    { "delay-estimator", new MemberCall2Way<IPTwoWay,ScriptCreatable>
      (&IPTwoWay::setDelayEstimator),
      "supply an estimator for set-up and per-byte transmit delay" },
    { "fill-packer", new MemberCall2Way<IPTwoWay,ScriptCreatable>
      (&IPTwoWay::setFillPacker),
      "packer that compacts low-priority (excess bw) data" },
    { "fill-unpacker", new MemberCall2Way<IPTwoWay,ScriptCreatable>
      (&IPTwoWay::setFillUnpacker),
      "fill-unpacker that extracts low-prio data" },
    { "n-senders", new VarProbe<IPTwoWay,int>
      (REF_MEMBER(&IPTwoWay::no_of_senders)),
      "number of senders, alternative formulation to one-way" },
    { "send-order", new VarProbe<IPTwoWay,int>
      (REF_MEMBER(&IPTwoWay::send_order)),
      "order of this node in the send cycle, alternative formulation to\n"
      "one-way parameter" },
#ifdef LOG_COMMUNICATIONS
    { "log-communications", new VarProbe<IPTwoWay,bool>
      (REF_MEMBER(&IPTwoWay::log_communications)),
      "Create log file with communication summary" },
#endif
#ifdef LOG_PACKING
    { "log-packing", new VarProbe<IPTwoWay,bool>
      (REF_MEMBER(&IPTwoWay::log_packing)),
      "Create log file with communication summary" },
#endif
    { NULL, NULL,
      "This is a DUECA communication media accessor, using point-to-point\n"
      "IP communication with another DUECA node"}
  };
  return table;
}

IPTwoWay::IPTwoWay() :
  IPAccessor()
{
  // be sure that the default set-up matches
  send_order = 0;
  no_of_senders = 2;
}


bool IPTwoWay::complete()
{
  // run complete method of parent
  bool res = IPAccessor::complete();

  // add own checks, max senders is 2 with this method
  SCRIPTSTART_CHECK(no_of_senders <= 2);

  // get host address
  in_addr host_address;
  SCRIPTSTART_CHECK2(inet_aton(if_address.c_str(), &host_address),
                     "own ip address not valid");

  if (!res) return false;

  int myself = send_order;
  int other = 1 - send_order;

  // create receiving socket
  if (no_of_senders > 1) {
    if ((sockfd[other] = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
      perror("Problem creating reception socket");
      res = false;
    }

    sockfd_high = sockfd[other] + 1;

    // prepare a reception address
    union {
      struct sockaddr_in recept_address;
      struct sockaddr    use_address;
    } addru;

    memset(&addru, 0, sizeof(addru));
    addru.recept_address.sin_addr = host_address;
    addru.recept_address.sin_port = htons(base_port + other);
    addru.recept_address.sin_family = AF_INET;

    // bind the socket to that reception address
    if (::bind(sockfd[other], &addru.use_address,
             sizeof(addru)) != 0) {
      perror("Cannot bind to reception address");
      res = false;
    }
  } // if (two_way)

  // complete the socket array
  sockfd[2]=-1;
  sockfd[3]=-1;

  // create the send socket
  if ((sockfd[myself] = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Problem creating sender socket");
    res = false;
  }

  // prepare my own "source" address
  struct sockaddr_in src_address;
  memset(&src_address, 0, sizeof(src_address));
  src_address.sin_addr = host_address;
  src_address.sin_family = AF_INET;
  src_address.sin_port = 0;

  // prepare the target address
  memset(&target_address_data, 0, sizeof(target_address_data));
  SCRIPTSTART_CHECK2(inet_aton(ip_address.c_str(),
                               &target_address_data.set.sin_addr),
                     "peer ip address not valid");
  target_address_data.set.sin_family = AF_INET;
  target_address_data.set.sin_port = htons(base_port + myself);

  return res;
}

IPTwoWay::~IPTwoWay()
{
  //
}

bool IPTwoWay::selectSendFirst(const bool& first)
{
  send_order = first ? 0 : 1;
  return true;
}

bool IPTwoWay::selectOneWay(const bool& oneway)
{
  no_of_senders = oneway ? 1 : 2;
  return true;
}

DUECA_NS_END
