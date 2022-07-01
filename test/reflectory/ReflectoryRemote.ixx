// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : ReflectoryRemote.cxx
        made by         : Rene' van Paassen
        date            : 160928
        category        : body file
        description     :
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectoryRemote_ixx
#define ReflectoryRemote_ixx

#include <cassert>
#include "ReflectoryRemote.hxx"
#include "DataClassRegistry.hxx"
#include "DataSetConverter.hxx"
#include "AmorphStore.hxx"
#include <iostream>

#define DEB(A) std::cerr << A << std::endl;

DUECA_NS_START;

template<typename TICK>
ReflectoryRemote<TICK>::ReflectoryRemote() :
  ReflectoryBase<TICK>(),
  converter(NULL),
  latest_unpacked(NULL)
{
  // search for the data converter (may throw DataObjectClassNotFound)
  this->state = ReflectoryBase<TICK>::Created;
  converter = NULL; //DataClassRegistry::single().getConverter("Object");
}

template<typename TICK>
ReflectoryRemote<TICK>::ReflectoryRemote(const ReflectoryData& cdata) :
  ReflectoryBase<TICK>(cdata.pathname)
{
  // only on accepted can this be done
  assert(cdata.itemstate == ReflectoryData::Accepted);

  // search for the data converter (may throw DataObjectClassNotFound)
  converter = DataClassRegistry::single().getConverter(cdata.dataclass);
}

template<typename TICK>
void ReflectoryRemote<TICK>::unpackData(AmorphReStore& as, bool diff)
{
  // unpack the time for the data change
  TICK tick(as);

  // diff indicates that the data was packed as difference to the
  // previous value
  if (diff) {
    if (latest_unpacked) {
      latest_unpacked = converter->createDiff(as, latest_unpacked);
    }
    else {
      // there was no previous value yet, awaiting a full pack for
      // kick-starting. However, need to unpack this diff data & delete again
      void *dum = converter->createDiff(as, NULL);
      converter->delData(dum);
      DEB("no data yet in " << this->path << " cannot unpack diff");
      return;
    }
  }
  else {
    latest_unpacked = converter->create(as);
  }
  this->planned_value.push_back
    (typename ReflectoryBase<TICK>::DataChange(latest_unpacked, tick));
}

template<typename TICK>
TICK ReflectoryRemote<TICK>::update(const TICK& tick, unsigned nodeid)
{
  TICK nxt = std::numeric_limits<TICK>::max();

  switch (this->state) {
  case ReflectoryBase<TICK>::Created: {
    ReflectoryData d(tick, ReflectoryData::JoinRequest, nodeid, "/", "");
    this->passConfigChange(d);
    this->state = ReflectoryBase<TICK>::WaitConfirm;
  }
    break;

  case ReflectoryBase<TICK>::WaitConfirm: {

    while (this->planned_config.notEmpty() &&
           this->planned_config.front().nodeid != nodeid &&
           this->planned_config.front().itemstate !=
           ReflectoryData::JoinConfirm) {
      this->planned_config.pop();
    }
    if (this->planned_config.notEmpty() &&
        this->planned_config.front().nodeid == nodeid &&
        this->planned_config.front().itemstate ==
        ReflectoryData::JoinConfirm) {
      this->state = ReflectoryBase<TICK>::Active;
    }
  break;

  default:
    break;
  }
  }
  // process any planned data changes
  while (this->planned_value.front().target_time <= tick) {
    converter->delData(this->current);
    this->current = this->planned_value.front().value;
    this->planned_value.pop();
  }
  if (this->planned_value.notEmpty()) {
    nxt = this->planned_value.front().target_time;
  }

  // now process planned configuration changes
  return nxt; //min(this->ReflectoryBase<TICK>::update(tick), nxt);
}

template<typename TICK>
ReflectoryRemote<TICK>::~ReflectoryRemote()
{
  // no further action needed
}

template<typename TICK>
reflectory_id ReflectoryRemote<TICK>::addChild
(typename ReflectoryBase<TICK>::ref_pointer child) const
{
  // add the child to a waiting room
  waitroom.push_back(child);

  // notify the addition for replication, create data object
  ReflectoryData d(0, ReflectoryData::Requested,
                   this->getNodeId(), child->getPath(),
                   child->getDataClass());

  // send on the configuration change
  this->passConfigChange(d);

  // return the invalid number
  return std::numeric_limits<reflectory_id>::max();
}


template<typename TICK>
void ReflectoryRemote<TICK>::extendJoinRequest(unsigned nid)
{
  // implement this as a "note to self", to be picked up on the next
  // update
  ReflectoryData d(0, ReflectoryData::JoinRequest, nid, this->getPath(), "");
  this->passConfigChange(d);
}

DUECA_NS_END;

#endif
