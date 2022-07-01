/* ------------------------------------------------------------------   */
/*      item            : TrimId.hxx
        made by         : Rene van Paassen
        date            : 010926
        category        : header file
        description     :
        changes         : 010926 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TrimId_hxx
#define TrimId_hxx

#ifdef TrimId_cxx
#endif

#include <vector>
#include <map>
#include <stringoptions.h>
#include <GlobalId.hxx>
#include <IncoMode.hxx>
using namespace std;
#include <dueca_ns.h>
#include <IncoVariableWork.hxx>
DUECA_NS_START
struct IncoVariable;

/** Object for organising trim name information.

    For capturing and organising trim feedback reports from modules
    and their descendants (in a Dusime environment SimulationModule
    and HardwareModule), this ModuleId provides a link between the
    following:
    <ol>
    <li> The two id's of the TrimCalculation, and Incovariable
    <li> A list of names, giving the nameset of the module + the
         name of the trim variable
    <li> The hierarchy of entities with modules.
    </ol>
*/

class TrimId
{
  /** A type for indexation. */
  typedef int Index;

  /** a list with indexes to the names. The size of the list also
      gives my level into the hierarchy. An empty list stands for a
      (usually temporary) object that will be used to search with the
      GlobalId */
  std::vector<Index> name_idx;

  /** Number of the trim calculator handling this variable. */
  int calculator;

  /** Index for the calculator, to identify the variable. */
  int tvariable;

  /** A map of names. Because of the tree-like structure,
      usually a lot of name components are the same. Here, we use the
      indices to point to the proper parts in the vector. */
  static std::map<vstring, Index> name_map;

  /** A list of names, for going from index to the name. */
  static std::vector<vstring>     names;

  /** A global id to trick the Summary into not writing a location
      number into the ctree field. Bad design! */
  static GlobalId dummy_global_id;

  /** A map of the global id's, so we can quickly locate a TrimId
      from the GlobalId. */
  typedef std::vector<TrimId*> vpernode;

  /** Huh? */
  static std::vector<vpernode> module_id_map;

  /** Constructor, using a name */
  TrimId(const vector<vstring>& nameparts,
         int calculator, int tvariable);

  /** Copy constructor, private, not implemented. Should never copy */
  TrimId(const TrimId& o);

  /** Assignment, private, should never assign */
  TrimId& operator = (const TrimId& o);

  /** update the globalid index. */
  void indexThreeId();

public:

  /** Destructor. */
  ~TrimId();

  /** Find a TrimId using a GlobalId. */
  static TrimId& find(int calculator, int tvariable);

  /** Create a new TrimId. */
  static TrimId& create(const vector<vstring>& nameparts,
                        int calculator, int tvariable);

  /** Create a new TrimId with a limited number of parts. */
  static TrimId& create(const TrimId& id, int nparts);

  /** See whether some module is one of my secendants. */
  bool isMeOrDescendant(const TrimId& o) const;

  /** See whether this is me. */
  bool isMe(const TrimId& o);

  /** get the number of name parts. */
  int getNumParts() const {return name_idx.size();}

  /** get the value of the associated object. */
  double getValue() const;

  /** get a reference to the associated object. */
  IncoVariableWork& getIncoVariable() const;

  /** get last component of the name. */
  const char* getLast() const
  {
    static const char* none = "";
    if (name_idx.size()) {
      return names[name_idx[name_idx.size() - 1]].c_str();
    }
    return none;
  }

  /** Get the calculator number. */
  inline const int getCalculator() const { return calculator;}

  /** Get the variable id within the calculator. */
  inline const int getVariable() const { return tvariable;}

  /** Trick. */
  const GlobalId &getGlobalId() const  {return dummy_global_id;}

  /** Return the role as a string. */
  const char* getRoleString(IncoMode mode);

private:
  /** Extend the vector with names. */
  Index findOrAddName(const vstring& name);

  /** find the present name, throws an exception if not found. */
  Index findName(const vstring& name, Index level);

  /** print to stream */
  friend ostream& operator << (ostream& os, const TrimId& o);
};
DUECA_NS_END
#endif
