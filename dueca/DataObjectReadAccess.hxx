/* ------------------------------------------------------------------   */
/*      item            : DataObjectReadAccess.hxx
        made by         : Rene van Paassen
        date            : 130122
        category        : header file
        description     :
        changes         : 130122 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DataObjectReadAccess_hxx
#define DataObjectReadAccess_hxx

#include <dueca_ns.h>
#include <GenericToken.hxx>

DUECA_NS_START;

/** Helper class that does the actual access, and encapsulates the object
    pointer for the object being read. */
class DataObjectReadAccess
{
  /** In this class, the object is a void pointer, must correspond with
      an actual object, though */
  void* object;

  /** The token that provided the access */
  GenericToken* granter;

  /** When the token provides access, it also can supply an id to track all
      accesses */
  DataResourceId rid;

  /** Time specification for the read access. */
  const DataTimeSpec ts;

public:
  /** Create access to data previously written to a channel. The time
      specification selects the data for the matching time, and the
      iterator selects which entry is to be returned, if multiple
      entries are present.
  */
  DataObjectReadAccess(GenericToken &t, DataTimeSpec& ts,
                       const ChannelEntryIndex it = 0) :
    object(NULL),
    granter(&t),
    rid(it),
    ts(ts)
  {
    //
  }

  /** Access the object pointer */
  void *operator () ()
  {
    if (!object) {
      granter->getDataAccess(object, rid, ts);
    }
    return object;
  }

  /** copy constructor. */
  DataObjectReadAccess(const DataObjectReadAccess& other) :
    object(NULL),
    granter(other.granter),
    rid(other.rid)
  {
    //
  }

  /** Access the latest version in the channel */
  DataObjectReadAccess(GenericToken &t, const ChannelEntryIndex it = 0) :
    object(NULL),
    granter(&t),
    rid(it)
  {
    //
  }

  /** Destructor. Releases the access again. */
  ~DataObjectReadAccess()
  {
    if (object) {
      granter->returnDataAccess(rid);
    }
  }
};

DUECA_NS_END;

#endif
