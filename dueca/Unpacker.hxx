/* ------------------------------------------------------------------   */
/*      item            : Unpacker.hh
        made by         : Rene' van Paassen
        date            : 990615
        category        : header file
        description     :
        changes         : 990615 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        doc             :
*/

#ifndef Unpacker_hh
#define Unpacker_hh

#ifdef Unpacker_cc
#endif

#include <dueca/Activity.hxx>
#include <dueca/NamedObject.hxx>
#include <dueca/AsyncList.hxx>
#include <dueca/dueca_ns.h>
#include <dueca/ScriptCreatable.hxx>
#include <dueca/MessageBuffer.hxx>
#include <dueca/Callback.hxx>
#include <dueca/AperiodicAlarm.hxx>

DUECA_NS_START

class AmorphReStore;
class TimeSpec;
class PrioritySpec;
struct ParameterTable;
class Accessor;

/** This is a class of object that collaborates with media accessors,
    and gets passed buffers with data that has to be unpacked in the
    present node. */
class Unpacker:
  public ScriptCreatable,
  public NamedObject,
  public TriggerPuller
{
protected:

  /** Array of stores which will be used for unpacking the data. */
  AmorphReStore* store;

  /** Status of all stores, kept to keep a tab on packing/unpacking,
      and to know whether things are running ok. */
  int* store_status;

  /** Number of stores in use. The IPAccessor will tell me how many
      there are. */
  unsigned int no_of_stores;

  /** Counter, so all unpackers can have an individual name. */
  static int unpacker_no;

  /** Pointer to my ip accessor */
  Accessor *accessor;

  /** A device to keep a number of work descriptions on a rotating
      base. As new work comes in, a work description is added, and
      when work is done, the description is removed again. */
  AsyncList<MessageBuffer::ptr_type> work;

  /** Callback item */
  Callback<Unpacker>           cb;

  /** Activity */
  ActivityCallback             run_unpack;

public: // scheme connectivity
  SCM_FEATURES_DEF;

  /** return the table with configuration parameters. */
  static const ParameterTable* getParameterTable();

public:

  /** No arguments constructor. */
  Unpacker();

  /** Complete method. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Change priority specification. */
  bool setPrioritySpec(const PrioritySpec& ps);

  /** Destructor. */
  virtual ~Unpacker();

  /** Report about the type of object. */
  ObjectType getObjectType() const;

  /** Called by the IPAccessor, to tell me where the data will be
      landing. */
  void initialiseStores();

  /** Re-defined from the Activity, does the unpacking. The despatch
      also takes a work description out of the list, to know which
      store to unpack. */
  virtual void despatch(const TimeSpec& t);

  /** Called by the IPAccessor, tells me that a new store has to be
      unpacked.
      \param store_no    Number of the store.
      \param fill_level  How much data is in store.
      \param ts          Time specification, from the receiving
                         process. */
  int changeCurrentStore(int store_no, unsigned int fill_level,
                         const TimeSpec& ts);

  /** Called by the IPAcessor, to indicate a new buffer needs to be unpacked
      @param buffer      Pointer to the buffer
      @param offset      Where to start reading in the buffer
      @param size        Size of data
      @param tick        Time specification
  */
  void acceptBuffer(struct MessageBuffer* buffer, size_t offset, size_t size,
                    TimeTickType tick);

  /** Called by the IPAcessor, to indicate a new buffer needs to be unpacked
      @param buffer      Pointer to the buffer
      @param ts          Time specification
  */
  void acceptBuffer(struct MessageBuffer* buffer, const TimeSpec&ts);

  /** Print to stream. */
  friend ostream& operator << (ostream& os, const Unpacker& p);

  /** Remember accessor */
  inline void setAccessor(Accessor* ac) {accessor = ac;}
};

DUECA_NS_END
#endif
