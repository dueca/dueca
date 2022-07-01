/* ------------------------------------------------------------------ */
/*      item            : ActivityContext.hxx
        made by         : repa
        date            : Wed Nov 22 10:11:09 2006
        category        : header file
        description     : DUECA event data object
        notes           :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ActivityContext_hxx
#define ActivityContext_hxx

#include <dueca_ns.h>
#include <AmorphStore.hxx>
#include <iostream>
#include <CommObjectTraits.hxx>

DUECA_NS_START

/** A union/class capturing the context in which an activity took
    place: node, manager/priority, and id of the activity.

    This class can be used for the transport of data in
    event channels */
union ActivityContext
{
  struct Parts {
    /** A single element manager. */
    unsigned int manager : 8;

    /** Node number. */
    unsigned int node : 8;

    /** A single element activity_id. */
    unsigned int activity_id : 16;

  } parts;

  /** Alternative to specifying the elements of the union. */
  uint32_t total;

  /** Constructor */
  ActivityContext();

  /** constructor (for use with event data). */
  ActivityContext(const unsigned int& manager,
                  const unsigned int& activity_id);

  /** constructor (with specified node). */
  ActivityContext(const unsigned int& node,
                  const unsigned int& manager,
                  const unsigned int& activity_id);

 /** copy constructor, will in practice not be used. */
  ActivityContext(const ActivityContext& o);

  /** constructor to restore an ActivityContext from amorphous storage. */
  ActivityContext(AmorphReStore& r);

  /** destructor. */
  ~ActivityContext();

  /** unpacks the ActivityContext from an amorphous storage. */
  void unPackData(AmorphReStore& s);

  /** packs the ActivityContext into amorphous storage. */
  void packData(AmorphStore& s) const;

  /** prints the ActivityContext to a stream.
      \todo Print descriptive information using activity description
      lists. */
  ostream & print(ostream& s) const;

  /** inequality */
  inline bool operator != (const ActivityContext& other) const
  { return this->total != other.total; }

  /** equality */
  inline bool operator == (const ActivityContext& other) const
  { return this->total == other.total; }
};

template <>
const char* getclassname<ActivityContext>();

DUECA_NS_END



/** pack the object into amorphous storage. */
inline void packData( DUECA_NS ::AmorphStore& s,
                      const DUECA_NS ::ActivityContext& o)
{ o.packData(s); }

inline void unPackData(DUECA_NS ::AmorphReStore& s,
                       DUECA_NS ::ActivityContext& o)
{ o.unPackData(s); }

PRINT_NS_START

/** print to a stream. */
inline ostream & operator << (ostream& s, const DUECA_NS::ActivityContext& o)
{ return o.print(s); }


istream& operator >> (istream&s, DUECA_NS::ActivityContext& o);

PRINT_NS_END

#endif
