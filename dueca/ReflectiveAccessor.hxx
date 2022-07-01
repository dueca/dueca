/* ------------------------------------------------------------------   */
/*      item            : ReflectiveAccessor.hxx
        made by         : Rene van Paassen
        date            : 010420
        category        : header file
        description     : Base class for media accessors that use a shared
                          memory range
                          NOTE: this header may not be installed!
        changes         : 010420 first version
                          141113 adaptation new activation and communication
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectiveAccessor_hxx
#define ReflectiveAccessor_hxx

#ifdef ReflectiveAccessor_cxx
#endif

#include <Accessor.hxx>
#include <EventAccessToken.hxx>
#include <Activity.hxx>
#include <Callback.hxx>
#include <inttypes.h>
#include <stringoptions.h>

#include <dueca_ns.h>
DUECA_NS_START
class ReflectivePacker;
class ReflectiveFillPacker;
class ReflectiveUnpacker;
class ReflectiveFillUnpacker;

/** ReflectiveAccessor is a media accessors based on some shared
    memory mechanism.

    This uses a shared memory area, e.g. implemented in SCRAMNet
    reflective memory, to communicate between DUECA nodes. The memory
    is set-up as follows, here "no_parties" is the number of nodes in
    the communication.

    <ol>

    <li> no_parties (4 byte) words, one for each participant in the
         communication. These words are used at initialisation of the
         communication, and after initialisation, for writing tick
         counters, which are then used to check all nodes are alive,
         and for synchronisation. Tick counters are written at the
         compatible-tick rate.

    <li> no_parties areas, each of size commbuf_size + no_parties + 1
         (in words), each area starts with a control buffer of
         no_parties words, a node identification with the dueca node
         id, 1 word, and then a communication buffer of commbuf_size
         words, which will be used as a circular buffer.
    </ol> */
class ReflectiveAccessor :
  public Accessor
{
protected:
  /** This defined the communication state, and the logic in
      connecting to the other nodes. */
  enum ComState {
    Operational,   /**< All in working mode. */
    Contact1,      /**< Node 0 has changed state on the net. */
    Contact2,      /**< Second check. */
    Contact3,      /**< Third check. */
    Unknown        /**< It is not known whether other nodes have
                        initialised communication. */
  };

  /** Flag to keep track of start-up communication. */
  ComState cstate;
private:

  /// print a comstate to stream
  friend ostream& operator << (ostream& os, const ComState& o);

  /** A sequence number, so different accessors get unique names. */
  static int sequence;

public:
  SCM_FEATURES_DEF;

protected:
  /** identification of the reflective area. */
  vstring reflect_area_id;

  /** \name Intermediaries
      Intermediaries between the media and dueca */
  //@{
  /** The object that "packs" data into the shared memory area. */
  ReflectivePacker* packer;

  /** The object that unpacks data from the shared memory area. */
  ReflectiveUnpacker* unpacker;

  /** The object that "packs" low prio data and indirectly sends it
      via a normal channel. Sending size and speed are limited. */
  ReflectiveFillPacker* fill_packer;

  /** The object that unpacks low prio bulk data */
  ReflectiveFillUnpacker* fill_unpacker;
  //@}

  /** \name Access to the shared memory */
  //@{
  /** The index of "me" in the shared memory communication group. */
  int my_index;

  /** The total number of participants in this shared memory
      communication group. */
  unsigned int no_parties;

  /** Size of the event and stream notification buffers. Each member
      in this group has a buffer for placing data change notifications
      and for transporting event data. This is the size of this
      buffer, in bytes, as declared in this node's dueca.cnf */
  uint32_t commbuf_size;

  /** Time period for watching and checking communications. */
  TimeSpec watchtime;

  /** Time period for writing sync updates. */
  TimeSpec clocktime;

  /** Pointer to the start of the area. */
  volatile uint32_t *area_start;

  /** Pointers to all notification/communication buffers control parts */
  volatile uint32_t **area_cb;

  /** Pointers to all communication buffers. */
  volatile uint32_t **area;

  /** Total size of the area. */
  unsigned int area_size;
  //@}

  /** Countdown for init messages. */
  int countdown;

  /** Reset value for the countdown. */
  int countdown_init;

  /** Information about timing speed. */
  unsigned int counts_in_second;

  /** Flag to remember the initialisation of packer and unpacker. */
  bool helpers_initialised;

  /** Flag to remember starting the packer and unpacker. */
  bool helpers_started;

  /** Flag that tells me whether the clock writing has been started. */
  bool have_to_start_clock;

  /** A flag that tells me whether the timing pulses are quick
      enough. */
  volatile bool timing_ok;

  /** Flag to decide whether shared memory should be usable by stream
      channels. */
  bool direct_comm_allowed;

  /** Copy of writer indexes, used for recovery in case strange
      offsets are received. */
  uint32_t *write_index_copy;

private:
  /** An event access token for sending communication area information
      to the main ChannelManager. */
  /** Callback object 1 */
  Callback<ReflectiveAccessor>              cb1;

  /** Callback object 2 */
  Callback<ReflectiveAccessor>              cb2;

  /** An activity that acts as a low-priority watchdog. This will also
      send the information about the reflective memory availability
      for stream channels. */
  ActivityCallback                          watchdog;

protected:
  /** An activity that will write timing information for the other
      nodes. Node 0's clock will be the master, the other nodes simply
      write confirmation, to enable timing and safety checks. */
  ActivityCallback                         clockwrite;

private:
  /// To check for sanity of communications
  bool line_ok;

  /// Is asserted whenever something comes in
  bool data_received;

protected:
  /** Constructor. Can only be called from child class. */
  ReflectiveAccessor();

  /** Calculate correctness of parameters after constructor. */
  bool complete();

  /** Further specify the accessor, by initialising the area */
  void initialiseArea(volatile uint32_t* area_start);

  /** Do the initial write. */
  void startContact();

  /// Destructor.
  virtual ~ReflectiveAccessor();

  /** Handle an incoming control write */
  void handleControlWrite(uint32_t offset, const TimeSpec& ts);

  /** Accept a packer. */
  bool setPacker(ScriptCreatable& p, bool in);

  /** Accept an unpacker. */
  bool setUnpacker(ScriptCreatable& p, bool in);

  /** Accept a fill packer. */
  bool setFillPacker(ScriptCreatable& p, bool in);

  /** Accept an fill unpacker. */
  bool setFillUnpacker(ScriptCreatable& p, bool in);

private:
  /** Check validity of communications. */
  void watchTheLine(const TimeSpec &ts);

  /** Write the new clock tick to the net. */
  void writeClock(const TimeSpec &ts);

  /** Initialise the packer. */
  void initialiseHelpers();

  /** Initialise and start the packer and the unpacker. */
  void startHelpers();

  /** The following function is to be called by a ReflectivePacker. */
  friend class ReflectivePacker;

public:
  /** Write a ReflectiveAccessor to a stream. Mainly for debugging
      purposes. */
  friend ostream& operator << (ostream& os, const ReflectiveAccessor& r);

  /** Return the object type, indicates that this is an object
      belonging to dueca itself. */
  ObjectType getObjectType() const {return O_CommAccessor;};

  /** explicitly write a value at a location in the buffer. */
  virtual void write(volatile uint32_t* address, uint32_t value) = 0;

};

DUECA_NS_END
#endif





