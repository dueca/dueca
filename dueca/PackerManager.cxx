/* ------------------------------------------------------------------   */
/*      item            : PackerManager.cxx
        made by         : Rene' van Paassen
        date            : 990624
        category        : body file
        description     :
        changes         : 990624 first version
                          020327 added method stopPackers
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define PackerManager_cc
#include "PackerManager.hxx"
#include "ObjectManager.hxx"
#include "GenericPacker.hxx"
#include "CriticalActivity.hxx"
#include "ParameterTable.hxx"
#include <dueca-conf.h>
#define I_NET
#define E_CNF
#include "debug.h"
#include "dueca_assert.h"
#include <dassert.h>
#define DO_INSTANTIATE
#include "MemberCall.hxx"
#include "MemberCall2Way.hxx"

DUECA_NS_START
// -----------------------------------------------------------------

PackerManager* PackerManager::singleton = NULL;
// packermanager still has an odd scheme-only interface;
const ParameterTable* PackerManager::getParameterTable()
{
  static const ParameterTable table[] = {
#if defined(SCRIPT_SCHEME)
    { "obsolete-vector", new MemberCall<PackerManager,SCM>
      (&PackerManager::setVector),
      "A scheme vector with PackerSet objects as elements\n"
      "This is obsolete, please use the 'add-set method instead." },
#endif
    { "add-set", new MemberCall2Way<PackerManager,ScriptCreatable>
      (&PackerManager::addSet),
      "A PackerSet. Repeat as necessary, one set for each node." },
    { NULL, NULL,
      "The PackerManager maintains an index of packer sets, each of these\n"
      "packer sets contains three packer objects, for routing data to a\n"
      "node at bulk, normal and high priority respectively. Specify a packer\n"
      "set for each node in the DUECA process. The set of the 'own' node\n"
      "will be ignored"}
  };

  return table;
}



PackerManager::PackerManager()
{
  singleton = this;
}

PackerManager::~PackerManager()
{
  //
}

const char* PackerManager::getTypeName()
{
  return "PackerManager";
}

bool PackerManager::addSet(ScriptCreatable& s, bool in)
{
  // only valid for specifying stuff, not for getting it out again
  if (!in) return false;

  // check whether the inheritance is correct
  PackerSet *snew = dynamic_cast<PackerSet*> (&s);
  if (snew == NULL) {
    /* DUECA network.

       Packer set-up attempted with an object that is not a
       packerset. Check your DUECA configuration file. */
    E_CNF("object is not a PackerSet");
    return false;
  }

  // protect from Scheme garbage collection, and add to the sets.
  //scheme_id.addReferred(&s);
  packer_set.push_back(snew);

  // signal success
  return true;
}

#if defined(SCRIPT_SCHEME)
// obsolete!!
bool PackerManager::setVector(const SCM& vec)
{
  // should be a vector object
  if ( !(SCM_NIMP(vec) && SCM_VECTORP(vec)) ) {
    /* DUECA network.

       Packer set-up attempted with an object that is not a
       sequence/vector of packers. Check your DUECA configuration
       file. */
    E_CNF("Supply a vector, or better, use 'add-set");
    return false;
  }

  // check the size and create packer sets from all elements
  int n_elts = SCM_LENGTH(vec);
  const SCM* v = SCM_VELTS(vec);
  for (int ii = 0; ii < n_elts; ii++) {
    if (ii != ObjectManager::single()->getLocation()) {
      DUECA_SCM_ASSERT
        (SCM_NIMP(v[ii]) &&
         SchemeClassData<PackerSet>::single()->validTag(v[ii]),
         v[ii], SCM_ARG1, SchemeClassData<PackerSet>::single()->getMakeName());

      packer_set.push_back(reinterpret_cast<PackerSet*>(SCM_SMOB_DATA(v[ii])));
      scheme_id.addReferred(v[ii]);
    }
    else {
      // don't care whats at place location
      packer_set.push_back(NULL);
    }
  }
  return true;
}
#endif

bool PackerManager::complete()
{
  //

  // check the size of the packer array, must be large enough for all
  // nodes.
  if (ObjectManager::single()->getNoOfNodes() > 1 &&
      int(packer_set.size()) < ObjectManager::single()->getNoOfNodes()) {
    /* DUECA network.

       Not enough packer configurations set-up; for each node in the
       DUECA process there should be a packer set. Check your DUECA
       configuration file.
    */
    E_CNF("Not enough packer sets");
    return false;
  }
  return true;
}

void PackerManager::stopPackers()
{
  for (int ii = packer_set.size(); ii--; ) {
    if (ii != ObjectManager::single()->getLocation()) {
      (packer_set[ii]->getPacker(Bulk))->stopPacking();
      (packer_set[ii]->getPacker(Regular))->stopPacking();
      (packer_set[ii]->getPacker(HighPriority))->stopPacking();
    }
  }
  /* DUECA network.

     Commanded all data packing objects to stop further data
     packing. */
  I_NET("PackerManager stopped packers");
}

GenericPacker* PackerManager::
findMatchingTransport(int destination, Channel::TransportClass tclass)
{
  // this should now be caught in the complete method
  assert(destination < int(singleton->packer_set.size()));

  return (singleton->packer_set[destination])->getPacker(tclass);
}

DUECA_NS_END
