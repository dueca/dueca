/* ------------------------------------------------------------------   */
/*      item            : Accessor.cxx
        made by         : Rene' van Paassen
        date            : 020417
        category        : body file
        description     :
        changes         : 020417 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Accessor_cxx
#include "Accessor.hxx"
#include <dueca/Packer.hxx>
#include <dueca/Unpacker.hxx>
#include <dueca/FillPacker.hxx>
#include <dueca/FillUnpacker.hxx>
#include <dueca/debug.h>
#include <dueca/PythonCorrectedName.hxx>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

Accessor::Accessor(const NameSet& name,
                   size_t input_packet_size, size_t input_offset) :
  NamedObject(name),
  packer(NULL),
  unpacker(NULL),
  fill_packer(NULL),
  fill_unpacker(NULL),
  input_packet_size(input_packet_size),
  input_offset(input_offset),
  input_stores(20, "UDP spare message stores")
#ifdef LOG_COMMUNICATIONS
  ,log_communications(false)
#endif
#ifdef LOG_PACKING
  ,log_packing(false)
#endif
{
  //
}

bool Accessor::complete()
{
#ifdef LOG_COMMUNICATIONS
  if (log_communications) {
    commlog.open("dueca.commlog", ios_base::trunc);
  }
#endif
#ifdef LOG_PACKING
  if (log_packing) {
    packlog.open("dueca.packlog", ios_base::trunc);
  }
#endif
  packer->setAccessor(this);
  unpacker->setAccessor(this);
  fill_packer->setAccessor(this);
  fill_unpacker->setAccessor(this);

  return true;
}

const char* Accessor::getTypeName()
{
  return "Accessor";
}

Accessor::~Accessor()
{
  while  (input_stores.notEmpty()) {
    AsyncQueueReader<MessageBuffer::ptr_type> r(input_stores);
    delete r.data();
  }
}

bool Accessor::setPacker(ScriptCreatable &p, bool in)
{
  bool res = true;
  assert(in);
  Packer *pnew =  dynamic_cast<Packer*> (&p);
  SCRIPTSTART_CHECK2(pnew != NULL, "object is not a packer");
  SCRIPTSTART_CHECK2(packer == NULL, "you already have a packer");
  if (res) {
    packer = pnew;
    //scheme_id.addReferred(&p);
  }
  return res;
}

bool Accessor::setFillPacker(ScriptCreatable &p, bool in)
{
  bool res = true;
  assert(in);
  FillPacker *pnew =  dynamic_cast<FillPacker*> (&p);
  SCRIPTSTART_CHECK2(pnew != NULL, "object is not a fill packer");
  SCRIPTSTART_CHECK2(fill_packer == NULL, "you already have a fill packer");
  if (res) {
    fill_packer = pnew;
    //scheme_id.addReferred(&p);
  }
  return res;
}

bool Accessor::setUnpacker(ScriptCreatable &p, bool in)
{
  bool res = true;
  assert(in);
  Unpacker *pnew =  dynamic_cast<Unpacker*> (&p);
  SCRIPTSTART_CHECK2(pnew != NULL, "object is not an unpacker");
  SCRIPTSTART_CHECK2(unpacker == NULL, "you already have an unpacker");
  if (res) {
    unpacker = pnew;
    //scheme_id.addReferred(&p);
  }
  return res;
}

bool Accessor::setFillUnpacker(ScriptCreatable &p, bool in)
{
  bool res = true;
  assert(in);
  FillUnpacker *pnew = dynamic_cast<FillUnpacker*> (&p);
  SCRIPTSTART_CHECK2(pnew != NULL, "object is not an unpacker");
  SCRIPTSTART_CHECK2(fill_unpacker == NULL,
                     "you already have a fill unpacker");
  if (res) {
    fill_unpacker = pnew;
    //scheme_id.addReferred(&p);
  }
  return res;
}

void Accessor::returnBuffer(MessageBuffer::ptr_type buffer)
{
  assert(buffer->nusers);
  if (buffer->release()) {
    {
      AsyncQueueWriter<MessageBuffer::ptr_type> w(input_stores);
      w.data() = buffer;
    }
    DEB("buffer returned " << input_stores.size());
  }
}

MessageBuffer::ptr_type Accessor::getBuffer()
{
  MessageBuffer::ptr_type buf;

  // possible because only reading from one thread
  if (input_stores.notEmpty()) {
    {
      AsyncQueueReader<MessageBuffer::ptr_type> r(input_stores);
      buf = r.data();
    }
    DEB("buffer reused " << input_stores.size());
  }
  else {
    /* DUECA network.

       Because unpacking buffers have not yet been returned and data
       keeps coming in, an additional reception buffer needs to be
       created. Check priority assignments if this is very common, and
       ensure the unpackers get scheduled to unpack the data.
    */
    W_NET("Need to create additional reception buffer");
    buf = new MessageBuffer(input_packet_size, input_offset);
  }

  buf->nusers = 1;
  return buf;
}

template<> const char* core_creator_name<Accessor>(const char* given)
{
  if (given != NULL) {
    static PythonCorrectedName name(given);
    return name.c_str();
  }
  return "Accessor";
}

DUECA_NS_END
