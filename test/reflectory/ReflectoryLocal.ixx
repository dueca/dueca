// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : ReflectoryLocal.ixx
        made by         : Rene' van Paassen
        date            : 160928
        category        : implementation include file
        description     :
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectoryLocal_ixx
#define ReflectoryLocal_ixx
#include "ReflectoryLocal.hxx"
#include "ReflectoryRemote.hxx"
#include "ReflectoryData.hxx"

DUECA_NS_START;

template<typename TICK>
reflectory_id ReflectoryLocal<TICK>::addChild
(typename ReflectoryBase<TICK>::ref_pointer child) const
{
  for (unsigned ii = 0; ii < this->slots.size(); ii++) {
    if (this->slots[ii]->getName() == child->getName()) {
      throw(reflectory_duplicatechild());
    }
  }

  // add the child
  this->slots.push_back(child);

  // notify the addition for replication, create data object
  ReflectoryData d(0, ReflectoryData::Accepted, this->getNodeId(),
                   child->getPath(), child->getDataClass());

  // path initially empty, ends with new client's id
  d.issued_path.push_front(this->slots.size()-1);

  this->passConfigChange(d);

  return reflectory_id(this->slots.size()-1);
}


template<typename TICK>
ReflectoryLocal<TICK>::ReflectoryLocal(const std::string& path) :
  ReflectoryBase<TICK>(path)
{
  //
}

template<typename TICK>
ReflectoryLocal<TICK>::ReflectoryLocal() :
  ReflectoryBase<TICK>()
{
  //
}

template<typename TICK>
ReflectoryLocal<TICK>::~ReflectoryLocal()
{

}

template<typename TICK>
TICK ReflectoryLocal<TICK>::update(const TICK& tick, unsigned nodeid)
{
  TICK nxt = std::numeric_limits<TICK>::max();

  while (this->planned_config.notEmpty() &&
         this->planned_config.front().target_time <= tick) {

    switch (this->planned_config.front().itemstate) {

    case ReflectoryData::JoinRequest: {

      // there is an additional slave, also remember to do a full
      // copy of the data now
      nslaves++;
      fullcopy = true;

      // send a confirmation, add information on the data type
      ReflectoryData d(0, ReflectoryData::JoinConfirm,
                       this->planned_config.front().nodeid,
                       this->planned_config.front().pathname,
                       this->getDataClass());
      this->passConfigChange(d);

      // for all children, either insert a synthetic join request
      // (local children) or send an actual join request on behalf of the
      // new copy
      for (typename ReflectoryBase<TICK>::childvec_type::iterator ii =
             this->slots.begin(); ii != this->slots.end(); ii++) {
        (*ii)->extendJoinRequest(this->planned_config.front().nodeid);
      }
    }
      break;

    case ReflectoryData::Requested: {
      // check that the child does not yet exist
      try {
        this->operator [] (this->planned_config.front().pathname);
        // already exists, should push a refusal message

        ReflectoryData d(0, ReflectoryData::Rejected, this->getNodeId(),
                         this->planned_config.front().pathname, "");
        this->passConfigChange(d);
      }
      catch (const reflectory_notfound& e) {

        // this is what we need. Insert a new backend here
        reflectory_id nid = this->addChild
          (typename ReflectoryBase<TICK>::ref_pointer
           (new ReflectoryRemote<TICK>(this->planned_config.front())));

        // send the acceptance
        ReflectoryData d(0, ReflectoryData::Accepted, this->getNodeId(),
                         this->planned_config.front().pathname, "");
        d.issued_path.push_back(nid);
        this->passConfigChange(d);
      }
      break;
    }
    case ReflectoryData::Rejected:
    case ReflectoryData::Accepted:
    case ReflectoryData::Released:
      // these are not relevant/possible for a master end
      assert(0);
    case ReflectoryData::DeletionOrdered: {

      // if no children found later, consider self pruned
      this->state = ReflectoryBase<TICK>::Pruned;

      // for all still active children, order deletion
      for (reflectory_id did = this->slots.size(); did--; ) {
        if (this->slots[did].get()) {
          // this->slots[did]->orderDeletion();
          // if any found, still pruning
          this->state = ReflectoryBase<TICK>::Pruning;
        }
      }
    }
      break;
    case ReflectoryData::DeletionRequested: {
      // remove local end
      reflectory_id did = this->planned_config.front().issued_path.back();
      assert(did < this->slots.size() && this->slots[did].get());
      this->slots[did].reset();

      // confirm + broadcast deletion
      ReflectoryData d(0, ReflectoryData::Released, this->getNodeId(),
                       this->planned_config.front().pathname, "");
      d.issued_path.push_back(did);
    }
      break;

    case ReflectoryData::JoinConfirm:
      break;
    }
  }
  if (this->planned_config.notEmpty()) {
    nxt = min(this->planned_config.front().target_time, nxt);
  }

  // cycle through to the children
  for (typename ReflectoryBase<TICK>::childvec_type::iterator cc = this->slots.begin();
       cc != this->slots.end(); cc++) {
    if (cc->get()) {
      nxt = min((*cc)->update(tick, 0), nxt);
    }
  }

  //

  return nxt;
}

template<typename TICK>
void ReflectoryLocal<TICK>::extendJoinRequest(unsigned nid)
{
  // implement this as a "note to self", to be picked up on the next
  // update
  ReflectoryData d(0, ReflectoryData::JoinRequest, nid, this->getPath(), "");
  this->planned_config.push_back(d);
}

DUECA_NS_END;

#endif
