/* ------------------------------------------------------------------   */
/*      item            : FillUnpacker.hh
        made by         : Rene' van Paassen
        date            : 001024
        category        : header file
        description     :
        changes         : 001024 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef FillUnpacker_hh
#define FillUnpacker_hh

#include "NamedObject.hxx"
#include "Activity.hxx"
#include "AmorphStore.hxx"
#include "AsyncList.hxx"
#include "ScriptCreatable.hxx"
#include "MessageBuffer.hxx"
#include "Callback.hxx"
#include "AperiodicAlarm.hxx"
#include <dueca-conf.h>
#include "dueca_ns.h"
#include "varvector.hxx"

DUECA_NS_START
class ChannelManager;
class PrioritySpec;
class Accessor;

/** Unpacks low-priority (bulk) messages.
    A FillUnpacker unpacks the low-priority bulk messages that are
    sent in the space left over by the normal messaging. This space is
    filled at the sending end by a FillPacker. A FillPacker message
    block may be divided over several normal messages, the
    FillUnpacker is called with the function changeCurrentStore when a
    message comes in with this data. It then schedules itself to be
    run in the requested priority (usually level 0), where it accesses
    the store and re-assembles the pieces found in the stores. It then
    proceeds to unpack data. */
class FillUnpacker:
  public ScriptCreatable,
  public NamedObject,
  public TriggerPuller
{
  /** remember hop many senders */
  int no_of_senders;

  /** Array of pointer to all the stores used by the media accessor. */
  //char** store;

  /** Status of each of these stores. */
  //int* store_status;

  /** Array of buffers in which to copy the data, and from which to
      unpack the stuff for the channels. One buffer for each "other"
      sender. */
  char** buffer;

  /** Array of restore objects that work on the buffers. */
  AmorphReStore *amorph_store;

  /** Check counter for verifying that messages are in order */
  dueca::varvector<uint32_t> countcheck;

#ifdef FILLPACKER_SEND_ID
  /** Counter for the number of packages sent, in debug mode */
  dueca::varvector<uint32_t> pkg_count;

  /** And sender id */
  dueca::varvector<int>      senderid;
#endif

  /** Remember the number of unpackers made, so each can have a unique
      name. */
  static int unpacker_no;

  /** List of all the work still to do. */
  AsyncList<MessageBuffer::ptr_type> work;

  /** Size of each of the unpacking buffers. */
  unsigned buffer_size;

  /** IP accessor */
  Accessor* accessor;

  /** Callback item */
  Callback<FillUnpacker>       cb;

  /** Activity */
  ActivityCallback             run_unpack;

public:
  /** This class can be created from scheme */
  SCM_FEATURES_DEF;

  /** Return parameter table. */
  static const ParameterTable* getParameterTable();

public:
  /** Constructor, normally called from scheme. */
  FillUnpacker();

  /** Change priority specification. */
  bool setPrioritySpec(const PrioritySpec& ps);

  /** complete method. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Destructor. */
  ~FillUnpacker();

  /** Return the type of object, a part of DUECA. */
  ObjectType getObjectType() const;

  /** Initialise the unpacker with the information necessary to access
      the stores of the communication accessor.
      \param send_order Order of this node in sending.
      \param no_of_senders Total number of senders. For each sender
                        except this sender we will need a buffer for
                        unpacking the (potentially large) objects sent
                        by bulk mail. */
  void initialiseStores(int send_order,  int no_of_senders);

  /** Method, re-implemented from my activity base class, to do the
      actual unpacking work. */
  void despatch(const TimeSpec& t);

  /** Called by the communications accessor to indicate that I (may)
      have to unpack the data from another store. */
  void changeCurrentStore(int store_no, unsigned int start_from,
                          unsigned int fill_level, int sender,
                          const TimeSpec& ts);

  /** Called by the IPAcessor, to indicate a new buffer needs to be unpacked
      @param buffer      Pointer to the buffer
      @param ts          Time specification
  */
  void acceptBuffer(MessageBuffer::ptr_type buffer, const TimeSpec&ts);

  /** Remember accessor */
  inline void setAccessor(Accessor* ac) {accessor = ac;}

};

DUECA_NS_END
#endif
