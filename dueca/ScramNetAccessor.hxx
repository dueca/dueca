/* ------------------------------------------------------------------   */
/*      item            : ScramNetAccessor.hxx
        made by         : Rene van Paassen
        date            : 010710
        category        : header file
        description     :
        changes         : 010710 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        api             : DUECA_API
*/

#ifndef ScramNetAccessor_hxx
#define ScramNetAccessor_hxx

#ifdef ScramNetAccessor_cxx
#endif

#include <ReflectiveAccessor.hxx>
#include <dueca_ns.h>
#include <StateGuard.hxx>

DUECA_NS_START
class Activitymanager;

/** This is a media accessor based on a common memory principle. This
    accessor uses SCRAMNet shared memory to communicate between
    several computers.

    A ScramNetAccessor is not normally called from a user program. Use
    the scheme construction capabilities to create it as follows:
    \code
    (dueca-list
     (make-scramnet-accessor
      no-of-nodes this-node-id ; no parties, my id
      "scram"                  ; name for the area
      1                        ; only used on QNX, Scramnet ID
      (* 1024 1024)            ; area size, 1M
      8000                     ; each block
      pkt                      ; packer
      upkt                     ; unpacker
      (make-time-spec 0 40)    ; time spec watcher
      (make-time-spec 0 tick-compatible-increment) ; time spec clock writer
      (make-priority-spec 2 0)  ; priority
      fpkt fupkt                ; fill packer, unpacker
     )
    )
    \endcode
*/
class ScramNetAccessor:
  public ReflectiveAccessor
{
  /** To guard the reading of a new interrupt location. */
  StateGuard int_read_guard;

  /** Location of a previously read and checked interrupt. */
  uint32_t waiting_interrupt;

  /// integer variant of the key value
  int ikey;

  /// Key value for accessing shared memory and message queue
  key_t key;

  /// id returned by the shmget call
  int   shm_id;

  /// id returned by the msgget call
  int   msg_id;

  /// process id of the process / thread handling the scramnet
  int   hand_pid;

  /// flag to remember whether attaching to scramnet signal has to be done
  bool  have_to_attach;

  /// flag to remember whether the system transferred to real-time operation
  bool  not_transferred;

  /// file descriptor for the scramnet interrupt device
  int   scr_int_fd;

  /// flag to communicate with the watchdog
  volatile bool worked;

  /// A counter for the stop work
  int stop_counter;

  /** Priority at which accessor runs. */
  PrioritySpec prio;

  /** Scramnet mode (direct 16 bit number) */
  uint32_t csr2_mode;

  /** a flag to notify about stop work. */
  volatile bool stopping;

  /// Pointer to the activity manager, for logging block/unblock
  ActivityManager* my_activity_manager;

private:

  /** Callback objects. \{ */
  Callback<ScramNetAccessor>         cb1, cb2, cb3, cb4; /// \}

  /** The activity that checks data when running non-real time. */
  ActivityCallback                   check;

  /** The activity that blocks, waiting for data. */
  ActivityCallback                   block;

  /** The activity that works as a watchdog, tickling when no data
      seems to come in. */
  ActivityCallback                   tickle;

  /** This activity is run when a stop of the system is planned, it
      checks for data but does not block. */
  ActivityCallback                   stopwork;

  /** This is the check on timing of the block activity. Block works
      at 0.1 second interval. Check that 0.17 / 0.27 second are not
      exceeded. */
  TimingCheck                        timing_check;

public:
  SCM_FEATURES_DEF;

  /** Obtain a pointer to the parameter table. */
  static const ParameterTable* getParameterTable();

  /** Constructor. This will be called from Scheme code. */
  ScramNetAccessor();

  /** complete method. */
  bool complete();

  /// Destructor
  ~ScramNetAccessor();

private:

  /** prepare to stop, override from Accessor base class. */
  void prepareToStop();

  /// method that checks incoming data, in case we are not running real-time
  void checkData(const TimeSpec &ts);

  /// method that blocks, waiting for data from the other side
  void blockForData(const TimeSpec& ts);

  /// method that checks comm, and tickles if needed
  void checkAndTickle(const TimeSpec& ts);

  /** a method that takes over the role of blockForData when the
      system is stopped. This does not block, and stops itself when
      all nodes are found to respond stopping. */
  void stopWork(const TimeSpec& ts);

  /** This encapsulates reading a new interrupt location from the
      card. */
  bool readInterrupt(uint32_t& location);

  /** This encapsulates reading a new interrupt location from the
      card. */
  bool checkInterrupt();

  /** write into a control area, and send the write action as a
      message. */
  void write(volatile uint32_t* address, uint32_t value);
};

DUECA_NS_END
#endif
