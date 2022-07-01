/* ------------------------------------------------------------------   */
/*      item            : Summary.cxx
        made by         : Rene' van Paassen
        date            : 010819
        category        : body file
        description     :
        changes         : 010819 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Summary_cxx

#include "DuecaView.hxx"
#include "Summary.hxx"
#include <ModuleId.hxx>
#include <StatusT1.hxx>
#include <dassert.h>
//#define D_STS
#define W_STS
#define E_STS
#include <debug.h>
#include <debprint.h>
DUECA_NS_START

template<class LI, class S, class V>
Summary<LI,S,V>::Summary(LI* id, S* status, Summary<LI,S,V>* parent) :
  link_id(id),
  status(status),
  cnode(NULL),
  branches(),
  dirty(true),
  parent(parent)
{
  // nothing more
}

template<class LI, class S, class V>
Summary<LI,S,V>::~Summary()
{
  delete link_id;
  delete status;
  for (typename list <Summary<LI,S,V>* >::iterator ii = branches.begin();
       ii != branches.end(); ii++) {
    delete (*ii);
  }
}

template<class LI, class S, class V> const S&
Summary<LI,S,V>::getOrCalculateStatus()
{
  if (dirty) {
    if (branches.size()) {

      DEB1(*this << " dirty, checking");

      // combine my status out of that of all my children
      status->clear();
      for (typename list <Summary<LI,S,V>* >::iterator ii = branches.begin();
           ii != branches.end(); ii++) {
        *status &= (*ii)->getOrCalculateStatus();
      }
    }
  }

  dirty = false;
  return *status;
}

template<class LI, class S, class V>
void Summary<LI,S,V>::setDirty()
{
  dirty = true;
  if (parent != NULL) parent->setDirty();
}


template<class LI, class S, class V> bool
Summary<LI,S,V>::updateStatus(const LI& id, const S& newstatus)
{
  // if this is a match, use the status and return true
  if (link_id->isMe(id)) {
    DEB1(*this << " new status" << newstatus);
    if (!(*status == newstatus)) {
      *status = newstatus;
      setDirty();
      if (parent == NULL) {
        /* DUECA UI.

           Setting the status of an orphaned node to dirty.
        */
        W_STS("Setting node " << *link_id
              << " dirty, no parent!");
      }
    }
    return true;
  }

  // if this is a descendant, ask them
  if (branches.size() && link_id->isMeOrDescendant(id)) {
    for (typename list <Summary<LI,S,V>* >::iterator ii = branches.begin();
         ii != branches.end(); ii++) {
      if ((*ii)->updateStatus(id, newstatus)) {
        //dirty |= (*ii)->isDirty();
        return true;
      }
    }
  }

  // otherwise, this status cannot be found here
  return false;
}

template<class LI, class S, class V> bool
Summary<LI,S,V>::existsSummary(const LI& id)
{
  // if this is a match, use the status and return true
  if (link_id->isMe(id)) {
    return true;
  }

  // if this is a descendant, ask them
  if (branches.size() && link_id->isMeOrDescendant(id)) {
    for (typename list <Summary<LI,S,V>* >::const_iterator ii = branches.begin();
         ii != branches.end(); ii++) {
      if ((*ii)->link_id->isMeOrDescendant(id)) {
        return (*ii)->existsSummary(id);
      }
    }
  }

  return false;
}

template<class LI, class S, class V> Summary<LI,S,V>&
Summary<LI,S,V>::findSummary(const LI& id)
{
  // if this is a match, use the status and return true
  if (link_id->isMe(id)) {
    return *this;
  }

  // if this is a descendant, ask them
  if (branches.size() && link_id->isMeOrDescendant(id)) {
    for (typename list <Summary<LI,S,V>* >::const_iterator ii = branches.begin();
         ii != branches.end(); ii++) {
      if ((*ii)->link_id->isMeOrDescendant(id)) {
        return (*ii)->findSummary(id);
      }
    }
  }

  // otherwise, this status cannot be found here
  throw NotFound();
}

template<class LI, class S, class V> bool
Summary<LI,S,V>::insertLinkAndStatus(const LI& id, const S& status)
{
  if (!link_id->isMeOrDescendant(id)) return false;
  if (link_id->isMe(id)) return true;

  // apparently this link belongs here, maybe with one of my children?
  for (typename list <Summary<LI,S,V>* >::iterator ii = branches.begin();
       ii != branches.end(); ii++) {
    if ((*ii)->insertLinkAndStatus(id, status)) return true;
  }

  // must insert a new child here. This child may have only one name
  // component more than me
  LI& id2 = LI::create(id, link_id->getNumParts() + 1);
  Summary<LI,S,V>* s = new Summary(&id2, new S (status), this);
  branches.push_back(s);
  DEB1("Inserted new summary " << s << ", parent = " << *this);

  s->cnode = V::single()->insertEntityNode
    (id2.getLast(), cnode, int(id.getGlobalId().getLocationId()),
     &(s->getStatus()));

  // chain on to ensure that no more components have to be inserted
  s->insertLinkAndStatus(id, status);

  return true;
}

template<class LI, class S, class V>
ostream&  Summary<LI,S,V>::print(ostream& os) const
{
  return os << "Summary(link_id=" << *link_id
            << ", status=" << *status << ')';
}


template<class LI, class S, class V>
ostream& operator << (ostream& os, const Summary<LI,S,V>& o)
{
  return o.print(os);
}

template<class LI, class S, class V>
const Summary<LI,S,V>& Summary<LI,S,V>::getParent() const
{
  if (parent == NULL) throw NotFound();
  return *parent;
}

template class Summary<ModuleId,StatusT1,DuecaView>;
template ostream& operator << <ModuleId,StatusT1,DuecaView>
      (ostream& os, const Summary<ModuleId,StatusT1,DuecaView> & o);
DUECA_NS_END
