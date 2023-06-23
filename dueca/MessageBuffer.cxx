/* ------------------------------------------------------------------   */
/*      item            : MessageBuffer.cxx
        made by         : Rene' van Paassen
        date            : 161203
        category        : body file
        description     :
        changes         : 161203 first version
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        language        : C++
*/

#define MessageBuffer_cxx
#include "MessageBuffer.hxx"
#include "DAtomics.hxx"
#include "debug.h"
#include <iostream>
#include <dassert.h>
#include <exception>
#include <algorithm>

DUECA_NS_START;

#define DEBPRINTLEVEL -1
#include "debprint.h"

struct bufferboundaryexception: public std::exception
{
  const char * what() const noexcept
  { return "attempt to write past buffer size"; }
};

unsigned MessageBuffer::creation_count = 0;

MessageBuffer::MessageBuffer(size_t size, size_t offset) :
  capacity(size),
  offset(offset),
  regular(0),
  fill(0),
  nusers(1), // implicit claim
  origin(0),
  message_cycle(0U),
  buffer(new char[capacity]),
  creation_id(creation_count++)
{
  DEB("MessageBuffer, creating id=" << creation_id << " size " << size);
}

MessageBuffer& MessageBuffer::operator=(const MessageBuffer& o)
{
  if (this != &o) {
    if (this->capacity < o.fill) {
      throw(bufferboundaryexception());
    }
    this->offset = o.offset;
    this->regular = o.regular;
    this->fill = o.fill;
    this->origin = o.origin;
    this->message_cycle = o.message_cycle;
    std::copy(o.buffer, o.buffer + o.fill, this->buffer);
  }

  return *this;
}

void  MessageBuffer::reset()
{
  // capacity does not change
  this->offset = 0;
  this->regular = 0;
  this->fill = 0;
  this->nusers = 1;
  this->message_cycle = 0;
  this->offset = 0;
}

MessageBuffer::~MessageBuffer()
{
  if (nusers) {
    /* DUECA network.

       A buffer for data transmission is being deleted while it is
       flagged as being still in use. This can produce crashes or
       undetermined results. */
    W_NET("Warning, deleting a MessageBuffer that is in use");
  }
  delete [] buffer;
  DEB("MessageBuffer, deleting id=" << creation_id);
}

void MessageBuffer::claim()
{
  atomic_increment32(nusers);
  DEB1("buffer " << creation_id << " cycle " << message_cycle <<
       " claim, new nusers = " << nusers);
  //std::atomic_fetch_add(&nusers, 1U);
}

bool MessageBuffer::release()
{
  uint32_t left = atomic_decrement32(nusers);

  // test if fully released
  if (left) {
    DEB1("buffer " << creation_id << " cycle " << message_cycle
         << " release, new nusers = " << nusers);
    return false;
  }

  // reset use
  regular = fill = 0U;
  DEB1("buffer " << creation_id << " cycle " << message_cycle << " released");
  return true;
}

void MessageBuffer::write(const char* data, std::size_t dsize)
{
  if (fill + dsize <= capacity) {
    memcpy(&buffer[fill], data, dsize);
    fill += dsize;
  }
  else {
    throw bufferboundaryexception();
  }
}

void MessageBuffer::zeroUnused()
{
  if (fill < capacity) {
    memset(&buffer[fill], 0, capacity - fill);
  }
}

MessageBuffer::Iterator::Iterator(const char *val) : m_ptr(val) {} 
MessageBuffer::Iterator::Iterator(const Iterator& other) : m_ptr(other.m_ptr) {}
MessageBuffer::Iterator& MessageBuffer::Iterator::operator=(const Iterator& other)
{
  if (&other != this) this->m_ptr = other.m_ptr;
  return *this;
}
MessageBuffer::Iterator::~Iterator() {}

DUECA_NS_END;
