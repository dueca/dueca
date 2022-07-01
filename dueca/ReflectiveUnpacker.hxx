/* ------------------------------------------------------------------   */
/*      item            : ReflectiveUnpacker.hxx
        made by         : Rene van Paassen
        date            : 010711
        category        : header file
        description     :
        changes         : 010711 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectiveUnpacker_hxx
#define ReflectiveUnpacker_hxx

#ifdef ReflectiveUnpacker_cxx
#endif

#include <dueca/NamedObject.hxx>
#include <dueca/Activity.hxx>
#include <dueca/AsyncList.hxx>
#include <dueca_ns.h>
#include <dueca/ScriptCreatable.hxx>
#include <dueca/AperiodicAlarm.hxx>
#include <dueca/Callback.hxx>


DUECA_NS_START
class AmorphReStore;
class TimeSpec;
class PrioritySpec;
class StoreInformation;
class ChannelManager;
class UnifiedChannel;

/** Helper class, contains the information needed for an unpacking
    job. */
class HeadInfo
{
  /** Pointer to the channel, null if not present in this node. */
  UnifiedChannel* channel_ptr;

  /** Location of data. If NULL, the message was a notification and
      thereby self-contained. */
  uint32_t index;

  /** Size of data, or entry index */
  uint16_t size;

  /** Flag to remember whether message needs more data from buffer. */
  uint16_t entry;

  /** Send order of the message. */
  int sender;

public:
  /** Constructor. */
  HeadInfo(volatile uint32_t* area, uint32_t index,
           unsigned int area_size, int sender_no, int sender_id);

  /** Empty consturctor */
  HeadInfo();

  /** Returns true if channel end here ? */
  inline bool skipThisMessage() const { return channel_ptr == NULL; }

  /** Return the channel end pointer. */
  inline UnifiedChannel* getChannel() const { return channel_ptr; }

  /** Returns true if this is only a notification. */
  inline bool isNotification() const { return size == 0; }

  /** return size of message. */
  inline uint16_t getSize() const {return size; }

  /** return size of message. */
  inline uint16_t getEntry() const {return entry; }

  /** return index of data. */
  inline uint16_t getIndex() const {return index; }

  /** return sender of the data. */
  inline int getSender() const {return sender; }

  /** print to stream. */
  ostream& print(ostream& os) const;
};

inline ostream& operator << (ostream& os, const HeadInfo& hi)
{ return hi.print(os); }

/** This class can unpack data coming in on reflective or shared
    memory communication. */
class ReflectiveUnpacker:
  public ScriptCreatable,
  public NamedObject
{
  /** A device to keep a number of work descriptions on a rotating
      base. As new work comes in, a work description is added, and
      when work is done, the description is removed again. */
  AsyncList<HeadInfo> work;

  /** To keep track of what has been unpacked. */
  struct WorkDone {
    /** Points to the one who dropped this load. */
    uint16_t sender;
    /** Where in the buffer this work was to be done. */
    uint16_t index;
    /** Constructor. */
    WorkDone(uint16_t sender = 0, uint16_t index = 0) :
      sender(sender), index(index) { }
  };

  /** A list to keep track of what has been processed. */
  AsyncList<WorkDone> work_done;

  /** A type to keep indexes on the buffers. */
  struct Indexes {
    /** Point where the analysis stage is reading currently. */
    uint32_t stage1;
    /** Point up to where the unpacking stage has been commanded. */
    uint32_t stage2_commanded;
    /** Point up to where unpacking has progressed. */
    uint32_t stage2_done;

    Indexes() : stage1(0), stage2_commanded(0), stage2_done(0) { }
  };

  /** pointer to array of active indices in the buffers. */
  Indexes *indexes;

  /** To make these unpackers unique, an unpacker number is
      allocated. */
  static int unpacker_no;

  /** This is a pointer to an array of the control areas. */
  volatile uint32_t **area_cb;

  /** This is a pointer to an array of communication areas. */
  volatile uint32_t **area;

  /** Array of indices last written to. */
  uint32_t* last_write_idx;

  /** The number of senders in this communication scheme. */
  int no_parties;

  /** Id of this node in the reflective memory usage. */
  int reflect_node_id;

  /** Id of this node within dueca. */
  int dueca_node_id;

  /** And the size of each individual area. */
  unsigned int area_size;

  /** A buffer to move objects into, in case the input buffer wraps
      around, which an AmorphReStore object would not know what to do
      about. */
  uint32_t *object_buffer;

  /** Remember the size of this object buffer. */
  int object_buffer_size;

  /** Callback item */
  Callback<ReflectiveUnpacker> cb;

  /** Activity */
  ActivityCallback             run_unpack;

  /** Trigger puller for activation */
  AperiodicAlarm               waker;

public:
  /** Scheme connectivity. */
  SCM_FEATURES_DEF;

  /** Table with tunable parameters. */
  static const ParameterTable* getParameterTable();

public:
  /** Constructor. */
  ReflectiveUnpacker();

  /** Complete method. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Change priority specification. */
  bool setPrioritySpec(const PrioritySpec& ps);

  /** Destructor. */
  ~ReflectiveUnpacker();

  /** Return the object type */
  ObjectType getObjectType() const {return O_Dueca;}

  /** Initialise the communication buffers. This works differently
      from IP-based packers, but we are using the same interface.
      \param i   A StoreInformation object that contains the sizes and
                 locations of the stores, and the number of nodes
                 using the shared memory, and the node id of this node
                 in that number. */
  void initialiseStoresR(StoreInformation& i);

  /** Call from the accessor to pick up data from another node, and
      distribute it here.
      \param sending_node   Node where the data came from, and that
                            determines the place to look in the
                            communication buffer. */
  void distribute(int sending_node);

  /** Implement the action from my "Activity" base class. This does
      the actual unpacking. */
  void despatch(const TimeSpec& t);

private:
  /** Unpack the data in the given area. Not possible here, but given
      for call compatibility reasons. */
  inline void unpackData(volatile uint32_t *area, uint32_t read_idx,
                         UnifiedChannel* channel, int channel_id,
                         unsigned int size, int sender);

  /** Print to a stream. */
  friend ostream& operator << (ostream& os, const ReflectiveUnpacker& p);
};

DUECA_NS_END
#endif
