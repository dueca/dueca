/* ------------------------------------------------------------------   */
/*      item            : ChannelIdList.hh
        made by         : Rene' van Paassen
        date            : 980319
        category        : header file
        description     :
        notes           : object for keeping channel data in the registry.
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelIdList_hh
#define ChannelIdList_hh

#include "GlobalId.hxx"
#include "Exception.hxx"
#include "NameSet.hxx"
#include "ChannelDistribution.hxx"
#include <EndRole.hxx>

#include <dueca_ns.h>
DUECA_NS_START
// class names
class UnifiedChannel;
struct ChannelEndUpdate;

/** This class defines a set of data needed for keeping an inventory
    of channel ends in different places. The originating channel must
    be at the first location given. */
struct ChannelIdList
{
  /** the name tuple of a channel. */
  NameSet ns;

  /** id of this channel end. */
  GlobalId channel_id;

  /** a pointer to the channel object. */
  UnifiedChannel* channel;

public:
  /** Empty constructor. */
  ChannelIdList();

  /** creates the basic entry. */
  ChannelIdList(const NameSet& ns,  UnifiedChannel* channel);

  /** destructor. */
  ~ChannelIdList();

  /** assignment operation. */
  ChannelIdList& operator= (const ChannelIdList& l);

  /** changing a channel end. */
  void adjustChannelEnd(const ChannelEndUpdate& u);

  /** get the name of the channel. */
  inline const NameSet& getNameSet() const {return ns;}

  /** Return a pointer to the local end, if there is one. */
  inline UnifiedChannel* getLocalEnd() const {return channel;}

  /** copy constructor. */
  ChannelIdList(const ChannelIdList& l);

  /** getGlobalId, needed for the registry template. */
  const GlobalId& getGlobalId() const {return channel_id; }

  /** \name Comparison functions for the set. */
  //@{
  /** Test for being smaller. Smaller means that the name is
      lexicographically less. */
  bool operator < (const ChannelIdList& s2) const
    { return ns < s2.ns; }

  /** For being bigger. */
  bool operator > (const ChannelIdList& s2) const
    { return ns > s2.ns; }

  /** Equality, here means having the same name. */
  bool operator == (const ChannelIdList& s2) const
    { return ns == s2.ns; }

  /** Inequality. */
  bool operator != (const ChannelIdList& s2) const
    { return ns != s2.ns; }
  //@}

  /** Print to stream. */
  std::ostream& print(std::ostream& o) const;
};

DUECA_NS_END

PRINT_NS_START
inline ostream& operator << (ostream& o, const DUECA_NS::ChannelIdList& cl)
{ return cl.print(o); }
PRINT_NS_END
#endif
