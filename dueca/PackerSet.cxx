/* ------------------------------------------------------------------   */
/*      item            : TransporterSet.cxx
        made by         : Rene' van Paassen
        date            : 990708
        category        : body file
        description     :
        changes         : 990708 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TransporterSet_cc
#include "PackerSet.hxx"
#include "GenericPacker.hxx"
#include "ParameterTable.hxx"
#define E_CNF
#include "debug.h"
#include "ScriptCreatable.hxx"
#define DO_INSTANTIATE
#include "MemberCall2Way.hxx"

DUECA_NS_START

const ParameterTable *PackerSet::getParameterTable()
{
  static const ParameterTable table[] = {
    { "low-prio", new MemberCall2Way<PackerSet,ScriptCreatable>
      (&PackerSet::setAdmin),
      "A packer-compatible object, that does the bulk, low prority packing\n"
      " to another node"},
    { "regular", new MemberCall2Way<PackerSet,ScriptCreatable>
      (&PackerSet::setRegular),
      "A packer for regular priority messages to another node" },
    { "high-prio", new MemberCall2Way<PackerSet,ScriptCreatable>
      (&PackerSet::setHighPrio),
      "A packer for high-priority, urgent messages to another node" },
    { NULL, NULL,
      "A PackerSet defines which set of packers (for low, regular and high)\n"
      "priority messages channels data for a specific other node.\n"
      "With a table of packer sets in the PackerManager, messages are\n"
      "routed."}
  };
  return table;
}

PackerSet::PackerSet() :
  ScriptCreatable(),
  admin(), regular(), high_prio()
{
  //
}

PackerSet::PackerSet(GenericPacker& admin,
                     GenericPacker& regular,
                     GenericPacker& highprio) :
  admin(boost::intrusive_ptr<GenericPacker>
        (const_cast<GenericPacker*>(&admin))),
  regular(boost::intrusive_ptr<GenericPacker>
          (const_cast<GenericPacker*>(&regular))),
  high_prio(boost::intrusive_ptr<GenericPacker>
            (const_cast<GenericPacker*>(&highprio)))
{
  //
}

bool PackerSet::complete()
{
  return bool(admin) && bool(regular) && bool(high_prio);
}

PackerSet::~PackerSet()
{
  //
}

const char* PackerSet::getTypeName()
{
  return "PackerSet";
}

bool PackerSet::setAdmin(ScriptCreatable &p, bool in)
{
  if (!in) return false;

  GenericPacker *pnew =  dynamic_cast<GenericPacker*> (&p);
  if (pnew == NULL) {
    /* DUECA network.

       Wrong object type encountered. Supply a packer object to the
       PackerSet.
    */
    E_CNF("object is not a packer");
    return false;
  }
  //scheme_id.addReferred(&p);
  admin.reset(pnew);
  return true;
}

bool PackerSet::setRegular(ScriptCreatable &p, bool in)
{
  if (!in) return false;

  GenericPacker *pnew = dynamic_cast<GenericPacker*>(&p);
  if (pnew == NULL) {
    /* DUECA network.

       Wrong object type encountered. Supply a packer object to the
       PackerSet.
    */
    E_CNF("object is not a packer");
    return false;
  }
  //scheme_id.addReferred(&p);
  regular.reset(pnew);
  return true;
}

bool PackerSet::setHighPrio(ScriptCreatable &p, bool in)
{
  if (!in) return false;

  GenericPacker *pnew = dynamic_cast<GenericPacker*>(&p);
  if (pnew == NULL) {
    /* DUECA network.

       Wrong object type encountered. Supply a packer object to the
       PackerSet.
    */
    E_CNF("object is not a packer");
    return false;
  }
  //scheme_id.addReferred(&p);
  high_prio.reset(pnew);
  return true;
}

GenericPacker* PackerSet::getPacker(TransportClass tclass) const
{
  if (tclass == Bulk) return admin.get();
  if (tclass == Regular) return regular.get();
  return high_prio.get();
}

DUECA_NS_END

