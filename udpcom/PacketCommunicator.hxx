/* ------------------------------------------------------------------   */
/*      item            : PacketCommunicator.hxx
        made by         : Rene van Paassen
        date            : 200405
        category        : header file
        description     :
        changes         : 200405 first version
        language        : C++
        copyright       : (c) 2020 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef PacketCommunicator_hxx
#define PacketCommunicator_hxx

#include <string>
#include <utility>

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <dueca_ns.h>
#include <dueca/AsyncQueueMT.hxx>
#include <dueca/CommonCallback.hxx>
#include <dueca/MessageBuffer.hxx>
#include <dueca/SharedPtrTemplates.hxx>
#include <extra/ConglomerateFactory.hxx>

DUECA_NS_START;

/** Specification for all needed information for the different
    packet communicators */
struct PacketCommunicatorSpecification
{
  /** Address of communication endpoint */
  std::string url;

  /** Desired size of buffers */
  uint32_t buffer_size;

  /** Number of buffers */
  uint32_t nbuffers;

  /** Timeout value, [s] */
  double timeout;

  /** Node ID for sending/communicating */
  uint32_t peer_id;

  /** Interface address, if relevant */
  std::string interface_address;

  /** Port re-use, for specific (UDP) communication */
  bool port_re_use;

  /** Low delay TOS setting, for UDP communication */
  bool lowdelay;

  /** Socket priority for sending */
  int                                 socket_priority;

  /** Server key, if using ssl connection */
  std::string server_key;

  /** Certificate, if using ssl connection */
  std::string server_crt;

  /** The callback pointer */
  CommonCallbackBase<void,MessageBuffer::ptr_type>::smart_ptr_type
                     callback;

  /** Constructor */
  PacketCommunicatorSpecification();
};


/** Packet oriented communication mechanism.

    This communication mechanism is currently implemented over UDP,
    point-to-point, broadcast or multicast, or using WebSocket */
class PacketCommunicator
INHERIT_REFCOUNT(PacketCommunicator)
{
  INCLASS_REFCOUNT(PacketCommunicator);

  /** Free storage for received messages */
  AsyncQueueMT<MessageBuffer::ptr_type> messagebuffers;

  /** Size of each buffer */
  const size_t                     buffersize;

protected:
  /** Number of received bytes in the last transaction */
  ssize_t                          latest_received;

  /** Peer id of the last transaction */
  int                              latest_peer;

  /** Flag to know if passing the data */
  bool                             pass_data;

  /** Node id for the purpose of sending/communicating */
  const unsigned                   peer_id;

  /** Operational flag, reset when errors occur */
  bool                             is_operational;

  /** Callback object */
  const CommonCallbackBase
  <void, MessageBuffer::ptr_type>::smart_ptr_type callback;

protected:
  /** Obtain a new or recycled buffer */
  MessageBuffer::ptr_type getBuffer();

public:
  /** Constructor */
  PacketCommunicator(const PacketCommunicatorSpecification& spec);

  /** Destructor */
  virtual ~PacketCommunicator();

  /** interface for sending */
  virtual void send(MessageBuffer::ptr_type buffer) = 0;

  /** interface for receiving, data will be returned through a callback */
  virtual std::pair<int,ssize_t> receive() = 0;

  /** flush initial messages (if needed, e.g., for udp) */
  virtual void flush();

  /** Simulate failing receive */
  inline void setFailReceive() { pass_data = false; }

  /** Return one of the message buffers */
  void returnBuffer(MessageBuffer::ptr_type buffer);

  /** Is operational ?*/
  virtual bool isOperational();
};

/** Key object for factory specification */
struct PacketCommunicatorKey
{
  /** Communicator types are indexed by string value */
  typedef const std::string Key;
  /** Result produced by factory */
  typedef boost::intrusive_ptr<PacketCommunicator> ProductBase;
  /** Specification information for different communicators */
  typedef PacketCommunicatorSpecification SpecBase;
};

/** Type definition/implementation of the factory */
typedef
CFSingletonWrapper<
  ConglomerateFactory<
    PacketCommunicatorKey,
    SubcontractorBase<PacketCommunicatorKey>* > >
PacketCommunicatorFactory;


DUECA_NS_END;

#endif
