/* ------------------------------------------------------------------   */
/*      item            : Accessor.hxx
        made by         : Rene van Paassen
        date            : 020417
        category        : header file
        description     :
        changes         : 020417 first version
                          171226 made more general, for interface with NetAccessor
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Accessor_hxx
#define Accessor_hxx


#include "ScriptCreatable.hxx"
#include <NamedObject.hxx>
#include <dueca_ns.h>
#include <fstream>
#include "AsyncQueueMT.hxx"
#include "MessageBuffer.hxx"
#include <oddoptions.h>

DUECA_NS_START

class Packer;
class Unpacker;
class FillPacker;
class FillUnpacker;

/** A base class for all objects that access communication devices for
    communication between nodes. */
class Accessor:
  public ScriptCreatable,
  public NamedObject
{
protected:
  /** The object that fills the data storage that has to be sent. */
  Packer        *packer;

  /** The object that unpacks the data that has come in. */
  Unpacker      *unpacker;

  /** If there is any space left, the fill packer might fill it with
      low-priority data. */
  FillPacker    *fill_packer;

  /** Unpacks the data sent by other fill packers. */
  FillUnpacker  *fill_unpacker;

  /** Packet size for buffers */
  int            input_packet_size;

private:
  /** Offset buffer data */
  size_t         input_offset;

  /** Free storage for received messages */
  AsyncQueueMT<MessageBuffer::ptr_type> input_stores;

protected:
  /// constructor
  Accessor(const NameSet& name,
           size_t input_packet_size = 2048, size_t input_offset = 0);

public:
  /// destructor
  virtual ~Accessor();

protected:
  /** Type name information */
  virtual const char* getTypeName();

  /** Being warned by the packer that we are going to stop. */
  virtual void prepareToStop() = 0;

  /** The only one who may call prepareToStop. */
  friend class GenericPacker;

  /** Add a packer to the class. */
  bool setPacker(ScriptCreatable &p, bool in);

  /** Add a fill packer to the class. */
  bool setFillPacker(ScriptCreatable &p, bool in);

  /** Add an unpacker to the class. */
  bool setUnpacker(ScriptCreatable &p, bool in);

  /** Add a fill packer to the class. */
  bool setFillUnpacker(ScriptCreatable &p, bool in);

public:
  /** Return one of the message buffers */
  virtual void returnBuffer(MessageBuffer::ptr_type buffer);

protected:
  /** Obtain a new or recycled buffer */
  MessageBuffer::ptr_type getBuffer();

#ifdef LOG_COMMUNICATIONS
  /** communications log switch */
  bool log_communications;

  /** File for messages */
  std::ofstream commlog;
#endif

#ifdef LOG_PACKING
  /** packing log switch */
  bool log_packing;

  /** File for messages */
  std::ofstream packlog;

public:
  /** Switch to see if packing log requested */
  bool getLogPacking() { return log_packing; }

  /** File for packing log */
  std::ofstream& getPackLog() { return packlog; }


private:
#endif

public:
  /** Open the log file(s), if applicable */
  bool complete();

  /// This is a scheme-level callable class
  SCM_FEATURES_DEF;
};

/** Return the (python) script name for this object */
template<> const char* core_creator_name<Accessor>(const char*);

DUECA_NS_END
#endif
