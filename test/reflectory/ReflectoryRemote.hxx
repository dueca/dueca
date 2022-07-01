/* ------------------------------------------------------------------   */
/*      item            : ReflectoryRemote.hxx
        made by         : Rene van Paassen
        date            : 160928
        category        : header file
        description     :
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectoryRemote_hxx
#define ReflectoryRemote_hxx

#include "ReflectoryBase.hxx"

DUECA_NS_START;

class DataSetConverter;

/** Distributed configuration tree nodes, with the "master" end
    somewhere else.

    This class does:
    - unpacking and representation of the local data copy
    - marshalling of local requests, response to remote requests
*/
template<typename TICK>
class ReflectoryRemote: public ReflectoryBase<TICK>
{
  /** Converter to unpack data from incoming stream */
  const DataSetConverter* converter;

  /** Remember the latest unpacked object as reference for
      differential unpack */
  const void* latest_unpacked;

  /** Location for locally created children waiting for approval */
  mutable std::list<typename ReflectoryBase<TICK>::ref_pointer> waitroom;

private:
  /** re-implementation of virtual function, unpack data from net

      @param as      Store with data
      @param diff    Unpack relative difference to latest incoming data */
  void unpackData(AmorphReStore& as, bool diff);

  /** Re-implementation of virtual function, update the current data state and
      process planned changes.

      @param tick    Time/cycle to take. */
  TICK update(const TICK& tick, unsigned nodeid);

  /** re-implementation virtual function, add a child

      Stores the child pointer for local reference, and passes a
      request to the "master"

      @param child     pointer to the new child node
  */
  reflectory_id addChild
  (typename ReflectoryBase<TICK>::ref_pointer child) const;

  /** Add a join request for replication.

      @param nid     node that needs the extension. */
  void extendJoinRequest(unsigned nid);

public:
  /** Constructor for the root remote only */
  ReflectoryRemote();

  /** Constructor */
  ReflectoryRemote(const ReflectoryData& cdata);

  /** Destructor */
  ~ReflectoryRemote();
};

DUECA_NS_END;

#endif
