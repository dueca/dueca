/* ------------------------------------------------------------------   */
/*      item            : ReflectoryLocal.hxx
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

#ifndef ReflectoryLocal_hxx
#define ReflectoryLocal_hxx

#include "ReflectoryBase.hxx"

DUECA_NS_START;

/** Distributed configuration tree nodes, with the "master" end
    somewhere else.

    This class does:
    - packing and representation of the master data copy
    - accepting and processing remote requests
*/
template<typename TICK>
class ReflectoryLocal: public ReflectoryBase<TICK>
{
  /** Current number of slaves */
  unsigned nslaves;

  /** Flag to do a full data copy instead of incremental */
  bool fullcopy;

private:
  /** re-implementation virtual function, add a child

      Checks whether the child name is free, then adds the child.
      Throws an exception if child name is already taken.
      The acceptance of the child is issued.

      @param child     pointer to the new child node
  */
  virtual reflectory_id addChild
  (typename ReflectoryBase<TICK>::ref_pointer child) const;

  /** Add a join request for replication.

      @param nid     node that needs the extension. */
  void extendJoinRequest(unsigned nid);

  /** re-implementation virtual function, update logic */
  TICK update(const TICK& tick, unsigned nodeid);

public:
  /** Constructor for only root */
  ReflectoryLocal();

  /** Constructor */
  ReflectoryLocal(const std::string& path);

  /** Destructor */
  ~ReflectoryLocal();
};

DUECA_NS_END;

#endif
