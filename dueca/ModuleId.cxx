/* ------------------------------------------------------------------   */
/*      item            : ModuleId.cxx
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


#define ModuleId_cxx
#include "ModuleId.hxx"
#include <NameSet.hxx>
//#define D_STS
#define I_STS
#include <debug.h>
#include <Summary.hxx>
#include <dassert.h>
#include <cstring>
#define DEBPRINTLEVEL -1
#include <debprint.h>
DUECA_NS_START

map<vstring, ModuleId::Index> ModuleId::name_map;
vector<vstring> ModuleId::names;
vector<vector <ModuleId*> > ModuleId::module_id_map;

const char* NotFound::msg = "Module not found";

ModuleId::ModuleId(const vector<vstring>& nameparts,
                   const GlobalId& id) :
  name_idx(),
  global_id(id)
{
  // get the list of indices to name parts, may update the map of names
  for (vector<vstring>::const_iterator ii = nameparts.begin();
       ii != nameparts.end(); ii++) {
    name_idx.push_back(findOrAddName(*ii));
  }

  // connect the global_id matrix to this id, at least if it is not an
  // unititialised -1 -1 id
  if (global_id != GlobalId()) {
    indexGlobalId();
  }
  DEB("new " << *this);
}


ModuleId& ModuleId::create(const NameSet& ns, const GlobalId& id)
{
  vector<vstring> nameparts;
  nameparts.push_back(ns.getEntity());
  #if OLD_INDEXING
  if (ns.getPart() != std::string("")) nameparts.push_back(ns.getPart());
  if (ns.getClass() != std::string("")) nameparts.push_back(ns.getClass());
#else
  if (ns.getClass().size()) {
    if (ns.getPart().size()) {
      nameparts.push_back(ns.getClass() + ':' + ns.getPart());
    }
    else {
      nameparts.push_back(ns.getClass());
    }
  }
#endif
  return create(nameparts, id);
}

bool ModuleId::exists(const GlobalId& id)
{
  return id.getLocationId() >= 0 && id.getObjectId() >= 0 &&
    id.getLocationId() < int(module_id_map.size()) &&
    id.getObjectId() < int(module_id_map[id.getLocationId()].size()) &&
    module_id_map[id.getLocationId()][id.getObjectId()] != NULL;
}

ModuleId& ModuleId::create(const vector<vstring>& nameparts,
                           const GlobalId& id)
{
  ModuleId* o;
  try {
    // check previous existence
    o = &find(id);
  }
  catch (NotFound &e) {

    // make a new module
    return * new ModuleId(nameparts, id);
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
  if (o->global_id == GlobalId() && id != GlobalId()) {
    o->global_id = id;
    o->indexGlobalId();
  }
  return *o;
}

ModuleId& ModuleId::create(const ModuleId& id, int nparts)
{
  if (int(id.name_idx.size()) == nparts)
    return *const_cast<ModuleId*>(&id);
  if (int(id.name_idx.size()) < nparts) {
    throw NotFound();
  }

  vector<vstring> name_idx;
  for (int ii = 0; ii < nparts; ii++) {
    name_idx.push_back(names[id.name_idx[ii]]);
  }
  return create(name_idx, GlobalId());
}

ModuleId::~ModuleId()
{
  DEB("deleting " << *this);
  // names are not yet deleted.
}

bool ModuleId::isMeOrDescendant(const ModuleId& o) const
{
  if (o.name_idx.size() < name_idx.size()) return false;

  for (unsigned int ii = 0; ii < name_idx.size(); ii++) {
    if (name_idx[ii] != o.name_idx[ii]) return false;
  }

  // passed all tests,
  return true;
}

bool ModuleId::isMe(const ModuleId& o)
{
  if (o.name_idx.size() == name_idx.size() &&
      isMeOrDescendant(o)) return true;
  return false;
}

ModuleId& ModuleId::find(const GlobalId& id)
{
  unsigned int idx1 = id.getLocationId();
  unsigned int idx2 = id.getObjectId();
  if (idx1 >= module_id_map.size() ||
      idx2 >= module_id_map[idx1].size() ||
      module_id_map[idx1][idx2] == NULL) {
    throw NotFound();
  }
  return *module_id_map[idx1][idx2];
}

ModuleId::Index ModuleId::findOrAddName(const vstring& name)
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


void ModuleId::indexGlobalId()
{
  // Update the index matrix with globalId's
  assert(global_id.getLocationId() >= 0);
  assert(global_id.getObjectId() >= 0);

  unsigned int idx1 = global_id.getLocationId();
  unsigned int idx2 = global_id.getObjectId();
  if (module_id_map.size() <= idx1) {
    module_id_map.resize(idx1+1,vector<ModuleId*>());
  }
  if (module_id_map[idx1].size() <= idx2) {
    module_id_map[idx1].resize(idx2+1, NULL);
  }
  if (module_id_map[idx1][idx2] != NULL) {
    /* DUECA system.

       When making an inventory of all ID's for DUECA modules, an ID
       was flagged as entered previously. This indicates a programming
       error in the DUECA code.
    */
    W_STS(global_id << " entered previously");
  }
  module_id_map[idx1][idx2] = this;
}

ModuleId::Index ModuleId::findName(const vstring& name, Index level)
{
  map<vstring,Index>::iterator r;
  if ((r = name_map.find(name)) == name_map.end()) {
    return -1;
  }
  else {
    return r->second;
  }
}

/*ModuleId& ModuleId::truncate(int n)
{
  assert(n <= int(name_idx.size()));
  name_idx.resize(n);
  return *this;
}*/

std::ostream& ModuleId::print(std::ostream& os) const
{
  os << "ModuleId(global_id=" << global_id << ", names=";
  for (unsigned int ii = 0; ii < name_idx.size(); ii++) {
    os << name_idx[ii] << '/' << names[name_idx[ii]] <<
      (ii + 1 == name_idx.size() ? ")" : ", ");
  }
  return os;
}
DUECA_NS_END
