/* ------------------------------------------------------------------   */
/*      item            : ChannelOrganiser.hh
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

#ifndef ChannelOrganiser_hh
#define ChannelOrganiser_hh

#include <list>

#include "GlobalId.hxx"
#include "Exception.hxx"
#include "NameSet.hxx"
#include "ChannelDistribution.hxx"
#include <EndRole.hxx>
#include <nodes.h>
#include "ChannelWriteToken.hxx"

using namespace std;

#include <dueca_ns.h>

DUECA_NS_START
// class names
class UnifiedChannel;
class Environment;
struct ChannelEndUpdate;
struct ChannelChangeNotification;

/** This class defines a set of data needed for keeping an inventory
    of channel ends in different places. The originating channel must
    be at the first location given. */
class ChannelOrganiser
{
public:

  /** This represents a specification for a single end of this
      channel. Each end (i.e.) that exists has its state defined in
      the end spec. */
  class ChannelEndSpec
  {
  public:
    /// The id of the end. Differs only in location from the others.
    GlobalId end_id;

  public:
    /// Constructor
    ChannelEndSpec(const GlobalId& secondary_end);

    /// Destructor
    ~ChannelEndSpec();

    /// Test for equality of two end specifications
    bool operator == (const ChannelEndSpec& e2) const
      {return end_id == e2.end_id;}

    /// Assigment operator
    ChannelEndSpec& operator= (const ChannelEndSpec& l);

    /// Return the end id
    inline const GlobalId& getId() const { return end_id;}

    /// Print one of these to an output stream
    ostream& print(ostream& o) const;
  };

private:
  /// the name tuple of a channel
  NameSet ns;

  /// the ObjectId part, common to all channel ends
  ObjectId common_end_id;

  /// the master ID, set to the first sender
  GlobalId master_id;

  /// transport class
  uint8_t transportclass;

  /** Type of list */
  typedef list<ChannelEndSpec> end_spec_type;

  /** a list of specifications, with the id of the channel end, an
      indication of whether it sends or receives, or possibly both. */
  end_spec_type end_spec;

private:
  /** The ChannelManager gives "us organisers" a pointer to the access
      token that can be used to send the updates to the different
      channel ends. */
  static ChannelWriteToken *channel_updates;

  /// We like the ChannelManager for giving us this pointer.
  friend class ChannelManager;

public:
  /// assignment operation
  ChannelOrganiser& operator= (const ChannelOrganiser& l);

  /// Constructor for an empty, useless ChannelOrganiser
  ChannelOrganiser();

  /// creates the basic entry
  ChannelOrganiser(const NameSet& ns,
                   const ObjectId& common_end_id);

  /// destructor
  ~ChannelOrganiser();

  /// adding a channel end
  void handleEvent(const ChannelChangeNotification &ev,
                   const DataTimeSpec& ts);

  /// get the name of the channel
  inline const NameSet& getNameSet() const {return ns;}

  /// query presence of an outlet here
  bool localOutletPresent() const;

  /// copy constructor
  ChannelOrganiser(const ChannelOrganiser& l);

  /// getGlobalId, needed for the registry template
  const GlobalId& getGlobalId() const
  {
    static GlobalId no_master;
    if (end_spec.size()) return end_spec.front().end_id;
    return no_master;
  }

  /** \name Comparison functions for the set. */
  //@{
  /** Test for being smaller. Smaller means that the name is
      lexicographically less. */
  bool operator < (const ChannelOrganiser& s2) const
    { return ns < s2.ns; }
  /** For being bigger. */
  bool operator > (const ChannelOrganiser& s2) const
    { return ns > s2.ns; }
  /** Equality, here means having the same name. */
  bool operator == (const ChannelOrganiser& s2) const
    { return ns == s2.ns; }
  /** Inequality. */
  bool operator != (const ChannelOrganiser& s2) const
    { return ns != s2.ns; }
  //@}

  /** Print a ChannelOrganiser to a stream, for debugging mainly. */
  ostream& print (ostream& o) const;
};

DUECA_NS_END

PRINT_NS_START
/// Print one of these to an output stream
inline ostream& operator << (ostream& o, const DUECA_NS::ChannelOrganiser::
                             ChannelEndSpec &ce)
{ return ce.print(o); }

/** Print a ChannelOrganiser to a stream, for debugging mainly. */
inline ostream& operator << (ostream& o, const DUECA_NS::ChannelOrganiser& cl)
{ return cl.print(o); }
PRINT_NS_END
#endif




