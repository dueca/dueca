/* ------------------------------------------------------------------   */
/*      item            : SegmentedRecorderBase.cxx
        made by         : Rene' van Paassen
        date            : 250225
        category        : body file
        description     :
        changes         : 250225 first version
        language        : C++
        copyright       : (c) 25 TUDelft-AE-C&S
*/

#include "SegmentedRecorderBase.hxx"
#include "DDFFExceptions.hxx"
#include <dueca/TimeSpec.hxx>

DDFF_NS_START

SegmentedRecorderBase::SegmentedRecorderBase() :
  boost::intrusive_ref_counter<SegmentedRecorderBase>(),
  marked_tick(0U),
  record_start_tick(MAX_TIMETICK),
  dirty(false),
  w_stream()
{
  //
}

SegmentedRecorderBase::~SegmentedRecorderBase() {}

void SegmentedRecorderBase::spoolReplay(ddff::FileHandler::pos_type offset,
                                        ddff::FileHandler::pos_type end_offset)
{
  throw replay_not_implemented();
}

void SegmentedRecorderBase::startReplay(TimeTickType tick)
{
  throw replay_not_implemented();
}

void SegmentedRecorderBase::syncRecorder()
{
  if (dirty) {
    w_stream->closeOff();
  }
}

bool SegmentedRecorderBase::checkAndMakeClean()
{
  if (dirty) {
    dirty = false;
    return false;
  }
  return true;
}

DDFF_NS_END