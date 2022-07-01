/* ------------------------------------------------------------------   */
/*      item            : TrimId.cxx
        made by         : Rene' van Paassen
        date            : 010926
        category        : body file
        description     :
        changes         : 010926 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TrimId_cxx
#include "TrimId.hxx"
#include <dueca-conf.h>
#include <Summary.hxx>
#include <debug.h>
#include <TrimView.hxx>
#include <IncoMode.hxx>
#include <IncoTable.hxx>
#include <IncoVariable.hxx>
#include <dassert.h>
#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START


map<vstring, TrimId::Index> TrimId::name_map;
vector<vstring> TrimId::names;
vector<vector<TrimId* > > TrimId::module_id_map;

GlobalId TrimId::dummy_global_id(-1, -1);

TrimId::TrimId(const vector<vstring>& nameparts,
               int cal, int tvar) :
  name_idx(),
  calculator(cal),
  tvariable(tvar)
{
  // get the list of indices to name parts, may update the map of names
  for (vector<vstring>::const_iterator ii = nameparts.begin();
       ii != nameparts.end(); ii++) {
    name_idx.push_back(findOrAddName(*ii));
  }

  // connect the global_id matrix to this id, at least if it is not an
  // unititialised -1 -1 id
  if (cal >= 0 && tvar >= 0) {
    indexThreeId();
  }
  DEB1("new " << *this);
}

TrimId& TrimId::create(const vector<vstring>& nameparts,
                       int cal, int tvar)
{
  TrimId* o;
  try {
    // check previous existence
    o = &find(cal, tvar);
  }
  catch (NotFound &e) {

    // make a new module
    return * new TrimId(nameparts, cal, tvar);
  }

  // found an old module. Error if this is not exactly the same!
  if (nameparts.size() != o->name_idx.size()) {
    throw NotFound();
  }
  for (int ii = o->name_idx.size(); ii--; ) {
    if (nameparts[ii] != names[o->name_idx[ii]]) {
      throw NotFound();
    }
  }

  // name is OK, maybe globalId was not initialised
  if (o->calculator == -1 && cal >= 0 && tvar >= 0) {
    o->calculator = cal;
    o->tvariable = tvar;
    o->indexThreeId();
  }
  return *o;
}

TrimId& TrimId::create(const TrimId& id, int nparts)
{
  if (int(id.name_idx.size()) == nparts)
    return *const_cast<TrimId*>(&id);
  if (int(id.name_idx.size()) < nparts) throw NotFound();

  vector<vstring> name_idx;
  for (int ii = 0; ii < nparts; ii++) {
    name_idx.push_back(names[id.name_idx[ii]]);
  }
  return create(name_idx, -1, -1);
}

TrimId::~TrimId()
{
  DEB1("deleting " << *this);
  // names are not yet deleted.
}

bool TrimId::isMeOrDescendant(const TrimId& o) const
{
  if (o.name_idx.size() < name_idx.size()) return false;

  for (unsigned int ii = 0; ii < name_idx.size(); ii++) {
    if (name_idx[ii] != o.name_idx[ii]) return false;
  }

  // passed all tests,
  return true;
}

bool TrimId::isMe(const TrimId& o)
{
  if (o.name_idx.size() == name_idx.size() &&
      isMeOrDescendant(o)) return true;
  return false;
}

TrimId& TrimId::find(int cal, int tvar)
{
  if (cal < 0 || cal >= int(module_id_map.size()) ||
      tvar < 0 || tvar >= int(module_id_map[cal].size()) ||
      module_id_map[cal][tvar] == NULL) {
    throw NotFound();
  }
  return *module_id_map[cal][tvar];
}

TrimId::Index TrimId::findOrAddName(const vstring& name)
{
  map<vstring,Index>::iterator r;
  if ((r = name_map.find(name)) == name_map.end()) {

    // invent a new index for this string, push it onto the map
    Index newindex = name_map.size();
    name_map[name] = newindex;
    names.push_back(name);
    assert(names.size() == name_map.size());


    return newindex;
  }
  else {
    return r->second;
  }
}


void TrimId::indexThreeId()
{
  // Update the index matrix with globalId's
  assert(calculator >= 0);
  assert(tvariable >= 0);

  if (int(module_id_map.size()) <= calculator) {
    module_id_map.resize(calculator+1,vector<TrimId*>());
  }
  if (int(module_id_map[calculator].size()) <= tvariable) {
    module_id_map[calculator].resize(tvariable+1, NULL);
  }
  if (module_id_map[calculator][tvariable] != NULL) {
    /* DUSIME Trim.

       The current trim ID was already entered, indicates an error in the
       trim table. */
    W_STS("trim id entered previously");
  }
  module_id_map[calculator][tvariable] = this;
}

TrimId::Index TrimId::findName(const vstring& name, Index level)
{
  map<vstring,Index>::iterator r;
  if ((r = name_map.find(name)) == name_map.end()) {
    return -1;
  }
  else {
    return r->second;
  }
}


ostream& operator << (ostream& os, const TrimId& o)
{
  os << "TrimId(cal=" << o.calculator
     << ", tvar=" << o.tvariable << ", names=";
  for (unsigned int ii = 0; ii < o.name_idx.size(); ii++) {
    os << o.name_idx[ii] << '/' << o.names[o.name_idx[ii]] <<
      (ii + 1 == o.name_idx.size() ? ")" : ", ");
  }
  return os;
}

double TrimId::getValue() const
{
#if defined(BUILD_DMODULES) && 0
  return TrimView::single()->getIncoVariable(calculator, tvariable).
    getValue();
#else
  assert(0);
  return 0.0;
#endif
}

IncoVariableWork& TrimId::getIncoVariable() const
{
#if defined(BUILD_DMODULES)
  return TrimView::single()->getIncoVariable(calculator, tvariable);
#else
  static IncoVariableWork invalid(IncoVariable("invalid", 0.0, 0.0));
  assert(0);
  return invalid;
#endif
}

const char* TrimId::getRoleString(IncoMode mode)
{
#if defined(BUILD_DMODULES)
  return
    getString(TrimView::single()->getIncoVariable(calculator, tvariable).
              findRole(mode));
#else
  assert(0);
  return NULL;
#endif
}
DUECA_NS_END
