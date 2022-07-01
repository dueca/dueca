/* ------------------------------------------------------------------   */
/*      item            : ActivityWeaver.hh
        made by         : Rene' van Paassen
        date            : 000908
        category        : header file
        description     :
        changes         : 000908 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ActivityWeaver_hh
#define ActivityWeaver_hh

#include <stringoptions.h>
#include <vector>
#include "TimeSpec.hxx"
#include "ActivityLister.hxx"

#include <dueca_ns.h>
DUECA_NS_START

class ActivityBit;
class ActivityWeaver;
struct ActivityLog;
class WeaverKeyInvalid;
struct ActivityDescription;

/** Objects that accept and process ActivityLog objects.

    ActivityManager objects can produce, upon request, a log of their
    activities. These logs are packed in events and sent as
    ActivityLog objects. The ActivityWeaver can be queried for
    completeness of the logs, and when complete, can be requested to
    produce an ActivityLister object, which can be used to list all
    these actions together and give a verbal report, or all the
    actions for one ActivityManager/thread, useful for drawing a time
    line. An ActivityWeaver integrates the logs from all
    ActivityManagers in a DUECA node */

class ActivityWeaver
{
  /** contains the logs from the ActivityManagers. */
  vector<const ActivityLog*> current_logs;

  /** The number of logs/ActivityManagers. */
  int no_of_logs;

  /** A flag to remember whether a consistent set of logs is present. */
  bool logs_complete;

  /** the key that is valid for the current set of logs.
      When new logs come in, a new key is chosen. This is used to
      prevent accesses from obsolete ActivityLister */
  unsigned int current_key;

  /** Node number I am waving for. */
  int node;

public:
  /** Constructor. Is empty to allow array allocation. */
  ActivityWeaver();

  /** Destructor */
  ~ActivityWeaver();

  /** This "swallows" a log from an ActivityManager. */
  void includeLog(const ActivityLog* log);

  /** Reset log validity. */
  void resetLogs();

  /** starts a traversal of all logs.
      This returns an ActivityLister object that can be used with
      stepToNext and reportVerbal/reportBit to evaluate the log. focus
      -1 means no special emphasis on a log, with a zero or positive
      focus value, one only get the starts and possible interruptions
      of a specific ActivityManager */
  ActivityLister startInvestigation(int focus = -1) const;

  /** Query completeness and consistency of the logs. */
  bool areTheLogsComplete();

  /** Get the number of logs. */
  int getNumLevels() { return current_logs.size(); }

  /** Set node number. */
  void setNode(int node) { this->node = node; }

  /** spool to the beginning of the log for ActivityManager i.
      This skips the LogStart ActivityBit */
  const ActivityBit* spoolLog(int i, uint32_t key) const;

  /** Returns the description of activity number i */
  const ActivityDescription& getActivityDescription(int i) const;

  /** Returns the scale factor that convert a tick fraction to
      \f$\mu\f$ secs. */
  double getScale() const;

  /** Returns the begin tick value of the log. */
  TimeTickType getOffset() const;

  /** Check the validity of a key */
  bool checkValidity(uint32_t key) const;
};

DUECA_NS_END
#endif
