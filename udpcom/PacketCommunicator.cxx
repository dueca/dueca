/* ------------------------------------------------------------------   */
/*      item            : PacketCommunicator.cxx
        made by         : Rene' van Paassen
        date            : 200405
        category        : body file
        description     :
        changes         : 200405 first version
        language        : C++
        copyright       : (c) 2020 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define PacketCommunicator_cxx
#include "PacketCommunicator.hxx"
#include <boost/serialization/singleton.hpp>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;

CODE_REFCOUNT(PacketCommunicator)

PacketCommunicatorSpecification::
PacketCommunicatorSpecification() :
  url(""),
  buffer_size(2048),
  nbuffers(3),
  timeout(2.0),
  peer_id(0),
  interface_address(""),
  port_re_use(false),
  lowdelay(true),
  socket_priority(6),
  server_key(""),
  server_crt(""),
  callback()
{ }


PacketCommunicator::
PacketCommunicator(const PacketCommunicatorSpecification& spec) :
#if USING_BOOST_INHERIT == 0
  intrusive_refcount(0),
#endif
  messagebuffers(spec.nbuffers, "Packet spare message buffers"),
  buffersize(spec.buffer_size),
  pass_data(true),
  peer_id(spec.peer_id),
  is_operational(false),
  callback(spec.callback)
{
  for (int ii = spec.nbuffers; ii--; ) {
    returnBuffer(new MessageBuffer(buffersize));
  }
}

PacketCommunicator::~PacketCommunicator()
{
  DEB("Destructor PacketCommunicator");//
}

MessageBuffer::ptr_type PacketCommunicator::getBuffer()
{
  MessageBuffer::ptr_type buf;
  if (messagebuffers.notEmpty()) {
    AsyncQueueReader<MessageBuffer::ptr_type> r(messagebuffers);
    buf = r.data();
  }
  else {
    DEB("Creating extra message buffer for receive");
    buf = new MessageBuffer(buffersize);
  }
  buf->nusers = 1;
  return buf;
}


void PacketCommunicator::returnBuffer(MessageBuffer::ptr_type buffer)
{
  assert(buffer->nusers);
  if (buffer->release()) {
    AsyncQueueWriter<MessageBuffer::ptr_type> w(messagebuffers);
    w.data() = buffer;
  }
}

bool PacketCommunicator::isOperational()
{
  return is_operational;
}

void PacketCommunicator::flush()
{
  // default action none
}
DUECA_NS_END;
