/* ------------------------------------------------------------------   */
/*      item            : ShmAccessor.hxx
        made by         : Rene van Paassen
        date            : 010710
        category        : header file
        description     :
        changes         : 010710 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ShmAccessor_hxx
#define ShmAccessor_hxx

#ifdef ShmAccessor_cxx
#endif

#include <ReflectiveAccessor.hxx>
#include <dueca_ns.h>

DUECA_NS_START
class Activitymanager;

/** This is a media accessor based on a common memory principle. This
    accessor uses shared memory on 1 computer, with SysV shared memory
    and SysV message communication. The purpose of this accessor is
    development of media accessors of this kind, since it is generally
    not very useful to run multiple dueca processes on one node. */
class ShmAccessor:
  public ReflectiveAccessor
{
  /// integer variant of the key value
  int ikey;

  /// Key value for accessing shared memory and message queue
  key_t key;

  /// id returned by the shmget call
  int   shm_id;

  /// id returned by the msgget call
  int   msg_id;

  /** Counter that is used when stopping the communication. */
  int   stop_counter;

  /** Priority at which manager runs. */
  PrioritySpec prio;

  /// Pointer to the activity manager, for logging block/unblock
  ActivityManager* my_activity_manager;

private:

  /** Callback objects. */
  Callback<ShmAccessor>              cb1, cb2;

  /** The activity that blocks, waiting for data. */
  ActivityCallback                   block;

  /** The activity that closes off the communication. */
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
  ShmAccessor();

  /** complete method. */
  bool complete();

  /// Destructor
  ~ShmAccessor();

private:
  /** prepare to stop. */
  void prepareToStop();

  /// method that blocks, waiting for data from the other side
  void blockForData(const TimeSpec& ts);

  /** The method that does data communication in the stopping
      phase. */
  void stopWork(const TimeSpec& ts);

public:
  /// write into a control area, and send the write action as a message.
  void write(volatile uint32_t* address, uint32_t value);
};

DUECA_NS_END
#endif
