/* ------------------------------------------------------------------   */
/*      item            : ActivityLister.hh
        made by         : Rene' van Paassen
        date            : 20001125
        category        : header file
        description     :
        changes         : 20001125 Rvp first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ActivityLister_hh
#define ActivityLister_hh

#include <stringoptions.h>
#include <vector>
#include "TimeSpec.hxx"
#include <iostream>

#include <dueca_ns.h>
DUECA_NS_START

class ActivityBit;
class ActivityWeaver;
struct ActivityLog;
class WeaverKeyInvalid;

/** An ActivityLine gives instructions to plot a single activity log
    item. */
struct ActivityLine
{
  /** Activity type. */
  enum Type {
    Blank,   /**< nothing, done. */
    Run,     /**< a process running. */
    Block,   /**< blocked on input/time. */
    Graphics /**< update of graphics. */
  };

  /** Type of activity. */
  Type type;

  /** Scaled start of activity. */
  int x0;

  /** Scaled end of activity. */
  int x1;

public:
  /** Constructor. */
  ActivityLine(Type t, int x0, int x1);
};

/** An ActivityStrings object gives a list of five strings with the
    description of an activity. */
struct ActivityStrings
{
  /** Integer tick value of the activity. */
  char tick[13];

  /** offset, in ms, from start of log. */
  char offset[9];

  /** time specification of the activity. */
  char ts[15];

  /** duration, in ms, of activity. */
  char dt[8];

  /** name of the module. */
  char module[65];

  /** name for the activity. */
  char activity[34];

  /** List with character pointers. */
  char* strlist[6];

  /** Return strings in an array. Note: single thread! */
  char **str();

  /** Constructor. */
  ActivityStrings();

  /** Copy constructor. */
  ActivityStrings(const ActivityStrings& as);

private:
  /** Assignment. */
  //ActivityStrings& operator = (const ActivityStrings& as);
};

/** print a set of activity strings. */
std::ostream& operator << (std::ostream& os, const ActivityStrings& as);

/** An ActivityLister object can read out the log in an ActivityWeaver.

    An ActivityLister object acts as a key for a client that wants to
    traverse the data managed by an ActivityWeaver. The creation of a
    lister initiates traversal, and the reportVerbal or reportWithBit
    methods return the data of the next action in the log. */
class ActivityLister
{
  /** The ActivityWeaver that this ActivityLister has been made by/for. */
  const ActivityWeaver* weaver;

  /** The ActivityBit log pieces from all ActivityManagers.
      As a log is traversed, the ActivityLister spools down all the
      different ActivityBits, these are the ones currently under
      attention */
  vector<const ActivityBit*> bit_index;

  /** Indicates which ActivityManagers were active (but possibly
      interrupted). */
  vector<bool> activity_on;

  /** Indexes for traversing the log. */
  int current_locus;

  /** Indexes for traversing the log. */
  int locus_changing_locus;

  /** Pointer to the bit that changes the locus of activity. */
  const ActivityBit* locus_changing_bit;

  /** Focus determines a "preference" for one of the ActivityManager's
      logs. */
  int focus;

  /** The key given to this lister, I am valid as long as this matches
      the ActivityWeaver's key. */
  int match_key;

  /** Default type of string to return when no data found. */
  ActivityStrings as_nodata;

public:
  /** constructor.
      Accepts 4 parameters,
      \param weaver   a pointer to the weaver holding the logs.
      \param sources  number of sources (logs) to mix.
      \param focus    the source to focus on (or -1 if focusing on
                      all).
      \param match_key a key that is given by the ActivityWeaver */
  ActivityLister(const ActivityWeaver* weaver,
                 int sources, int focus, int match_key);

  /** destructor */
  ~ActivityLister();

public:
  /** gives a string describing the next item */
  vstring reportVerbal();

  /** returns the relevant ActivityBit */
  pair<int,int> reportWithBit(const ActivityBit*& bit);

  /** Return a line piece for plotting.
      \param prio        Priority level to plot.
      \param tick_start  Start tick of plot.
      \param tick_end    End tick of plot.
      \param winwidth    width of plot window, in pixels. */
  ActivityLine getNextActivity(int prio, float tick_start,
                               float tick_end, unsigned winwidth);

  /** Return a string description of the activity, for in the selected
      action list.
      \param prio        Priority level to return.
      \param tick_start  Start time for the list
      \param tick_end    End time for the list
      \param more        Boolean, is set to false when no more data. */
  ActivityStrings getNextActivityDesc(int prio, float tick_start,
                                      float tick_end, bool& more);

  /** Return maximum priority. */
  inline int getMaxPrio() const { return bit_index.size(); }

private:
  /** Find the next relevant bit of activity. */
  int findRelevant();

  /** Forgot what this does. */
  void clearSuspends();

  /** Activity weaver needs access to these private methods. */
  friend class ActivityWeaver;
};

DUECA_NS_END
#endif
