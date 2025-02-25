/* ------------------------------------------------------------------   */
/*      item            : SegmentedRecorderBase.hxx
        made by         : Rene van Paassen
        date            : 250225
        category        : header file
        description     :
        changes         : 250225 first version
        language        : C++
        copyright       : (c) 2025 DUECA Authors
*/

#pragma once

#include <dueca_ns.h>
#include <ddff/FileStreamWrite.hxx>
#include <ddff/FileHandler.hxx>
#include <boost/intrusive_ptr.hpp>
#ifdef HAVE_BOOST_SMART_PTR_INTRUSIVE_REF_COUNTER_HPP
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#endif

DDFF_NS_START

/** Base class for segmented recording. Handles marking of recording stretches,
   and callback to the writing stream to inform on segment-data links */
class SegmentedRecorderBase :
  public boost::intrusive_ref_counter<SegmentedRecorderBase>
{

protected:
  /** Remember to where data was written/handled */
  TimeTickType marked_tick;

  /** Control indicating the start of a recording stretch */
  TimeTickType record_start_tick;

  /** Flag to remember data written during a stretch of recording */
  bool dirty;

  /** file stream for writing */
  ddff::FileStreamWrite::pointer w_stream;

public:
  /** Constructor */
  SegmentedRecorderBase();

  /** Destructor */
  ~SegmentedRecorderBase();

  /** Starting a new stretch; will mark the first data written in this
  stretch (if any) for callback with the offset of that data */
  inline void startStretch(TimeTickType tick)
  {
    record_start_tick = tick;
  }

  /** Check and possibly reset the dirty flag.

      @returns true if already clean (meaning that tag offset can stay
               0), otherwise returns false, and tag offset should
               have a value.
  */
  bool checkAndMakeClean();

  /** Initiate syncing of the data to disk */
  void syncRecorder();

  /** Check write status, before forcing a flush of the file.

      @param  tick   End time for which writing should be complete.
      @returns       "true", if written until tick.
   */
  inline bool checkWriteTick(TimeTickType tick)
  {
    return tick <= marked_tick;
  }

  /** Control spooling replay position.

      @param offset     Location in file where data starts
      @param end_offset Location in file where data ends.
   */
  virtual void spoolReplay(ddff::FileHandler::pos_type offset,
                           ddff::FileHandler::pos_type end_offset);

    /** Starting a new replay; provide offset for the replayed data */
  virtual void startReplay(TimeTickType tick);
};

DDFF_NS_END