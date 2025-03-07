/* ------------------------------------------------------------------   */
/*      item            : ModuleId.hxx
        made by         : Rene van Paassen
        date            : 010819
        category        : header file
        description     :
        changes         : 010819 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ModuleId_hxx
#define ModuleId_hxx

#include <inttypes.h>
#include <GlobalId.hxx>
#include <stringoptions.h>
#include <vector>
#include <map>
using namespace std;

#include <dueca_ns.h>
DUECA_NS_START
struct NameSet;
class NotFound;

/** Object for organising name information.

    For capturing and organising status feedback reports from modules
    and their descendants (in a Dusime environment SimulationModule
    and HardwareModule), this ModuleId provides a link between the
    following:
    <ol>
    <li> The GlobalId of the named DUECA object.
    <li> The NameSet of the named DUECA object.
    <li> The hierarchy of entities with modules.
    </ol>

    \todo When this type of information is also used for channels,
    then a common base class should be created, with the common
    functionality, and derived classes, one with a module_id_map and
    one with a channel_id_map, are needed.
*/

class ModuleId
{
  /** A type for indices. */
  typedef int Index;

  /** a list with indexes to the names. The size of the list also
      gives my level into the hierarchy. An empty list stands for a
      (usually temporary) object that will be used to search with the
      GlobalId */
  std::vector<Index> name_idx;

  /** DUECA object id, also gives information about node. */
  GlobalId global_id;

  /** A map of names. Because of the tree-like structure,
      usually a lot of name components are the same. Here, we use the
      indices to point to the proper parts in the vector. */
  static std::map<vstring, Index> name_map;

  /** A vector of the names, to find a name corresponding to an
      index. */
  static std::vector<vstring>     names;

  /** A type name for the vector of pointers to the module ID's */
  typedef std::vector<ModuleId*> vpernode;

  /** A map of the global id's, so we can quickly locate a ModuleId
      from the GlobalId. */
  static std::vector<vpernode> module_id_map;

  /** Constructor, using a name */
  ModuleId(const vector<vstring>& nameparts,
           const GlobalId& id);

  /** Copy constructor, private, not implemented. Should never copy */
  ModuleId(const ModuleId& o);

  /** Assignment, private, should never assign */
  ModuleId& operator = (const ModuleId& o);

  /** Update the globalid index. */
  void indexGlobalId();

public:

  /** Destructor. */
  ~ModuleId();

  /** Find a ModuleId using a GlobalId. */
  static ModuleId& find(const GlobalId& id);

  /** Create a new ModuleId. */
  static ModuleId& create(const NameSet& ns, const GlobalId& id);

  /** Create a new ModuleId. */
  static ModuleId& create(const vector<vstring>& nameparts,
                          const GlobalId& id);

  /** Create a new ModuleId with a limited number of parts. */
  static ModuleId& create(const ModuleId& id, int nparts);

  /** See whether some module is one of my secendants. */
  bool isMeOrDescendant(const ModuleId& o) const;

  /** See whether this is me. */
  bool isMe(const ModuleId& o);

  // ** Truncate this id to a limited number of name parts. */
  //  ModuleId& truncate(int n);

  /** get the number of name parts. */
  int getNumParts() const {return name_idx.size();}

  /** get last component of the name. */
  const char* getLast() const
  {
    static const char* none = "";
    if (name_idx.size()) {
      return names[name_idx[name_idx.size() - 1]].c_str();
    }
    return none;
  }

  /** Return the global id corresponding to this ModuleId. */
  inline const GlobalId& getGlobalId() const { return global_id;}

  /** Print to stream. */
  std::ostream& print(std::ostream& o) const;

private:
  /** Extend the vector with names. */
  Index findOrAddName(const vstring& name);

  /** find the present name, throws an exception if not found. */
  Index findName(const vstring& name, Index level);

  /** Check for existence of an id. */
  static bool exists(const GlobalId& id);
};

DUECA_NS_END

PRINT_NS_START
inline ostream& operator << (ostream& o, const DUECA_NS::ModuleId& cl)
{ return cl.print(o); }
PRINT_NS_END
#endif
